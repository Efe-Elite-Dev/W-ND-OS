#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>

/* =============================================================================
   MERKEZİ GRAFİK VE EKRAN FONKSİYON PROTOTİPLERİ
   ============================================================================= */

/* VBE Grafik modunu ilklendirir (GRUB ve boot.asm ile uyumlu) */
void init_graph_mode(void);

/* Belirtilen koordinata (x, y) 32-bit renkli piksel basar */
void draw_pixel_pure(int x, int y, uint32_t color);

/* Tüm grafik ekranını tek bir renkle kaplar */
void clear_screen_gfx(uint32_t color);

/* VGA Metin Ekranı Temizleme Fonksiyonu (Eski mod kalıntıları için) */
void clear_text_screen(void);

#endif /* SCREEN_H */
