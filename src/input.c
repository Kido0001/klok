/*
 * input.c - keybindings. Every CLI flag has a matching key so the
 * whole app is controllable without restarting it.
 *
 *   q / Q / Ctrl-C   quit (the ONLY things that quit)
 *   arrow keys       move the clock when centering is off
 *
 *   s   toggle seconds            c   toggle centering
 *   x   toggle box                b   toggle bold
 *   u   toggle UTC                t   toggle 12h/24h
 *   D   toggle date line          f   cycle date format (DMY/MDY/long)
 *   z   digit size +              Z   digit size -
 *   C   prompt: set hex color     T   toggle dynamic theme
 *
 *   p   pomodoro: big display     o   pomodoro: small (status-line) display
 *       (both start the timer if it's idle)
 *   space  pause/resume pomodoro  r   reset/disable pomodoro
 *   n   skip to next phase
 *
 *   w   toggle weather            W   prompt: set weather location
 *   U   cycle weather units (auto/metric/imperial)
 *
 *   ?   toggle this help overlay
 */

#include <string.h>
#include <stdio.h>
#include "klok.h"
#include "input.h"
#include "clock.h"
#include "color.h"
#include "pomodoro.h"
#include "theme.h"
#include "weather.h"

/* Re-lays-out the clock window after an option changed its size
 * (e.g. seconds, box, date line, digit size, pomodoro on/off).
 * Clears stdscr first so a shrinking window doesn't leave stale
 * cells behind. */
static void
relayout(void)
{
    clear();
    refresh();
    clock_compute_geometry();
    if (klok.win) {
        wresize(klok.win, klok.geo.h, klok.geo.w);
        mvwin(klok.win, klok.geo.y, klok.geo.x);
    }
}

static bool
prompt_text(const char *label, char *out, size_t out_sz)
{
    char buf[128] = {0};

    curs_set(1);
    echo();

    move(LINES - 1, 0);
    clrtoeol();
    mvprintw(LINES - 1, 0, "%s: ", label);
    refresh();

    wgetnstr(stdscr, buf, (int)sizeof(buf) - 1);

    noecho();
    curs_set(0);

    move(LINES - 1, 0);
    clrtoeol();
    refresh();

    if (buf[0] == '\0')
        return false;

    strncpy(out, buf, out_sz - 1);
    out[out_sz - 1] = '\0';
    return true;
}

static void
handle_color_prompt(void)
{
    char buf[64];
    if (!prompt_text("Hex color (#rrggbb)", buf, sizeof(buf)))
        return;

    klok_color_t parsed;
    if (color_parse_hex(buf, &parsed)) {
        klok.color = parsed;
        klok.theme.user_overrode = true;
        color_apply();
    }
    /* Silently ignore invalid input -- the prompt line already
     * cleared itself, no need for a modal error. */
}

static void
handle_weather_location_prompt(void)
{
    char buf[128];
    if (!prompt_text("Weather location (blank = auto)", buf, sizeof(buf)))
        buf[0] = '\0';
    weather_set_location(buf);
    relayout();
}

static void
cycle_date_format(void)
{
    klok.option.date_fmt = (date_fmt_t)((klok.option.date_fmt + 1) % DATE_FMT_COUNT);
}

void
input_handle_key(int ch)
{
    if (ch == ERR)
        return;

    if (ch == KEY_RESIZE) {
        klok_resize_pending = 1;
        return;
    }

    switch (ch) {
        case 'q': case 'Q': case 3: /* Ctrl-C */
            klok.running = false;
            return;

        case KEY_UP:    clock_move(0, -1); return;
        case KEY_DOWN:  clock_move(0, 1);  return;
        case KEY_LEFT:  clock_move(-1, 0); return;
        case KEY_RIGHT: clock_move(1, 0);  return;

        case 's': klok.option.seconds = !klok.option.seconds; relayout(); return;
        case 'c': klok.option.center = !klok.option.center; relayout(); return;
        case 'x': klok.option.box = !klok.option.box; relayout(); return;
        case 'b': klok.option.bold = !klok.option.bold; return;
        case 'u': klok.option.utc = !klok.option.utc; return;
        case 't': klok.option.twelve = !klok.option.twelve; return;
        case 'D': klok.option.show_date = !klok.option.show_date; relayout(); return;
        case 'f': cycle_date_format(); return;

        case 'z':
            if (klok.option.size < 6) klok.option.size++;
            relayout();
            return;
        case 'Z':
            if (klok.option.size > 1) klok.option.size--;
            relayout();
            return;

        case 'C': handle_color_prompt(); return;
        case 'T': theme_toggle(); return;

        case 'p': pomodoro_show_big();   relayout(); return;
        case 'o': pomodoro_show_small(); relayout(); return;
        case ' ': pomodoro_pause_resume(); return;
        case 'r': pomodoro_reset(); relayout(); return;
        case 'n': pomodoro_skip(); return;

        case 'w': weather_toggle(); relayout(); return;
        case 'W': handle_weather_location_prompt(); return;
        case 'U': weather_cycle_unit(); return;

        case '?': klok.show_help = !klok.show_help; return;

        default:
            return; /* unrecognized keys are ignored */
    }
}

static const char *HELP_LINES[] = {
    "klok keybindings",
    "",
    "q          quit",
    "arrows     move clock (when centering is off)",
    "",
    "s  seconds     c  centering     x  box       b  bold",
    "u  UTC         t  12h/24h       D  date line  f  date format",
    "z/Z  digit size +/-             C  set hex color",
    "T  dynamic theme",
    "",
    "p  pomodoro big display   o  pomodoro small display",
    "space  pause/resume       r  reset pomodoro   n  skip phase",
    "",
    "w  toggle weather   W  set weather location   U  cycle units",
    "",
    "?  toggle this help",
};
#define N_HELP_LINES (int)(sizeof(HELP_LINES) / sizeof(HELP_LINES[0]))

void
input_draw_help(void)
{
    if (!klok.show_help) {
        if (klok.helpwin) {
            delwin(klok.helpwin);
            klok.helpwin = NULL;
            clear();
            refresh();
        }
        return;
    }

    int w = 0;
    for (int i = 0; i < N_HELP_LINES; i++) {
        int len = (int)strlen(HELP_LINES[i]);
        if (len > w)
            w = len;
    }
    w += 4;
    int h = N_HELP_LINES + 2;

    if (w > COLS) w = COLS;
    if (h > LINES) h = LINES;

    int y = (LINES - h) / 2;
    int x = (COLS - w) / 2;
    if (y < 0) y = 0;
    if (x < 0) x = 0;

    if (!klok.helpwin) {
        klok.helpwin = newwin(h, w, y, x);
    } else {
        wresize(klok.helpwin, h, w);
        mvwin(klok.helpwin, y, x);
    }

    werase(klok.helpwin);
    box(klok.helpwin, 0, 0);
    for (int i = 0; i < N_HELP_LINES && i + 1 < h - 1; i++)
        mvwaddnstr(klok.helpwin, i + 1, 2, HELP_LINES[i], w - 4);

    wnoutrefresh(klok.helpwin);
    doupdate();
}

void
input_destroy(void)
{
    if (klok.helpwin) {
        delwin(klok.helpwin);
        klok.helpwin = NULL;
    }
}
