# klok

A lightweight terminal clock in C/ncurses, with:

- **Resizable windows**: reacts live to terminal resizes (`SIGWINCH` +
  `KEY_RESIZE`), re-centers and re-clamps automatically.
- **Pomodoro mode**: work/short-break/long-break cycle, either as a **big**
  countdown (replaces the clock digits) or a **small** one-line status under
  the normal clock. Pause/resume/skip/reset are all separate keys.
- **Weather**: optional one-line summary via [wttr.in](https://wttr.in),
  built only when you opt in (`make WITH_WEATHER=1`), so the base binary has
  zero network dependencies.
- **Dynamic theme**: clock color shifts automatically through night/dawn/day/dusk
  presets across the day, unless you pin a color yourself.
- **Dynamic thme base on cumputer theme**: Change your theme once in order to unlock this.
- **Hex colors**: `-C '#rrggbb'` (or the 3-digit shorthand `-C fff`) instead
  of the classic `-C [0-7]`, using ncurses' palette-redefinition where the
  terminal supports it, and falling back to the nearest of the 8 standard
  ANSI colors otherwise.
- **Everything is also a key**: every CLI flag has a matching in-app
  keybinding, so you never have to restart klok to change something. Press
  `?` while it's running for the full list.

klok is a from-scratch reimplementation, not a copy of any existing
project's source but it owes its concept to [xorg62/tty-clock](https://github.com/xorg62/tty-clock)
(the original minimal ncurses clock) and [arthur-dnts/ClockTemp](https://github.com/arthur-dnts/ClockTemp)
(which added weather/pomodoro/themes, in Python).

## Build

Requires `ncursesw` development headers.

```sh
sudo apt install libncursesw5-dev   # Debian/Ubuntu
make
./klok
```

To enable weather, you'll also need libcurl development headers:

```sh
sudo apt install libcurl4-openssl-dev
make clean && make WITH_WEATHER=1
```

## Keybindings

The only key that quits is **q** (or Ctrl-C). Every other key toggles or
adjusts something live:

```
q               quit
arrow keys      move the clock (only when centering is off)

s   seconds            c   centering
x   box                b   bold
u   UTC                t   12h/24h
D   date line           f   cycle date format (dd/mm/yyyy, mm/dd/yyyy, long)
z / Z  digit size +/-   C   prompt for a hex color
T   dynamic theme

p   pomodoro, big display    o   pomodoro, small display
    (both start the timer if it's idle)
space   pause/resume pomodoro
r   reset/disable pomodoro   n   skip to next phase

w   toggle weather      W   prompt for weather location
U   cycle weather units (auto/metric/imperial)

?   toggle this help overlay
```

## CLI options

```
klok [options]
  -s                 Show seconds
  -c                 Center the clock (default: on)
  -x                 Show a box around the clock
  -b                 Bold digits
  -u                 Use UTC time
  -t                 12-hour format
  -D                 Hide the date line
  -d <seconds>       Redraw delay (default 1)
  -C <#rrggbb|rgb>   Clock color as hex, e.g. -C '#ff8800' or -C fff
                     (default: white)
  -z <size>          Digit scale factor, 1-6 (default 1)
      --date-format <dmy|mdy|long>  Initial date format (default dmy)
  -p                 Enable pomodoro, big display
      --work <min>              Work length (default 25)
      --short-break <min>       Short break length (default 5)
      --long-break <min>        Long break length (default 15)
      --cycles <n>              Work cycles before a long break (default 4)
  -w [location]      Enable weather (blank = auto-detect by IP)
      --weather-refresh <sec>   Weather refresh interval (default 600)
      --weather-unit <auto|metric|imperial>  (default auto)
      --theme                   Enable the dynamic time-of-day theme
  -h                 Show help
  -v                 Show version
```

**Heads up on `-C` and your shell:** `#` starts a comment in bash, so
`-C #ff8800` typed _unquoted_ gets truncated by your shell before klok ever
sees it. Always quote it: `-C '#ff8800'` or just skip the `#` entirely,
`-C ff8800` works too.

Examples:

```sh
./klok -x -s -C 'ff5f5f'                  # boxed, seconds, custom red
./klok -p --work 50 --short-break 10      # 50/10 pomodoro, big display
./klok --theme                             # color drifts through the day
./klok -w Rotterdam                        # weather for Rotterdam
```

## Project layout

```
src/
  klok.h        shared state struct
  main.c        arg parsing, init, main loop
  input.c/.h    every keybinding (mirrors the CLI flags 1:1)
  clock.c/.h    7-segment digit font, geometry, resize handling, drawing
  color.c/.h    hex parsing, ncurses palette-redefinition + 8-color fallback
  pomodoro.c/.h work/break timer state machine (big + small display modes)
  theme.c/.h    time-of-day color bands
  weather.c/.h  optional libcurl + wttr.in fetch (WITH_WEATHER)
```

## Known limitations (v1)

- Weather fetches happen synchronously on the main thread, so klok will
  visibly pause for the duration of the HTTP request when its refresh
  interval elapses (default every 10 minutes), or when you set a new
  location/unit. Fine in practice; a future version could move this to a
  worker thread.
- Hex colors rely on `can_change_color()` (i.e. the terminal's terminfo
  entry advertising palette redefinition). Most terminals support this, but
  a few minimal/older terminfo entries don't.
- The dynamic theme uses local wall-clock hour bands, not real sunrise/sunset
  times.
- No config file yet everything is CLI flags / in-app keys.

## To-Do

- Everything must be toggles
- Tasks system
- Re-work of the weather system

## License

See [LICENSE](LICENSE). klok's code was written from scratch and does not
reuse source from tty-clock or ClockTemp, so it isn't bound by either
project's license terms but both are credited above as the inspiration.
