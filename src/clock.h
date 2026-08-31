#ifndef KLOK_CLOCK_H
#define KLOK_CLOCK_H

/* Creates the clock window and computes initial geometry. */
void clock_init(void);

/* Tears down ncurses windows owned by the clock module. */
void clock_destroy(void);

/* Refreshes klok.date from the current time (respects utc/twelve)
 * and formats klok.date.datestr from the active date_fmt. */
void clock_update_time(void);

/* Recomputes geo.x/y/w/h from COLS/LINES and current options. */
void clock_compute_geometry(void);

/* Handles a terminal resize: recompute geometry */
void clock_handle_resize(void);

/* Nudge the clock window by (dx, dy) character cells, clamped to
 * stay on screen. Only meaningful when option.center is false. */
void clock_move(int dx, int dy);

/* Draws the clock digits, optional box, date/weather line and
 * pomodoro status. */
void clock_draw(void);

#endif /* KLOK_CLOCK_H */
