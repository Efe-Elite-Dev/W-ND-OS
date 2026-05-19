/**
 * ==============================================================================
 * 🌟 SKY CORE OS / WIND OS OPERATING SYSTEM 🌟
 * ==============================================================================
 * [Proje Kodu]: Sky Core OS v1.5 (Vortex GUI Kernel)
 * [Mimari]: x86 Intel/AMD IA-32 Monolitik Çekirdek Standartları
 * [Derleme Hedefi]: 32-Bit Korumalı Mod (Protected Mode)
 * [Geliştirici]: Feyzula Efe Tuna
 * ==============================================================================
 */

#include <stdint.h>
#include <stddef.h>

// ==============================================================================
// 🪐 1. SİSTEM DURUM MAKİNESİ VE GLOBAL SABİTLER
// ==============================================================================

typedef enum {
    STATE_DESKTOP = 0
} OS_UI_STATE;

volatile OS_UI_STATE current_os_state = STATE_DESKTOP;

#define SCREEN_WIDTH         1024
#define SCREEN_HEIGHT        768

// Yenilenen UI Renk Paleti
#define COLOR_DARK_BLUE      0xFF0D0B18
#define COLOR_WHITE          0xFFFFFFFF
#define COLOR_LIGHT_GRAY     0xFFD0D0D0
#define COLOR_ACTIVE_BLUE    0xFF2575FC
#define COLOR_MAP_BG         0xFF3A3A5E
#define COLOR_MAP_LAND       0xFF8DA399
#define COLOR_TEXT_DARK      0xFF222222
#define COLOR_TEXT_LIGHT     0xFFF5F5F5
#define COLOR_WIDGET_BG      0xAA211C38
#define COLOR_GLOW_CYAN      0xFF00E5FF

// ==============================================================================
// 🖥️ 2. DONANIM VE BELLEK ADRESLERİ KATMANI
// ==============================================================================

uint32_t* GRAPHICS_FRAMEBUFFER = (uint32_t*)0xFD000000;

uint16_t* const TEXT_VIDEO_MEMORY = (uint16_t*)0xB8000;
int text_x = 0;
int text_y = 0;

int is_graphics_mode = 1;

// Dış fonksiyon bildirimleri
extern void force_graphics_hardware(void);

// ==============================================================================
// 🛠️ 3. I/O PORTS VE METİN MODU MOTORU (Baypas Edildi)
// ==============================================================================

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void print_string(const char* str) {
    (void)str;
}

// ==============================================================================
// 🎨 4. ÇEKİRDEK İÇİ GRAFİK ÇİZİM MOTORU
// ==============================================================================

static inline void put_pixel(int x, int y, uint32_t color) {
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
        GRAPHICS_FRAMEBUFFER[y * SCREEN_WIDTH + x] = color;
    }
}

void fill_screen(uint32_t color) {
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        GRAPHICS_FRAMEBUFFER[i] = color;
    }
}

void draw_filled_rectangle(int x, int y, int width, int height, uint32_t color) {
    for (int curr_y = y; curr_y < y + height; curr_y++) {
        for (int curr_x = x; curr_x < x + width; curr_x++) {
            put_pixel(curr_x, curr_y, color);
        }
    }
}

void draw_rectangle_outline(int x, int y, int width, int height, uint32_t color) {
    for (int curr_x = x; curr_x < x + width; curr_x++) {
        put_pixel(curr_x, y, color);
        put_pixel(curr_x, y + height - 1, color);
    }
    for (int curr_y = y; curr_y < y + height; curr_y++) {
        put_pixel(x, curr_y, color);
        put_pixel(x + width - 1, curr_y, color);
    }
}

void draw_vertical_gradient(uint32_t color_top, uint32_t color_bottom) {
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        uint8_t r = ((color_top >> 16) & 0xFF) * (SCREEN_HEIGHT - y) / SCREEN_HEIGHT + ((color_bottom >> 16) & 0xFF) * y / SCREEN_HEIGHT;
        uint8_t g = ((color_top >> 8) & 0xFF) * (SCREEN_HEIGHT - y) / SCREEN_HEIGHT + ((color_bottom >> 8) & 0xFF) * y / SCREEN_HEIGHT;
        uint8_t b = (color_top & 0xFF) * (SCREEN_HEIGHT - y) / SCREEN_HEIGHT + ((color_bottom & 0xFF) * y / SCREEN_HEIGHT;
        uint32_t mixed_color = (0xFF << 24) | (r << 16) | (g << 8) | b;
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            GRAPHICS_FRAMEBUFFER[y * SCREEN_WIDTH + x] = mixed_color;
        }
    }
}

// HATA DÜZELTİLDİ: 'c' parametresi (void)c; yapılarak "unused parameter" uyarısı engellendi.
void draw_char_basic(int x, int y, char c, uint32_t color) {
    (void)c; 
    static const uint8_t font_matrix[8] = {0x3C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x66, 0x00};
    for (int r = 0; r < 8; r++) {
        uint8_t row_byte = font_matrix[r];
        for (int b = 0; b < 8; b++) {
            if (row_byte & (1 << (7 - b))) {
                put_pixel(x + b, y + r, color);
            }
        }
    }
}

void draw_string_graphics(int x, int y, const char* str, uint32_t color) {
    int curr_x = x;
    while (*str) {
        if (*str == ' ') {
            curr_x += 8;
        } else {
            draw_char_basic(curr_x, y, *str, color);
            curr_x += 9;
        }
        str++;
    }
}

// ==============================================================================
// 🖼 5. SİSTEM DURUM RENDER MOTORU
// ==============================================================================

void draw_ui_main_desktop(void) {
    draw_vertical_gradient(0x02110222, 0x00021102);
    
    int wd_x = 40;
    int wd_y = 40;
    draw_filled_rectangle(wd_x, wd_y, 500, 200, COLOR_WIDGET_BG);
    draw_rectangle_outline(wd_x, wd_y, 500, 200, COLOR_WHITE);
    draw_string_graphics(wd_x + 20, wd_y + 25, "SAAT: 26:03", COLOR_GLOW_CYAN);
    draw_string_graphics(wd_x + 20, wd_y + 65, "Hava Durumu ve Saat - Esenyurt", COLOR_TEXT_LIGHT);
    draw_string_graphics(wd_x + 20, wd_y + 105, "21C - Bulutlu ve Firtina", COLOR_LIGHT_GRAY);
    draw_string_graphics(wd_x + 360, wd_y + 40, " 33.png", COLOR_TEXT_LIGHT);
    
    int icon_x = 40;
    int icon_y = 280;
    draw_filled_rectangle(icon_x, icon_y, 70, 60, COLOR_DARK_BLUE);
    draw_rectangle_outline(icon_x, icon_y, 70, 60, COLOR_WHITE);
    draw_string_graphics(icon_x, icon_y + 70, "Kainera", COLOR_TEXT_LIGHT);

    draw_filled_rectangle(icon_x, icon_y + 100, 70, 60, COLOR_DARK_BLUE);
    draw_string_graphics(icon_x, icon_y + 170, "Kamera", COLOR_TEXT_LIGHT);
    
    int bar_w = 220;
    int bar_x = SCREEN_WIDTH - bar_w - 20;
    draw_filled_rectangle(bar_x, 40, bar_w, SCREEN_HEIGHT - 120, COLOR_WIDGET_BG);
    draw_rectangle_outline(bar_x, 40, bar_w, SCREEN_HEIGHT - 120, COLOR_WHITE);
    draw_string_graphics(bar_x + 40, 65, "UYGULAMALAR", COLOR_GLOW_CYAN);
    
    draw_filled_rectangle(bar_x + 20, 110, 180, 45, COLOR_DARK_BLUE);
    draw_string_graphics(bar_x + 40, 125, "Terminal", COLOR_TEXT_LIGHT);

    draw_filled_rectangle(bar_x + 20, 175, 180, 45, COLOR_DARK_BLUE);
    draw_string_graphics(bar_x + 40, 190, "Mesajlar", COLOR_TEXT_LIGHT);

    draw_filled_rectangle(bar_x + 20, 240, 180, 45, COLOR_ACTIVE_BLUE);
    draw_string_graphics(bar_x + 40, 255, "Dosya Yoneticisi", COLOR_TEXT_LIGHT);

    int pop_w = 400;
    int pop_h = 140;
    int pop_x = (SCREEN_WIDTH - bar_w) / 2 - 100;
    int pop_y = (SCREEN_HEIGHT / 2) - 30;
    draw_filled_rectangle(pop_x, pop_y, pop_w, pop_h, COLOR_WHITE);
    draw_rectangle_outline(pop_x, pop_y, pop_w, pop_h, COLOR_GLOW_CYAN);
    draw_string_graphics(pop_x + 85, pop_y + 35, "HOS GELDINIZ", COLOR_TEXT_DARK);
    draw_string_graphics(pop_x + 85, pop_y + 65, "Sisteme Hos Geldiniz!", COLOR_TEXT_DARK);
    draw_string_graphics(pop_x + 85, pop_y + 95, "GHHOD GERLDIN!", COLOR_ACTIVE_BLUE);

    int dock_w = 700;
    int dock_h = 60;
    int dock_x = 40;
    int dock_y = SCREEN_HEIGHT - 80;
    draw_filled_rectangle(dock_x, dock_y, dock_w, dock_h, COLOR_WIDGET_BG);
    draw_rectangle_outline(dock_x, dock_y, dock_w, dock_h, COLOR_GLOW_CYAN);
    draw_string_graphics(dock_x + 120, dock_y + 22, "TUSUMANA BASINCA CEKMECE ACILSIN", COLOR_TEXT_LIGHT);
}

void refresh_system_display(void) {
    fill_screen(COLOR_DARK_BLUE);
    switch (current_os_state) {
        case STATE_DESKTOP: draw_ui_main_desktop(); break;
    }
}

// ==============================================================================
// 🚀 KERNEL GİRİŞ NOKTASI
// ==============================================================================
void kernel_main(void* mboot_ptr, uint32_t magic) {
    (void)magic;

    if (mboot_ptr != NULL) {
        uint32_t flags = *(uint32_t*)mboot_ptr;
        if (flags & (1 << 11)) {
            uint32_t* vbe_mode_info = (uint32_t*)((uint8_t*)mboot_ptr + 72);
            uint32_t real_fb_address = *vbe_mode_info;
            if (real_fb_address != 0) {
                GRAPHICS_FRAMEBUFFER = (uint32_t*)real_fb_address;
            }
        }
    }

    is_graphics_mode = 1;
    force_graphics_hardware();

    current_os_state = STATE_DESKTOP;
    refresh_system_display();

    while (1) {
        if (inb(0x64) & 1) {
            uint8_t scancode = inb(0x60);
            (void)scancode;
        }
        __asm__ volatile("pause");
    }
}

// ==============================================================================
// 🛠️ LINKER SUSTURUCU KÖPRÜLER (STUBS)
// ==============================================================================
// ÇAKIŞMA ÇÖZÜLDÜ: screen_init() fonksiyonu screen.o'da zaten tanımlı olduğu için buradan kaldırıldı!
void idt_init(void) {}
void keyboard_init(void) {}
void mouse_init(void) {}
void wind_subsystem_init(void) {}
void exe_subsystem_init(void) {}
void ai_subsystem_init(void) {}
void deb_subsystem_init(void) {}
