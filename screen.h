#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 200

extern uint8_t* vbe_vram;
extern uint32_t vbe_pitch;

// Küresel piksel fonksiyonu prototipi
void draw_pixel_pure(int x, int y, uint8_t color);

void screen_init(void);
void screen_put_pixel(int x, int y, uint8_t color);
void screen_flush(void);

#endif
