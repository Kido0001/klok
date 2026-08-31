/*
 * klok - a lightweight terminal clock in C/ncurses with resizable
 * windows, a pomodoro timer, weather, and a dynamic time-of-day
 * theme.
 *
 * Inspired by xorg62/tty-clock and arthur-dnts/ClockTemp, written
 * from scratch. Press '?' while running for the full keybinding
 * list -- every CLI flag below has a matching in-app key.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <locale.h>
#include <signal.h>

#include "klok.h"
#include "clock.h"
#include "color.h"
#include "pomodoro.h"
#include "theme.h"
#include "weather.h"
#include "input.h"

klok_t klok;
volatile sig_atomic_t klok_resize_pending = 0;

static void
on_winch(int sig)
{
    (void)sig;
    klok_resize_pending = 1;
}

static void
set_defaults(void)
{
    memset(&klok, 0, sizeof(klok));

    klok.option.seconds = false;
    klok.option.center = true;
    klok.option.box = false;
    klok.option.bold = false;
    klok.option.utc = false;
    klok.option.twelve = false;
    klok.option.show_date = true;
    klok.option.date_fmt = DATE_FMT_DMY;
    klok.option.size = 1;
    klok.option.delay_sec = 1;

    /* Default color: white, per spec. */
    klok.color.r = 0xFF;
    klok.color.g = 0xFF;
    klok.color.b = 0xFF;

    klok.pomo.enabled = false;
    klok.pomo.big = true;
    klok.theme.enabled = false;
    klok.theme.last_band = -1;
    klok.weather.enabled = false;
    klok.weather.unit = WEATHER_UNIT_AUTO;
    klok.weather.refresh_sec = 600; /* 10 min default */
}

static void
usage(const char *prog)
{
    printf(
        "usage: %s [options]\n"
        "  -s                 Show seconds\n"
        "  -c                 Center the clock (default: on)\n"
        "  -x                 Show a box around the clock\n"
        "  -b                 Bold digits\n"
        "  -u                 Use UTC time\n"
        "  -t                 12-hour format\n"
        "  -D                 Hide the date line\n"
        "  -d <seconds>       Redraw delay (default 1)\n"
        "  -C <#rrggbb|rgb>   Clock color as hex, e.g. -C '#ff8800' or -C fff\n"
        "  -z <size>          Digit scale factor, 1-6 (default 1)\n"
        "      --date-format <dmy|mdy|long>  Initial date format (default dmy)\n"
        "  -p                 Enable pomodoro, big display\n"
        "      --work <min>              Work length (default 25)\n"
        "      --short-break <min>       Short break length (default 5)\n"
        "      --long-break <min>        Long break length (default 15)\n"
        "      --cycles <n>              Work cycles before a long break (default 4)\n"
        "  -w [location]      Enable weather (blank = auto-detect by IP)\n"
        "      --weather-refresh <sec>   Weather refresh interval (default 600)\n"
        "      --weather-unit <auto|metric|imperial>  (default auto)\n"
        "      --theme                   Enable the dynamic time-of-day theme\n"
        "  -h                 Show this help\n"
        "  -v                 Show version\n"
        "\n"
        "Every option above also has an in-app key -- press '?' once klok\n"
        "is running to see the full keybinding list. The only quit key is q.\n",
        prog);
}

enum {
    OPT_WORK = 1000,
    OPT_SHORT_BREAK,
    OPT_LONG_BREAK,
    OPT_CYCLES,
    OPT_WEATHER_REFRESH,
    OPT_WEATHER_UNIT,
    OPT_THEME,
    OPT_DATE_FORMAT,
};

static struct option long_opts[] = {
    {"work",             required_argument, 0, OPT_WORK},
    {"short-break",       required_argument, 0, OPT_SHORT_BREAK},
    {"long-break",        required_argument, 0, OPT_LONG_BREAK},
    {"cycles",            required_argument, 0, OPT_CYCLES},
    {"weather-refresh",   required_argument, 0, OPT_WEATHER_REFRESH},
    {"weather-unit",       required_argument, 0, OPT_WEATHER_UNIT},
    {"theme",              no_argument,       0, OPT_THEME},
    {"date-format",        required_argument, 0, OPT_DATE_FORMAT},
    {0, 0, 0, 0}
};

static void
parse_args(int argc, char **argv)
{
    int ch;
    int longidx;

    while ((ch = getopt_long(argc, argv, "scxbutDd:C:z:pw::hv",
                              long_opts, &longidx)) != -1) {
        switch (ch) {
            case 's': klok.option.seconds = true; break;
            case 'c': klok.option.center = true; break;
            case 'x': klok.option.box = true; break;
            case 'b': klok.option.bold = true; break;
            case 'u': klok.option.utc = true; break;
            case 't': klok.option.twelve = true; break;
            case 'D': klok.option.show_date = false; break;
            case 'd': klok.option.delay_sec = atol(optarg); break;
            case 'C':
                if (!color_parse_hex(optarg, &klok.color)) {
                    fprintf(stderr,
                            "klok: invalid color '%s', expected #rrggbb (or #rgb)\n",
                            optarg);
                    exit(EXIT_FAILURE);
                }
                klok.theme.user_overrode = true;
                break;
            case 'z':
                klok.option.size = atoi(optarg);
                if (klok.option.size < 1) klok.option.size = 1;
                if (klok.option.size > 6) klok.option.size = 6;
                break;
            case 'p':
                klok.pomo.enabled = true;
                klok.pomo.big = true;
                break;
            case 'w':
                klok.weather.enabled = true;
                if (optarg) {
                    strncpy(klok.weather.location, optarg,
                             sizeof(klok.weather.location) - 1);
                } else if (optind < argc && argv[optind][0] != '-') {
                    strncpy(klok.weather.location, argv[optind],
                             sizeof(klok.weather.location) - 1);
                    optind++;
                }
                break;
            case 'h':
                usage(argv[0]);
                exit(EXIT_SUCCESS);
            case 'v':
                printf("klok %s\n", KLOK_VERSION);
                exit(EXIT_SUCCESS);

            case OPT_WORK:          klok.pomo.work_min = atoi(optarg); break;
            case OPT_SHORT_BREAK:    klok.pomo.short_break_min = atoi(optarg); break;
            case OPT_LONG_BREAK:     klok.pomo.long_break_min = atoi(optarg); break;
            case OPT_CYCLES:         klok.pomo.cycles_before_long_break = atoi(optarg); break;
            case OPT_WEATHER_REFRESH: klok.weather.refresh_sec = atoi(optarg); break;
            case OPT_WEATHER_UNIT:
                if (strcmp(optarg, "metric") == 0) klok.weather.unit = WEATHER_UNIT_METRIC;
                else if (strcmp(optarg, "imperial") == 0) klok.weather.unit = WEATHER_UNIT_IMPERIAL;
                else klok.weather.unit = WEATHER_UNIT_AUTO;
                break;
            case OPT_THEME:
                klok.theme.enabled = true;
                break;
            case OPT_DATE_FORMAT:
                if (strcmp(optarg, "mdy") == 0) klok.option.date_fmt = DATE_FMT_MDY;
                else if (strcmp(optarg, "long") == 0) klok.option.date_fmt = DATE_FMT_LONG;
                else klok.option.date_fmt = DATE_FMT_DMY;
                break;

            default:
                usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }
}

static void
init_ncurses(void)
{
    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    start_color();
    use_default_colors();
    clear();
    refresh();
}

int
main(int argc, char **argv)
{
    set_defaults();
    parse_args(argc, argv);

    pomodoro_init();
    bool start_pomo = klok.pomo.enabled;

    init_ncurses();
    color_init();
    clock_init();
    weather_init();

    if (start_pomo)
        pomodoro_show_big();

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_winch;
    sigaction(SIGWINCH, &sa, NULL);

    klok.running = true;
    wtimeout(klok.win, 1000);

    while (klok.running) {
        if (klok_resize_pending) {
            klok_resize_pending = 0;
            clock_handle_resize();
        }

        clock_update_time();
        theme_tick();
        pomodoro_tick();
        weather_tick();
        clock_draw();
        input_draw_help();

        int ch = wgetch(klok.win);
        input_handle_key(ch);
    }

    weather_shutdown();
    input_destroy();
    clock_destroy();
    endwin();
    return EXIT_SUCCESS;
}
