/*
 * pomodoro.c - work/break timer state machine.
 */

#include <string.h>
#include "klok.h"
#include "pomodoro.h"

static long
phase_length_sec(pomo_state_t s)
{
    switch (s) {
        case POMO_WORK:        return klok.pomo.work_min * 60L;
        case POMO_SHORT_BREAK: return klok.pomo.short_break_min * 60L;
        case POMO_LONG_BREAK:  return klok.pomo.long_break_min * 60L;
        default:                 return 0;
    }
}

void
pomodoro_init(void)
{
    if (klok.pomo.work_min <= 0)
        klok.pomo.work_min = 25;
    if (klok.pomo.short_break_min <= 0)
        klok.pomo.short_break_min = 5;
    if (klok.pomo.long_break_min <= 0)
        klok.pomo.long_break_min = 15;
    if (klok.pomo.cycles_before_long_break <= 0)
        klok.pomo.cycles_before_long_break = 4;
}

static void
enter_phase(pomo_state_t s)
{
    klok.pomo.state = s;
    klok.pomo.phase_started_at = time(NULL);
    klok.pomo.remaining_sec = phase_length_sec(s);
}

static void
advance_phase(void)
{
    switch (klok.pomo.state) {
        case POMO_WORK:
            klok.pomo.completed_work_cycles++;
            if (klok.pomo.completed_work_cycles % klok.pomo.cycles_before_long_break == 0)
                enter_phase(POMO_LONG_BREAK);
            else
                enter_phase(POMO_SHORT_BREAK);
            break;
        case POMO_SHORT_BREAK:
        case POMO_LONG_BREAK:
            enter_phase(POMO_WORK);
            break;
        default:
            break;
    }
}

void
pomodoro_tick(void)
{
    if (!klok.pomo.enabled)
        return;
    if (klok.pomo.state == POMO_IDLE || klok.pomo.state == POMO_PAUSED)
        return;

    time_t now = time(NULL);
    long elapsed = (long)(now - klok.pomo.phase_started_at);
    long total = phase_length_sec(klok.pomo.state);
    long remaining = total - elapsed;

    if (remaining <= 0)
        advance_phase();
    else
        klok.pomo.remaining_sec = remaining;
}

static void
ensure_running(void)
{
    klok.pomo.enabled = true;
    if (klok.pomo.state == POMO_IDLE)
        enter_phase(POMO_WORK);
}

void
pomodoro_show_big(void)
{
    ensure_running();
    klok.pomo.big = true;
}

void
pomodoro_show_small(void)
{
    ensure_running();
    klok.pomo.big = false;
}

void
pomodoro_pause_resume(void)
{
    if (!klok.pomo.enabled || klok.pomo.state == POMO_IDLE)
        return;

    if (klok.pomo.state == POMO_PAUSED) {
        klok.pomo.state = klok.pomo.resume_state;
        /* Keep remaining_sec accurate by re-basing phase_started_at
         * as if the phase had been running continuously. */
        klok.pomo.phase_started_at = time(NULL) -
            (phase_length_sec(klok.pomo.state) - klok.pomo.remaining_sec);
    } else {
        klok.pomo.resume_state = klok.pomo.state;
        klok.pomo.state = POMO_PAUSED;
    }
}

void
pomodoro_reset(void)
{
    bool was_big = klok.pomo.big;
    long work = klok.pomo.work_min;
    long sbrk = klok.pomo.short_break_min;
    long lbrk = klok.pomo.long_break_min;
    long cyc = klok.pomo.cycles_before_long_break;

    memset(&klok.pomo, 0, sizeof(klok.pomo));

    klok.pomo.work_min = (int)work;
    klok.pomo.short_break_min = (int)sbrk;
    klok.pomo.long_break_min = (int)lbrk;
    klok.pomo.cycles_before_long_break = (int)cyc;
    klok.pomo.big = was_big;
    klok.pomo.state = POMO_IDLE;
    klok.pomo.enabled = false;
}

void
pomodoro_skip(void)
{
    if (!klok.pomo.enabled)
        return;
    if (klok.pomo.state == POMO_IDLE)
        return;
    if (klok.pomo.state == POMO_PAUSED)
        klok.pomo.state = klok.pomo.resume_state;
    advance_phase();
}
