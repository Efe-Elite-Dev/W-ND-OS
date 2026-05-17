#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>

// Ekran çözünürlük ve VBE tampon tanımları
#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 200

// Küresel VBE değişkenlerinin dış bildirimi (Extern)
extern uint8_t* vbe_vram;
extern uint32_t vbe_pitch;

void screen_init(void);
void screen_put_pixel(int x, int y, uint8_t color);
void screen_flush(void);

#endif
