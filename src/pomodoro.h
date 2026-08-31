#ifndef KLOK_POMODORO_H
#define KLOK_POMODORO_H

#include <stdbool.h>

/* Fills in default lengths (25/5/15, 4 cycles) for any that weren't
 * set via CLI flags. Does not touch enabled/big/state. */
void pomodoro_init(void);

/* Call once per main-loop tick. Advances phases when time runs out
 * and updates klok.pomo.remaining_sec. No-op if !klok.pomo.enabled
 * or the timer is idle/paused. */
void pomodoro_tick(void);

/* Key actions */
void pomodoro_show_big(void);     /* enable (if needed) + big display; starts work if idle */
void pomodoro_show_small(void);   /* enable (if needed) + small display; starts work if idle */
void pomodoro_pause_resume(void);
void pomodoro_reset(void);        /* back to idle and disabled */
void pomodoro_skip(void);         /* jump to the next phase early */

#endif /* KLOK_POMODORO_H */
