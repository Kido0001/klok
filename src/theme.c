/*
 * theme.c - dynamic (time-of-day) color theme.
 *
 * When enabled with --theme (and the user hasn't pinned a color
 * with -C), klok shifts the clock color through a handful of
 * presets across the day: night / dawn / day / dusk. This is
 * intentionally simple (local wall-clock hour, no sunrise/sunset
 * lookup) to keep klok dependency-free.
 */

#include <time.h>
#include "klok.h"
#include "theme.h"
#include "color.h"

typedef struct {
    int start_hour;   /* inclusive, local time */
    klok_color_t color;
} band_t;

/* Ordered by start_hour. Last band wraps around to the first at
 * midnight. */
static const band_t BANDS[] = {
    {0,  {80,  80,  200}},   /* night: soft blue/violet */
    {6,  {255, 170, 90 }},   /* dawn: warm orange */
    {9,  {80,  200, 140}},   /* day: green */
    {17, {230, 140, 60 }},   /* dusk: amber */
    {20, {80,  80,  200}},   /* evening -> back to night tones */
};
#define N_BANDS (int)(sizeof(BANDS) / sizeof(BANDS[0]))

static int
band_for_hour(int hour)
{
    int chosen = 0;
    for (int i = 0; i < N_BANDS; i++) {
        if (hour >= BANDS[i].start_hour)
            chosen = i;
    }
    return chosen;
}

void
theme_toggle(void)
{
    klok.theme.enabled = !klok.theme.enabled;
    if (klok.theme.enabled) {
        klok.theme.user_overrode = false;
        klok.theme.last_band = -1;
    }
}

void
theme_tick(void)
{
    if (!klok.theme.enabled || klok.theme.user_overrode)
        return;

    time_t now = time(NULL);
    struct tm tmv;
    localtime_r(&now, &tmv);

    int band = band_for_hour(tmv.tm_hour);
    if (band == klok.theme.last_band)
        return;

    klok.theme.last_band = band;
    klok.color = BANDS[band].color;
    color_apply();
}
