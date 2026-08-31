/*
 * klok.h - shared state for klok
 *
 * klok is a lightweight terminal clock written in C with ncurses.
 * It started as a from-scratch reimplementation inspired by
 * xorg62/tty-clock (https://github.com/xorg62/tty-clock) and the
 * feature set of arthur-dnts/ClockTemp, but shares no source code
 * with either project.
 */

#ifndef KLOK_H
#define KLOK_H

#include <ncurses.h>
#include <stdbool.h>
#include <time.h>
#include <signal.h>

#define KLOK_VERSION "0.2.0"

/* Digit glyph is a classic 7-segment shape: 6 columns x 5 rows.
 * option.size repeats each cell of this grid it never changes
 * the underlying 6x5 segment layout, so digits stay readable and
 * on-model at every scale. */
#define DIGIT_COLS 6
#define DIGIT_ROWS 5
#define COLON_COLS 2

typedef struct {
    int r, g, b;   /* 0-255 each */
} klok_color_t;

typedef enum {
    POMO_IDLE = 0,
    POMO_WORK,
    POMO_SHORT_BREAK,
    POMO_LONG_BREAK,
    POMO_PAUSED
} pomo_state_t;

typedef struct {
    bool enabled;
    bool big;                    /* true = big digit countdown, false = one status line */
    pomo_state_t state;
    pomo_state_t resume_state;   /* state to return to after unpausing */
    time_t phase_started_at;
    int work_min;
    int short_break_min;
    int long_break_min;
    int cycles_before_long_break;
    int completed_work_cycles;
    long remaining_sec;          /* recomputed each tick */
} pomodoro_t;

typedef struct {
    bool enabled;         /* dynamic theme on/off */
    bool user_overrode;   /* true once the user picks a color -> theme stops touching it */
    int last_band;        /* last time-of-day band applied, -1 = none yet */
} theme_t;

typedef enum {
    WEATHER_UNIT_AUTO = 0,  /* let the provider decide from IP geolocation */
    WEATHER_UNIT_METRIC,
    WEATHER_UNIT_IMPERIAL,
    WEATHER_UNIT_COUNT
} weather_unit_t;

typedef struct {
    bool enabled;
    char location[128];    /* empty = auto (provider geolocates by IP) */
    weather_unit_t unit;
    char line[160];          /* one-line summary from the provider */
    time_t last_fetch;
    int refresh_sec;
    bool have_data;
    bool fetch_failed;
} weather_t;

typedef enum {
    DATE_FMT_DMY = 0,   /* 30/08/2026 */
    DATE_FMT_MDY,        /* 08/30/2026 */
    DATE_FMT_LONG,        /* Sat. 30 August 2026 */
    DATE_FMT_COUNT
} date_fmt_t;

typedef struct {
    bool running;

    WINDOW *win;       /* clock window */
    WINDOW *helpwin;    /* keybinding overlay, NULL when hidden */

    struct {
        bool seconds;
        bool center;
        bool box;
        bool bold;
        bool utc;
        bool twelve;
        bool show_date;
        date_fmt_t date_fmt;
        long delay_sec;
        int size;          /* digit scale, >=1, repeats each segment cell */
    } option;

    struct {
        int x, y;          /* top-left of clock window */
        int w, h;          /* clock window size */
    } geo;

    struct {
        unsigned int hour[2];
        unsigned int minute[2];
        unsigned int second[2];
        char datestr[64];  /* resolved by clock_update_time() from date_fmt */
    } date;

    klok_color_t color;

    pomodoro_t pomo;
    theme_t theme;
    weather_t weather;

    bool show_help;
} klok_t;

extern klok_t klok;
extern volatile sig_atomic_t klok_resize_pending;

#endif /* KLOK_H */
