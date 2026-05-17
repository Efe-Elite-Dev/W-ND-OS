#include "setup.h"

// Donanımsal VGA Grafik Bellek Adresi (Makro çakışmasını önleyen temiz pointer)
#define VGA_MEM_PTR ((uint8_t*)0xA0000)

// Donanımsal Kesme Sürücü Fonksiyonu
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static SetupStage current_stage = STAGE_WIND_COUNTRY;
static int button_press_depth = 0;
static int wind_anim_frame = 0;

void setup_init_palette(void) {
    // Grafik kartı renk paletini Windland OS konseptine göre boyuyoruz
    outb(0x3C8, COLOR_WIND_BG);       outb(0x3C9, 58); outb(0x3C9, 59); outb(0x3C9, 61);
    outb(0x3C8, COLOR_WIND_CARD);     outb(0x3C9, 63); outb(0x3C9, 63); outb(0x3C9, 63);
    outb(0x3C8, COLOR_WIND_TEXT);     outb(0x3C9, 5);  outb(0x3C9, 10); outb(0x3C9, 20);
    outb(0x3C8, COLOR_WIND_PRIMARY);  outb(0x3C9, 0);  outb(0x3C9, 40); outb(0x3C9, 58);
    outb(0x3C8, COLOR_WIND_SWIRL);    outb(0x3C9, 20); outb(0x3C9, 55); outb(0x3C9, 55);
    outb(0x3C8, COLOR_WIND_3D_LIGHT); outb(0x3C9, 63); outb(0x3C9, 63); outb(0x3C9, 63);
    outb(0x3C8, COLOR_WIND_3D_DARK);  outb(0x3C9, 30); outb(0x3C9, 30); outb(0x3C9, 32);
}

void setup_put_pixel(int x, int y, uint8_t color) {
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
        VGA_MEM_PTR[y * SCREEN_WIDTH + x] = color;
    }
}

void setup_draw_rect(int x, int y, int w, int h, uint8_t color) {
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            setup_put_pixel(x + j, y + i, color);
        }
    }
}

// Harika Kıvrımlı Rüzgarlı Fırtına Logoları Çizen Fonksiyon (Wind Swirls)
void draw_wind_swirl_vector(int cx, int cy, int size) {
    for (int i = 0; i < size; i++) {
        int offset_x = (i * i) / 20 + wind_anim_frame;
        int offset_y = i - (size / 2);
        setup_put_pixel(cx + offset_x, cy + offset_y, COLOR_WIND_SWIRL);
        setup_put_pixel(cx - offset_x, cy - offset_y, COLOR_WIND_TEXT);
        setup_put_pixel(cx + offset_y, cy - offset_x / 2, COLOR_WIND_SWIRL);
    }
}

// Fluent / 3D Hibrit Panel Çizici
void draw_window_card(int x, int y, int w, int h) {
    setup_draw_rect(x, y, w, h, COLOR_WIND_CARD);
    for (int i = 0; i < w; i++) {
        setup_put_pixel(x + i, y + h, COLOR_WIND_3D_DARK);
        setup_put_pixel(x + i, y + h + 1, COLOR_WIND_3D_DARK);
    }
    for (int i = 0; i < h; i++) {
        setup_put_pixel(x + w, y + i, COLOR_WIND_3D_DARK);
        setup_put_pixel(x + w + 1, y + i, COLOR_WIND_3D_DARK);
    }
}

// Mekanik Çökme Animasyonuna Sahip Akıllı Buton Sürücüsü
void draw_responsive_button(int x, int y, int w, int h, const char* label) {
    (void)label;
    int depth = button_press_depth;
    setup_draw_rect(x, y, w, h, COLOR_WIND_3D_DARK); 
    setup_draw_rect(x + depth, y + depth, w - depth, h - depth, COLOR_WIND_PRIMARY);
}

void setup_render(void) {
    // 1. Windland OS Gradyan Esintili Arka Plan
    setup_draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_WIND_BG);
    
    // 2. Ana İşlem Kartı (Center Panel)
    draw_window_card(20, 15, 280, 170);

    // 3. Kademelere Göre Görsel Vektör ve Form Çizimleri
    switch (current_stage) {
        case STAGE_WIND_COUNTRY:
            draw_wind_swirl_vector(80, 90, 40);
            setup_draw_rect(150, 40, 120, 15, COLOR_WIND_BG);
            setup_draw_rect(150, 60, 120, 15, COLOR_WIND_PRIMARY); 
            setup_draw_rect(150, 80, 120, 15, COLOR_WIND_BG);
            break;
            
        case STAGE_WIND_KEYBOARD:
            setup_draw_rect(50, 70, 60, 40, COLOR_WIND_BG);
            for(int k=0; k<4; k++) setup_draw_rect(55, 75 + (k*8), 50, 4, COLOR_WIND_TEXT);
            setup_draw_rect(150, 50, 120, 15, COLOR_WIND_PRIMARY); 
            break;
            
        case STAGE_WIND_NETWORK:
            draw_wind_swirl_vector(70, 90, 30);
            setup_draw_rect(140, 75, 130, 12, COLOR_WIND_BG); 
            break;
            
        case STAGE_WIND_HOSTNAME:
            setup_draw_rect(40, 80, 50, 35, COLOR_WIND_TEXT);
            setup_draw_rect(35, 115, 60, 5, COLOR_WIND_PRIMARY);
            setup_draw_rect(130, 85, 140, 15, COLOR_WIND_CARD); 
            setup_draw_rect(130, 100, 80, 2, COLOR_WIND_PRIMARY); 
            break;
            
        case STAGE_WIND_CUSTOMIZE:
            for(int i=0; i<3; i++) {
                setup_draw_rect(140, 45 + (i * 25), 10, 10, COLOR_WIND_BG);
                if (i == 0) setup_draw_rect(142, 47, 6, 6, COLOR_WIND_PRIMARY); 
            }
            break;
            
        default:
            draw_wind_swirl_vector(160, 100, 60);
            break;
    }

    // Alt Navigasyon "Sonraki / İleri" Mekanik Butonu
    draw_responsive_button(220, 155, 65, 20, "Ileri");
}

void setup_delay(int count) {
    volatile int i = 0;
    for (i = 0; i < count * 8000; i++) {
        __asm__("nop");
    }
}

void trigger_next_stage(void) {
    for (button_press_depth = 0; button_press_depth <= 3; button_press_depth++) {
        setup_render();
        setup_delay(8);
    }
    
    wind_anim_frame += 4;
    if (current_stage < STAGE_WIND_COMPLETE) {
        current_stage++;
    } else {
        current_stage = STAGE_WIND_COUNTRY; 
    }
    
    button_press_depth = 0;
    setup_render();
}

void setup_init(void) {
    setup_init_palette();
    setup_render();
}

void setup_handle_input(uint8_t scancode) {
    if (scancode == 0x1C) { // ENTER
        trigger_next_stage();
    }
}
