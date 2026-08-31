/*
 * color.c - hex color support for klok's -C flag.
 *
 * ncurses' classic API only ships 8 "named" color slots (0-7),
 * which is what the original tty-clock's -C [0-7] maps onto. To
 * support arbitrary #RRGGBB values instead, we redefine one unused
 * palette slot with init_color() + init_pair() -- this only works
 * if the terminal (per its terminfo entry) supports palette
 * redefinition, reported by can_change_color(). Most modern
 * terminal emulators do, including the Linux console and any
 * xterm-256color-class terminal; a handful of bare/minimal
 * terminfo entries don't advertise it even if the real terminal
 * could handle it.
 *
 * When it isn't available we fall back to the nearest of the 8
 * standard ANSI colors, so klok still runs everywhere -- just with
 * less color fidelity on those terminals.
 */

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "color.h"

static bool have_custom_color = false;
static int custom_color_id = 8;
static int custom_pair_id = 9;
static int fallback_ansi = COLOR_WHITE;

static void
trim(char *s)
{
    /* strip leading/trailing whitespace, including a trailing
     * newline some shells/readers may leave in. */
    size_t start = 0;
    while (s[start] && isspace((unsigned char)s[start]))
        start++;
    size_t end = strlen(s);
    while (end > start && isspace((unsigned char)s[end - 1]))
        end--;
    size_t len = end - start;
    memmove(s, s + start, len);
    s[len] = '\0';
}

bool
color_parse_hex(const char *hexstr, klok_color_t *out)
{
    if (!hexstr || !out)
        return false;

    char buf[32];
    strncpy(buf, hexstr, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    trim(buf);

    const char *p = buf;
    if (*p == '#')
        p++;

    size_t len = strlen(p);
    char expanded[7];

    if (len == 3) {
        /* CSS-style shorthand, e.g. "fff" -> "ffffff" */
        for (int i = 0; i < 3; i++) {
            if (!isxdigit((unsigned char)p[i]))
                return false;
            expanded[i * 2] = p[i];
            expanded[i * 2 + 1] = p[i];
        }
        expanded[6] = '\0';
        p = expanded;
        len = 6;
    } else if (len == 6) {
        for (size_t i = 0; i < len; i++) {
            if (!isxdigit((unsigned char)p[i]))
                return false;
        }
    } else {
        return false;
    }

    char nb[3] = {0};
    nb[0] = p[0]; nb[1] = p[1];
    out->r = (int)strtol(nb, NULL, 16);
    nb[0] = p[2]; nb[1] = p[3];
    out->g = (int)strtol(nb, NULL, 16);
    nb[0] = p[4]; nb[1] = p[5];
    out->b = (int)strtol(nb, NULL, 16);
    return true;
}

/* Rough nearest-of-8 mapping for terminals without palette
 * redefinition support. */
static int
nearest_ansi(int r, int g, int b)
{
    static const struct { int r, g, b; int id; } palette[] = {
        {0,   0,   0,   COLOR_BLACK},
        {205, 0,   0,   COLOR_RED},
        {0,   205, 0,   COLOR_GREEN},
        {205, 205, 0,   COLOR_YELLOW},
        {0,   0,   238, COLOR_BLUE},
        {205, 0,   205, COLOR_MAGENTA},
        {0,   205, 205, COLOR_CYAN},
        {229, 229, 229, COLOR_WHITE},
    };

    int best_id = COLOR_WHITE;
    long best_dist = -1;

    for (size_t i = 0; i < sizeof(palette) / sizeof(palette[0]); i++) {
        long dr = r - palette[i].r;
        long dg = g - palette[i].g;
        long db = b - palette[i].b;
        long dist = dr * dr + dg * dg + db * db;
        if (best_dist < 0 || dist < best_dist) {
            best_dist = dist;
            best_id = palette[i].id;
        }
    }
    return best_id;
}

void
color_init(void)
{
    /* Only require can_change_color() -- COLORS as low as 8 (e.g.
     * the Linux VT) can still support palette redefinition. */
    have_custom_color = can_change_color();
    if (have_custom_color) {
        custom_color_id = (COLORS > 16) ? 16 : (COLORS - 1);
        if (custom_color_id < 0)
            custom_color_id = 0;
    }
    color_apply();
}

void
color_apply(void)
{
    int r = klok.color.r, g = klok.color.g, b = klok.color.b;

    if (have_custom_color) {
        init_color(custom_color_id,
                    r * 1000 / 255, g * 1000 / 255, b * 1000 / 255);
        init_pair(custom_pair_id, custom_color_id, -1);
        return;
    }

    fallback_ansi = nearest_ansi(r, g, b);
    init_pair(custom_pair_id, fallback_ansi, -1);
}

void
color_on(WINDOW *w)
{
    wattron(w, COLOR_PAIR(custom_pair_id));
    if (klok.option.bold)
        wattron(w, A_BOLD);
}

void
color_off(WINDOW *w)
{
    wattroff(w, COLOR_PAIR(custom_pair_id));
    if (klok.option.bold)
        wattroff(w, A_BOLD);
}
