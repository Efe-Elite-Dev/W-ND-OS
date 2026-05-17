#ifndef GUI_H
#define GUI_H

#include <stdint.h>

// Temel Çizim Fonksiyonları
void draw_rect_pure(int x, int y, int w, int h, uint8_t color);

// Derleme Hatasını Çözen Güncel Pencere Fonksiyon İmzası
// Son parametreyi tam olarak gui.c içindeki gibi 'const char* title' yapıyoruz
void draw_window_pure(int x, int y, int w, int h, const char* title);

// GUI Başlatıcı
void gui_init(void);

#endif
