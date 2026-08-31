#ifndef KLOK_THEME_H
#define KLOK_THEME_H

/* Turns the dynamic theme on/off (key 'T'). Turning it on clears
 * any user-pinned color override so the theme takes effect right
 * away. */
void theme_toggle(void);

void theme_tick(void);

#endif /* KLOK_THEME_H */
