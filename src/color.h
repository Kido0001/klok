#ifndef KLOK_COLOR_H
#define KLOK_COLOR_H

#include <stdbool.h>
#include "klok.h"

/* Parse "#RRGGBB" or "RRGGBB" into r,g,b (0-255). Note: '#' is stripped
 * if present, but you can pass the value without it too -- doing so
 * avoids a common shell gotcha where an unquoted "-C #ffffff" gets
 * the "#ffffff" part eaten as a comment by the shell. */
bool color_parse_hex(const char *hexstr, klok_color_t *out);

/* Must be called after start_color(). Sets up the color pair used
 * for drawing the clock digits/box from the current klok.color. */
void color_init(void);

/* Re-applies klok.color to the digit color pair. Call again any
 * time klok.color changes (e.g. dynamic theme tick, or resize on
 * some terminals that reset custom colors). */
void color_apply(void);

void color_on(WINDOW *w);
void color_off(WINDOW *w);

#endif /* KLOK_COLOR_H */
