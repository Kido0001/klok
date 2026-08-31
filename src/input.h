#ifndef KLOK_INPUT_H
#define KLOK_INPUT_H

void input_handle_key(int ch);

/* Draws (or erases, if klok.show_help is false) the keybinding
 * help overlay. Call once per loop iteration after clock_draw(). */
void input_draw_help(void);

/* Frees the help overlay window, if any. Call on shutdown. */
void input_destroy(void);

#endif /* KLOK_INPUT_H */
