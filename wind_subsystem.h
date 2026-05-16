#ifndef WIND_SUBSYSTEM_H
#define WIND_SUBSYSTEM_H

#include <stdint.h>

/* Donanım Port Köprüleri */
void outb(unsigned short port, unsigned char val);
unsigned char inb(unsigned short port);

/* Grafik ve Ekran Sürücü Fonksiyonları */
void init_graph_mode(void);
void draw_pixel_pure(int x, int y, uint32_t color);
void clear_screen_gfx(uint32_t color);
void draw_window_pure(int x, int y, int width, int height, uint32_t border_color);

/* Altsistem Fonksiyonları */
void clear_text_screen(void);
void init_idt(void);
void init_keyboard(void);
void init_mouse(void);
void handle_mouse_polling(void);
void check_keyboard_pure(void);
void gui_refresh_desktop(void);
void run_exe_subsystem(void);

/* AI Tahmin Motoru Fonksiyonları */
int ai_mouse_analyze_stress(void);
int ai_keyboard_analyze_cadence(void);
int ai_core_predict_scheduler(int stress, int cadence, int loops);

#endif
