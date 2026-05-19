#include <stdint.h>
#include "sky_core.h"

// Ekran boyutları (Sabit)
#define SCREEN_W 1024
#define SCREEN_H 768

// Modern kartın koordinatları
#define CARD_W 600
#define CARD_H 400
#define CARD_X (SCREEN_W - CARD_W) / 2
#define CARD_Y (SCREEN_H - CARD_H) / 2

// Gölge çizimi için fonksiyon (Modern hava verir)
void draw_shadow(int x, int y, int w, int h, int offset) {
    draw_rect(x + offset, y + offset, w, h, 0x33000000); // Yarı saydam siyah
}

void render_ui() {
    // 1. Arka Plan: Modern Degrade (Gradient)
    for (int i = 0; i < SCREEN_H; i++) {
        uint32_t r = 30 + (i / 10); // Hafif morumsu/mavi ton
        uint32_t g = 30 + (i / 15);
        uint32_t b = 60 + (i / 10);
        uint32_t color = (0xFF << 24) | (r << 16) | (g << 8) | b;
        for (int j = 0; j < SCREEN_W; j++) draw_pixel(j, i, color);
    }

    // 2. Kartın Gölgesi (Derinlik efekti)
    draw_shadow(CARD_X, CARD_Y, CARD_W, CARD_H, 15);

    // 3. Ana Kart (Beyaz, yumuşak köşeli)
    draw_rounded_rect(CARD_X, CARD_Y, CARD_W, CARD_H, 20, 0xFFFFFFFF);

    // 4. Mavi Buton (Modern aksan)
    draw_rounded_rect(CARD_X + 20, CARD_Y + 300, 140, 50, 10, 0xFF0078D7);
}
