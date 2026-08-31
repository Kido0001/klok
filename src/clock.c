/* clock.c - digit rendering, layout, resize handling. */

#include <stdio.h>
#include <string.h>
#include "klok.h"
#include "clock.h"
#include "color.h"

typedef unsigned char bitmap6x5_t[DIGIT_ROWS][DIGIT_COLS];

static const bitmap6x5_t DIGIT_FONT[10] = {
    /* 0 */ {{1,1,1,1,1,1},{1,1,0,0,1,1},{1,1,0,0,1,1},{1,1,0,0,1,1},{1,1,1,1,1,1}},
    /* 1 */ {{0,0,0,0,1,1},{0,0,0,0,1,1},{0,0,0,0,1,1},{0,0,0,0,1,1},{0,0,0,0,1,1}},
    /* 2 */ {{1,1,1,1,1,1},{0,0,0,0,1,1},{1,1,1,1,1,1},{1,1,0,0,0,0},{1,1,1,1,1,1}},
    /* 3 */ {{1,1,1,1,1,1},{0,0,0,0,1,1},{1,1,1,1,1,1},{0,0,0,0,1,1},{1,1,1,1,1,1}},
    /* 4 */ {{1,1,0,0,1,1},{1,1,0,0,1,1},{1,1,1,1,1,1},{0,0,0,0,1,1},{0,0,0,0,1,1}},
    /* 5 */ {{1,1,1,1,1,1},{1,1,0,0,0,0},{1,1,1,1,1,1},{0,0,0,0,1,1},{1,1,1,1,1,1}},
    /* 6 */ {{1,1,1,1,1,1},{1,1,0,0,0,0},{1,1,1,1,1,1},{1,1,0,0,1,1},{1,1,1,1,1,1}},
    /* 7 */ {{1,1,1,1,1,1},{0,0,0,0,1,1},{0,0,0,0,1,1},{0,0,0,0,1,1},{0,0,0,0,1,1}},
    /* 8 */ {{1,1,1,1,1,1},{1,1,0,0,1,1},{1,1,1,1,1,1},{1,1,0,0,1,1},{1,1,1,1,1,1}},
    /* 9 */ {{1,1,1,1,1,1},{1,1,0,0,1,1},{1,1,1,1,1,1},{0,0,0,0,1,1},{1,1,1,1,1,1}},
};

static const unsigned char COLON_FONT[DIGIT_ROWS][COLON_COLS] = {
    {0,0}, {1,1}, {0,0}, {1,1}, {0,0}
};

static bool
using_pomo_big(void)
{
    return klok.pomo.enabled && klok.pomo.big && klok.pomo.state != POMO_IDLE;
}

static int
build_glyphs(char *out, size_t out_sz)
{
    int n;

    if (using_pomo_big()) {
        long m = klok.pomo.remaining_sec / 60;
        long s = klok.pomo.remaining_sec % 60;
        if (m > 99) m = 99;
        if (m < 0) m = 0;
        if (s < 0) s = 0;
        n = snprintf(out, out_sz, "%02ld:%02ld", m, s);
    } else if (klok.option.seconds) {
        n = snprintf(out, out_sz, "%u%u:%u%u:%u%u",
                      klok.date.hour[0], klok.date.hour[1],
                      klok.date.minute[0], klok.date.minute[1],
                      klok.date.second[0], klok.date.second[1]);
    } else {
        n = snprintf(out, out_sz, "%u%u:%u%u",
                      klok.date.hour[0], klok.date.hour[1],
                      klok.date.minute[0], klok.date.minute[1]);
    }
    if (n < 0)
        return 0;
    return (int)strlen(out);
}

static const char *
date_format_pattern(void)
{
    switch (klok.option.date_fmt) {
        case DATE_FMT_DMY:  return "%d/%m/%Y";
        case DATE_FMT_MDY:  return "%m/%d/%Y";
        case DATE_FMT_LONG: return "%a. %d %B %Y";
        default:             return "%d/%m/%Y";
    }
}

void
clock_update_time(void)
{
    time_t t = time(NULL);
    struct tm tmv;

    if (klok.option.utc)
        gmtime_r(&t, &tmv);
    else
        localtime_r(&t, &tmv);

    int h = tmv.tm_hour;
    if (klok.option.twelve) {
        h = h % 12;
        if (h == 0)
            h = 12;
    }

    klok.date.hour[0] = h / 10;
    klok.date.hour[1] = h % 10;
    klok.date.minute[0] = tmv.tm_min / 10;
    klok.date.minute[1] = tmv.tm_min % 10;
    klok.date.second[0] = tmv.tm_sec / 10;
    klok.date.second[1] = tmv.tm_sec % 10;

    strftime(klok.date.datestr, sizeof(klok.date.datestr),
              date_format_pattern(), &tmv);
}

static void
glyph_widths(int *digit_w, int *colon_w, int *gap)
{
    int size = klok.option.size;
    *digit_w = DIGIT_COLS * size;
    *colon_w = COLON_COLS * size;
    *gap = size;
}

/* Width in glyph slots for the current display (digits + colons). */
static int
content_dims(int *out_w, int *out_h)
{
    char glyphs[16];
    int n = build_glyphs(glyphs, sizeof(glyphs));
    int digit_w, colon_w, gap;
    glyph_widths(&digit_w, &colon_w, &gap);

    int w = 0;
    for (int i = 0; i < n; i++) {
        w += (glyphs[i] == ':') ? colon_w : digit_w;
        if (i < n - 1)
            w += gap;
    }

    *out_w = w;
    *out_h = DIGIT_ROWS * klok.option.size;
    return n;
}

void
clock_compute_geometry(void)
{
    int content_w, content_h;
    content_dims(&content_w, &content_h);

    int border = klok.option.box ? 1 : 0;
    int extra_h = 0;
    if (using_pomo_big())
        extra_h += 1; /* phase label line above the big digits */
    if (klok.option.show_date || klok.weather.enabled)
        extra_h += 2; /* blank line + date/weather line */
    if (klok.pomo.enabled && !klok.pomo.big)
        extra_h += 2; /* blank line + pomodoro status line */

    klok.geo.w = content_w + border * 2;
    klok.geo.h = content_h + border * 2 + extra_h;

    if (klok.geo.w > COLS)
        klok.geo.w = COLS;
    if (klok.geo.h > LINES)
        klok.geo.h = LINES;
    if (klok.geo.w < 1)
        klok.geo.w = 1;
    if (klok.geo.h < 1)
        klok.geo.h = 1;

    if (klok.option.center) {
        klok.geo.x = (COLS - klok.geo.w) / 2;
        klok.geo.y = (LINES - klok.geo.h) / 2;
    } else {
        if (klok.geo.x + klok.geo.w > COLS)
            klok.geo.x = COLS - klok.geo.w;
        if (klok.geo.y + klok.geo.h > LINES)
            klok.geo.y = LINES - klok.geo.h;
    }
    if (klok.geo.x < 0)
        klok.geo.x = 0;
    if (klok.geo.y < 0)
        klok.geo.y = 0;
}

void
clock_init(void)
{
    klok.geo.x = 0;
    klok.geo.y = 0;
    clock_compute_geometry();

    klok.win = newwin(klok.geo.h, klok.geo.w, klok.geo.y, klok.geo.x);
    keypad(klok.win, true);
}

void
clock_destroy(void)
{
    if (klok.win) {
        delwin(klok.win);
        klok.win = NULL;
    }
    if (klok.helpwin) {
        delwin(klok.helpwin);
        klok.helpwin = NULL;
    }
}

void
clock_handle_resize(void)
{
    endwin();
    refresh();
    clear();

    clock_compute_geometry();

    if (klok.win) {
        wresize(klok.win, klok.geo.h, klok.geo.w);
        mvwin(klok.win, klok.geo.y, klok.geo.x);
    } else {
        klok.win = newwin(klok.geo.h, klok.geo.w, klok.geo.y, klok.geo.x);
        keypad(klok.win, true);
    }

    clock_draw();
}

void
clock_move(int dx, int dy)
{
    if (klok.option.center)
        return;

    klok.geo.x += dx;
    klok.geo.y += dy;
    clock_compute_geometry();

    if (klok.win)
        mvwin(klok.win, klok.geo.y, klok.geo.x);
}

static void
draw_block(WINDOW *w, int top, int left, int size)
{
    for (int yy = 0; yy < size; yy++) {
        wmove(w, top + yy, left);
        for (int xx = 0; xx < size; xx++)
            waddch(w, ' ' | A_REVERSE);
    }
}

static void
draw_digit(WINDOW *w, int top, int left, int digit, int size)
{
    for (int r = 0; r < DIGIT_ROWS; r++)
        for (int c = 0; c < DIGIT_COLS; c++)
            if (DIGIT_FONT[digit][r][c])
                draw_block(w, top + r * size, left + c * size, size);
}

static void
draw_colon(WINDOW *w, int top, int left, int size)
{
    for (int r = 0; r < DIGIT_ROWS; r++)
        for (int c = 0; c < COLON_COLS; c++)
            if (COLON_FONT[r][c])
                draw_block(w, top + r * size, left + c * size, size);
}

static void
draw_centered(WINDOW *w, int row, int width, const char *text)
{
    int len = (int)strlen(text);
    int col = (width - len) / 2;
    if (col < 0)
        col = 0;
    wmove(w, row, 0);
    wclrtoeol(w);
    mvwaddnstr(w, row, col, text, width);
}

static void
draw_date_weather_line(WINDOW *w, int row, int width)
{
    char line[256] = {0};

    if (klok.option.show_date && klok.weather.enabled && klok.weather.have_data) {
        snprintf(line, sizeof(line), "%s \xC2\xB7 %s",
                  klok.date.datestr, klok.weather.line);
    } else if (klok.option.show_date) {
        snprintf(line, sizeof(line), "%s", klok.date.datestr);
    } else if (klok.weather.enabled && klok.weather.have_data) {
        snprintf(line, sizeof(line), "%s", klok.weather.line);
    } else if (klok.weather.enabled && klok.weather.fetch_failed) {
        snprintf(line, sizeof(line), "weather: unavailable");
    } else {
        return;
    }

    draw_centered(w, row, width, line);
}

static void
draw_pomo_status_line(WINDOW *w, int row, int width)
{
    if (!klok.pomo.enabled || klok.pomo.big)
        return;

    const char *label = "Pomodoro";
    switch (klok.pomo.state) {
        case POMO_WORK:        label = "Focus";      break;
        case POMO_SHORT_BREAK:  label = "Break";      break;
        case POMO_LONG_BREAK:   label = "Long break"; break;
        case POMO_PAUSED:       label = "Paused";     break;
        case POMO_IDLE:         label = "Pomodoro (press p or o to start)"; break;
    }

    char line[96];
    if (klok.pomo.state == POMO_IDLE) {
        snprintf(line, sizeof(line), "%s", label);
    } else {
        long m = klok.pomo.remaining_sec / 60;
        long s = klok.pomo.remaining_sec % 60;
        snprintf(line, sizeof(line), "%s %02ld:%02ld", label, m, s);
    }

    draw_centered(w, row, width, line);
}

static void
draw_pomo_label_line(WINDOW *w, int row, int width)
{
    /* Shown above the big digits when the pomodoro is driving them. */
    if (!using_pomo_big())
        return;

    const char *label = "Focus";
    switch (klok.pomo.state) {
        case POMO_SHORT_BREAK: label = "Break";      break;
        case POMO_LONG_BREAK:  label = "Long break"; break;
        case POMO_PAUSED:      label = "Paused";     break;
        default:                 label = "Focus";      break;
    }
    draw_centered(w, row, width, label);
}

void
clock_draw(void)
{
    if (!klok.win)
        return;

    werase(klok.win);

    int border = klok.option.box ? 1 : 0;
    if (klok.option.box)
        box(klok.win, 0, 0);

    char glyphs[16];
    int n = build_glyphs(glyphs, sizeof(glyphs));
    int digit_w, colon_w, gap;
    glyph_widths(&digit_w, &colon_w, &gap);
    int size = klok.option.size;

    int top = border;
    int left = border;
    int inner_w = klok.geo.w - border * 2;

    if (using_pomo_big()) {
        draw_pomo_label_line(klok.win, top, inner_w);
        top++;
    }

    color_on(klok.win);
    for (int i = 0; i < n; i++) {
        if (glyphs[i] == ':') {
            draw_colon(klok.win, top, left, size);
            left += colon_w + gap;
        } else {
            draw_digit(klok.win, top, left, glyphs[i] - '0', size);
            left += digit_w + gap;
        }
    }
    color_off(klok.win);

    int row = top + DIGIT_ROWS * size;

    if (klok.option.show_date || klok.weather.enabled) {
        row++;
        draw_date_weather_line(klok.win, row, inner_w);
        row++;
    }

    if (klok.pomo.enabled && !klok.pomo.big) {
        row++;
        draw_pomo_status_line(klok.win, row, inner_w);
    }

    wnoutrefresh(klok.win);
    doupdate();
}
