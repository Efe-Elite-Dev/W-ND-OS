#include "setup.h"

#define VGA_MEM_PTR ((uint8_t*)0xA0000)

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static SetupStage current_stage = STAGE_WIND_COUNTRY;
static int button_press_depth = 0;
static int wind_anim_frame = 0;

void setup_init_palette(void) {
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

void draw_wind_swirl_vector(int cx, int cy, int size) {
    for (int i = 0; i < size; i++) {
        int offset_x = (i * i) / 20 + wind_anim_frame;
        int offset_y = i - (size / 2);
        setup_put_pixel(cx + offset_x, cy + offset_y, COLOR_WIND_SWIRL);
        setup_put_pixel(cx - offset_x, cy - offset_y, COLOR_WIND_TEXT);
    }
}

void draw_window_card(int x, int y, int w, int h) {
    setup_draw_rect(x, y, w, h, COLOR_WIND_CARD);
    for (int i = 0; i < w; i++) {
        setup_put_pixel(x + i, y + h, COLOR_WIND_3D_DARK);
    }
    for (int i = 0; i < h; i++) {
        setup_put_pixel(x + w, y + i, COLOR_WIND_3D_DARK);
    }
}

void draw_responsive_button(int x, int y, int w, int h) {
    int depth = button_press_depth;
    setup_draw_rect(x, y, w, h, COLOR_WIND_3D_DARK); 
    setup_draw_rect(x + depth, y + depth, w - depth, h - depth, COLOR_WIND_PRIMARY);
}

void setup_render(void) {
    setup_draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_WIND_BG);
    draw_window_card(20, 15, 280, 170);

    switch (current_stage) {
        case STAGE_WIND_COUNTRY:
            draw_wind_swirl_vector(80, 90, 40);
            setup_draw_rect(150, 60, 120, 15, COLOR_WIND_PRIMARY); 
            break;
        case STAGE_WIND_KEYBOARD:
            setup_draw_rect(50, 70, 60, 40, COLOR_WIND_BG);
            setup_draw_rect(150, 50, 120, 15, COLOR_WIND_PRIMARY); 
            break;
        case STAGE_WIND_NETWORK:
            draw_wind_swirl_vector(70, 90, 30);
            setup_draw_rect(140, 75, 130, 12, COLOR_WIND_BG); 
            break;
        case STAGE_WIND_HOSTNAME:
            setup_draw_rect(40, 80, 50, 35, COLOR_WIND_TEXT);
            setup_draw_rect(130, 85, 140, 15, COLOR_WIND_BG); 
            break;
        case STAGE_WIND_CUSTOMIZE:
            setup_draw_rect(140, 45, 10, 10, COLOR_WIND_PRIMARY);
            break;
        default:
            draw_wind_swirl_vector(160, 100, 60);
            break;
    }
    draw_responsive_button(220, 155, 65, 20);
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
    if (current_stage < STAGE_WIND_COMPLETE) current_stage++;
    else current_stage = STAGE_WIND_COUNTRY; 
    
    button_press_depth = 0;
    setup_render();
}

void setup_init(void) {
    // Portlar üzerinden donanımsal Mode 13h Grafik modunu açıyoruz
    outb(0x3C2, 0x63);
    uint8_t vga_regs[] = {
        0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
        0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x9C, 0x0E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3, 0xFF
    };
    for (uint8_t i = 0; i < 24; i++) {
        outb(0x3D4, i);
        outb(0x3D5, vga_regs[i]);
    }

    setup_init_palette();
    setup_render();
}

void setup_handle_input(uint8_t scancode) {
    if (scancode == 0x1C) { // ENTER
        trigger_next_stage();
    }
}
