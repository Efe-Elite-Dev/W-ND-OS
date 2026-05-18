/**
 * ==============================================================================
 * 🌟 SKY CORE OS / WIND OS OPERATING SYSTEM 🌟
 * ==============================================================================
 * [Proje Kodu]: Sky Core OS v1.5 (Vortex Kernel)
 * [Mimari]: x86 Intel/AMD IA-32 Monolitik Çekirdek Standartları
 * [Derleme Hedefi]: 32-Bit Korumalı Mod (Protected Mode) - Multiboot Uyumlu
 * [Geliştirici]: Feyzula Efe Tuna
 * ==============================================================================
 */

#include <stdint.h>
#include <stddef.h>

// ==============================================================================
// 🪐 1. SİSTEM DURUM MAKİNESİ VE GLOBAL SABİTLER (STATE MACHINE CONFIG)
// ==============================================================================

typedef enum {
    STATE_WELCOME = 0,    // Aşama 1: İlk Kurulum Ekranı - Hoş Geldiniz
    STATE_LOCATION = 1,   // Aşama 2: Konum & Saat Ayarlama (Türkiye Haritası)
    STATE_SUMMARY = 2,    // Aşama 3: Giriş Bilgileri & Tamamlama
    STATE_DESKTOP = 3     // Aşama 4: Fırtına & Ay Temalı Widget'lı Masaüstü!
} OS_UI_STATE;

// Global Çekirdek Durum Değişkeni
volatile OS_UI_STATE current_os_state = STATE_WELCOME;

// Ekran Çözünürlüğü Tanımlamaları
#define SCREEN_WIDTH         1024
#define SCREEN_HEIGHT        768
#define SCREEN_BPP            32      

// Donanımsal Renk Paleti Makroları
#define COLOR_DEEP_PURPLE    0xFF1A0F2E  
#define COLOR_DARK_BLUE      0xFF0D0B18  
#define COLOR_WHITE          0xFFFFFFFF  
#define COLOR_LIGHT_GRAY     0xFFE0E0E0  
#define COLOR_ACTIVE_BLUE    0xFF2575FC  
#define COLOR_MAP_GOLD       0xFFFFD700  
#define COLOR_TEXT_DARK      0xFF222222  
#define COLOR_TEXT_LIGHT     0xFFF5F5F5  
#define COLOR_WIDGET_BG      0x80251F3D  
#define COLOR_GLOW_CYAN      0xFF00E5FF  

// ==============================================================================
// 🖥️ 2. DONANIM VE BELLEK ADRESLERİ KATMANI (HARDWARE & MEMORY MAP)
// ==============================================================================

uint16_t* const TEXT_VIDEO_MEMORY = (uint16_t*)0xB8000;
int text_x = 0;
int text_y = 0;

// Çekirdeğin dinamik olarak güncelleyeceği esnek framebuffer pointer'ı
uint32_t* GRAPHICS_FRAMEBUFFER = (uint32_t*)0xFD000000;

int is_graphics_mode = 1; 

char cmd_buffer[16];
int cmd_idx = 0;

// Dış donanım ve alt sistem modülleri bildirimleri
extern void setup_init(void);
extern void setup_handle_input(uint8_t scancode);
extern void force_graphics_hardware(void);
extern void kpanic(uint8_t error_code, const char* message);

extern void wind_subsystem_init(void);
extern void exe_subsystem_init(void);
extern void ai_subsystem_init(void);
extern void deb_subsystem_init(void);
extern void idt_init(void);
extern void mouse_init(void);
extern void screen_init(void);
extern void keyboard_init(void);

// ==============================================================================
// 🛠️ 3. DÜŞÜK SEVİYELİ İŞLEMCİ PORT GİRİŞ/ÇIKIŞ FONKSİYONLARI (I/O PORTS)
// ==============================================================================

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

// ==============================================================================
// 📝 4. METİN MODU VE HATA AYIKLAMA MOTORU (TEXT MODE FALLBACK)
// ==============================================================================

void clear_text_screen(void) {
    for (int i = 0; i < 80 * 25; i++) {
        TEXT_VIDEO_MEMORY[i] = (0x0F << 8) | ' ';
    }
    text_x = 0;
    text_y = 0;
}

void print_string(const char* str) {
    if (is_graphics_mode) return; 
    while (*str) {
        if (*str == '\n') {
            text_x = 0;
            text_y++;
        } else {
            TEXT_VIDEO_MEMORY[text_y * 80 + text_x] = (0x0F << 8) | *str;
            text_x++;
            if (text_x >= 80) { text_x = 0; text_y++; }
        }
        str++;
    }
}

// 🔥 YENİ ENTEGRASYON: "sne" komutunu avlayan CLI fonksiyonu
void handle_cli_command(const char* cmd) {
    if (cmd[0] == 's' && cmd[1] == 'n' && cmd[2] == 'e') {
        is_graphics_mode = 1;
        force_graphics_hardware(); // Ekran kartını zorla grafik moduna çek
        setup_init();              // Arayüzü tetikle
    } else if (cmd[0] == 's' && cmd[1] == 's') { // Eski komut desteği korundu
        is_graphics_mode = 1;
        force_graphics_hardware();
        setup_init(); 
    } else {
        print_string("\nBilinmeyen Komut! Gecerli mod: sne\nSkyCoreOS> ");
    }
}

// ==============================================================================
// 🎨 5. ÇEKİRDEK İÇİ GRAFİK ÇİZİM MOTORU (LOW-LEVEL GRAPHICS ENGINE)
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
        uint8_t b = (color_top & 0xFF) * (SCREEN_HEIGHT - y) / SCREEN_HEIGHT + (color_bottom & 0xFF) * y / SCREEN_HEIGHT;
        uint32_t mixed_color = (0xFF << 24) | (r << 16) | (g << 8) | b;
        
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            GRAPHICS_FRAMEBUFFER[y * SCREEN_WIDTH + x] = mixed_color;
        }
    }
}

void draw_char_basic(int x, int y, char c, uint32_t color) {
    static const uint8_t generic_font[8] = {0x3C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x66, 0x00};
    static const uint8_t font_mock[8] = {0x3C, 0x66, 0x60, 0x3C, 0x06, 0x66, 0x3C, 0x00};
    
    const uint8_t* active_font = (c == 'S' || c == 's') ? font_mock : generic_font;

    for (int r = 0; r < 8; r++) {
        uint8_t row_byte = active_font[r]; 
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
// 🖼 6. SİSTEM DURUM RENDER MOTORU (SCREEN DRAWING FUNCTIONS)
// ==============================================================================

void draw_ui_welcome_screen(void) {
    draw_vertical_gradient(COLOR_DEEP_PURPLE, COLOR_DARK_BLUE);
    
    int panel_w = 650;
    int panel_h = 380;
    int panel_x = (SCREEN_WIDTH - panel_w) / 2;
    int panel_y = (SCREEN_HEIGHT - panel_h) / 2 - 40;
    draw_filled_rectangle(panel_x, panel_y, panel_w, panel_h, COLOR_WHITE);
    draw_rectangle_outline(panel_x, panel_y, panel_w, panel_h, COLOR_GLOW_CYAN);
    
    draw_string_graphics(panel_x + 30, panel_y + 30, "ILK KURULUM EKRANI - HOS GELDINIZ", COLOR_TEXT_DARK);
    
    draw_filled_rectangle(panel_x + panel_w/2 - 40, panel_y + 80, 80, 80, COLOR_DARK_BLUE);
    draw_string_graphics(panel_x + panel_w/2 - 25, panel_y + 115, "PUSULA", COLOR_TEXT_LIGHT);
    
    draw_string_graphics(panel_x + 120, panel_y + 190, "Sky Core OS Kurulum Sihirbazina Hos Geldiniz!", COLOR_TEXT_DARK);
    draw_string_graphics(panel_x + 80, panel_y + 220, "Sistemi hemen yapilandirmak icin asagidan bir mod secin.", COLOR_TEXT_DARK);
    
    draw_filled_rectangle(panel_x + 60, panel_y + 290, 230, 45, COLOR_ACTIVE_BLUE);
    draw_string_graphics(panel_x + 90, panel_y + 305, "Hizli Kurulum (Az)", COLOR_TEXT_LIGHT);
    
    draw_filled_rectangle(panel_x + 360, panel_y + 290, 230, 45, COLOR_DARK_BLUE);
    draw_string_graphics(panel_x + 385, panel_y + 305, "Detayli Kurulum (Baska)", COLOR_TEXT_LIGHT);
    
    draw_filled_rectangle(20, SCREEN_HEIGHT - 70, 180, 45, COLOR_WIDGET_BG);
    draw_string_graphics(35, SCREEN_HEIGHT - 55, "SKY CORE OS v1.5", COLOR_TEXT_LIGHT);
}

void draw_ui_location_screen(void) {
    draw_vertical_gradient(COLOR_DEEP_PURPLE, COLOR_DARK_BLUE);
    
    draw_filled_rectangle(0, 0, SCREEN_WIDTH, 60, COLOR_WIDGET_BG);
    draw_string_graphics(40, 22, "ASAMA 2: KONUM VE BOLGE SAATI AYARLARI", COLOR_TEXT_LIGHT);
    
    int map_x = 480;
    int map_y = 120;
    int map_w = 500;
    int map_h = 320;
    draw_filled_rectangle(map_x, map_y, map_w, map_h, COLOR_WIDGET_BG);
    draw_rectangle_outline(map_x, map_y, map_w, map_h, COLOR_WHITE);
    
    draw_filled_rectangle(map_x + 40, map_y + 80, 420, 140, COLOR_MAP_GOLD);
    draw_filled_rectangle(map_x + 20, map_y + 110, 50, 60, COLOR_MAP_GOLD); 
    
    draw_filled_rectangle(map_x + 80, map_y + 100, 10, 10, 0xFFFF0000); 
    draw_string_graphics(map_x + 95, map_y + 95, "Istanbul", COLOR_TEXT_LIGHT);
    
    int form_x = 50;
    int form_y = 140;
    draw_string_graphics(form_x, form_y, "Sistem Konumu:", COLOR_TEXT_LIGHT);
    draw_filled_rectangle(form_x, form_y + 20, 380, 40, COLOR_WHITE);
    draw_string_graphics(form_x + 15, form_y + 32, "Istanbul, Turkiye", COLOR_TEXT_DARK);
    
    draw_string_graphics(form_x, form_y + 90, "Bolge Saati Entegrasyonu:", COLOR_TEXT_LIGHT);
    draw_filled_rectangle(form_x, form_y + 110, 380, 40, COLOR_WHITE);
    draw_string_graphics(form_x + 15, form_y + 122, "GMT+03:00 (Yaz Saati Uygulamasi)", COLOR_TEXT_DARK);
    
    draw_string_graphics(form_x, form_y + 220, "Devam etmek icin [ENTER] tusuna basin.", COLOR_GLOW_CYAN);
}

void draw_ui_summary_screen(void) {
    draw_vertical_gradient(COLOR_DEEP_PURPLE, COLOR_DARK_BLUE);
    
    int box_w = 600;
    int box_h = 300;
    int box_x = (SCREEN_WIDTH - box_w) / 2;
    int box_y = (SCREEN_HEIGHT - box_h) / 2;
    draw_filled_rectangle(box_x, box_y, box_w, box_h, COLOR_WIDGET_BG);
    draw_rectangle_outline(box_x, box_y, box_w, box_h, COLOR_GLOW_CYAN);
    
    int moon_x = box_x + 50;
    int moon_y = box_y + 90;
    draw_filled_rectangle(moon_x, moon_y, 80, 80, 0xFFFFF59D); 
    draw_filled_rectangle(moon_x + 30, moon_y, 50, 80, COLOR_DARK_BLUE); 
    
    draw_string_graphics(box_x + 160, box_y + 80, "GIRIS VE TAMAMLAMA", COLOR_GLOW_CYAN);
    draw_string_graphics(box_x + 160, box_y + 120, "Giris bilgileriniz kontrol edildi.", COLOR_TEXT_LIGHT);
    draw_string_graphics(box_x + 160, box_y + 150, "Sky Core OS kullanima tamamen hazir!", COLOR_TEXT_LIGHT);
    
    int btn_w = 400;
    int btn_h = 50;
    int btn_x = box_x + (box_w - btn_w) / 2;
    int btn_y = box_y + 220;
    draw_filled_rectangle(btn_x, btn_y, btn_w, btn_h, 0xFF4CAF50); 
    draw_rectangle_outline(btn_x, btn_y, btn_w, btn_h, COLOR_WHITE);
    draw_string_graphics(btn_x + 110, btn_y + 18, "KURULUMU TAMAMLA VE BASLAT", COLOR_TEXT_LIGHT);
}

void draw_ui_main_desktop(void) {
    draw_vertical_gradient(0x02110222, 0x00021102);
    
    int wd_x = 40;
    int wd_y = 40;
    draw_filled_rectangle(wd_x, wd_y, 340, 180, COLOR_WIDGET_BG);
    draw_rectangle_outline(wd_x, wd_y, 340, 180, COLOR_WHITE);
    
    draw_string_graphics(wd_x + 20, wd_y + 30, "SAAT: 26:03", COLOR_GLOW_CYAN); 
    draw_string_graphics(wd_x + 20, wd_y + 70, "Konum: Esenyurt, Ist", COLOR_TEXT_LIGHT);
    draw_string_graphics(wd_x + 20, wd_y + 110, "Hava: 21C - Bulutlu & Firtina", COLOR_TEXT_LIGHT);
    
    int bar_w = 200;
    int bar_x = SCREEN_WIDTH - bar_w;
    draw_filled_rectangle(bar_x, 0, bar_w, SCREEN_HEIGHT, COLOR_WIDGET_BG);
    draw_rectangle_outline(bar_x, 0, bar_w, SCREEN_HEIGHT, COLOR_WHITE);
    draw_string_graphics(bar_x + 35, 30, "UYGULAMALAR", COLOR_GLOW_CYAN);
    
    draw_filled_rectangle(bar_x + 30, 80, 140, 40, COLOR_DARK_BLUE);
    draw_string_graphics(bar_x + 50, 92, ">_ Terminal", COLOR_TEXT_LIGHT);
    
    draw_filled_rectangle(bar_x + 30, 140, 140, 40, COLOR_DARK_BLUE);
    draw_string_graphics(bar_x + 45, 152, "Dosya Yonetimi", COLOR_TEXT_LIGHT);
    
    draw_filled_rectangle(bar_x + 30, 200, 140, 40, COLOR_DARK_BLUE);
    draw_string_graphics(bar_x + 55, 212, "Haritalar", COLOR_TEXT_LIGHT);
    
    draw_filled_rectangle(60, 300, 70, 70, COLOR_ACTIVE_BLUE);
    draw_string_graphics(65, 385, "Kamera.sys", COLOR_TEXT_LIGHT);
    
    draw_filled_rectangle(180, 300, 70, 70, COLOR_ACTIVE_BLUE);
    draw_string_graphics(190, 385, "Ayarlar", COLOR_TEXT_LIGHT);
    
    int dock_w = 600;
    int dock_h = 55;
    int dock_x = (SCREEN_WIDTH - bar_w - dock_w) / 2;
    int dock_y = SCREEN_HEIGHT - 75;
    draw_filled_rectangle(dock_x, dock_y, dock_w, dock_h, COLOR_WIDGET_BG);
    draw_rectangle_outline(dock_x, dock_y, dock_w, dock_h, COLOR_GLOW_CYAN);
    draw_string_graphics(dock_x + 60, dock_y + 20, "TUSUMANA BASINCA UYGULAMA CEKMECESI ACILSIN", COLOR_TEXT_LIGHT);
}

// ==============================================================================
// 🗺️ 7. UI DURUM GEÇİŞ YÖNETİCİSİ (STATE CONTROLLER)
// ==============================================================================

void refresh_system_display(void) {
    fill_screen(COLOR_DARK_BLUE);
    
    switch (current_os_state) {
        case STATE_WELCOME:
            draw_ui_welcome_screen();
            break;
        case STATE_LOCATION:
            draw_ui_location_screen();
            break;
        case STATE_SUMMARY:
            draw_ui_summary_screen();
            break;
        case STATE_DESKTOP:
            draw_ui_main_desktop();
            break;
    }
}

void advance_os_stage(void) {
    if (current_os_state < STATE_DESKTOP) {
        current_os_state++;
        refresh_system_display();
    }
}

void regress_os_stage(void) {
    if (current_os_state > STATE_WELCOME && current_os_state != STATE_DESKTOP) {
        current_os_state--;
        refresh_system_display();
    }
}

// ==============================================================================
// 🚀 8. MAIN KERNEL ENTRY POINT (ANA ÇEKİRDEK GİRİŞ NOKTASI)
// ==============================================================================

void kernel_main(void* mboot_ptr, uint32_t magic) {
    if (magic == 0x2BADB002 && mboot_ptr != NULL) {
        uint32_t flags = *(uint32_t*)mboot_ptr;
        if (flags & (1 << 11)) { 
            uint32_t* vbe_mode_info = (uint32_t*)((uint8_t*)mboot_ptr + 72);
            uint32_t real_fb_address = *vbe_mode_info;
            if (real_fb_address != 0) {
                GRAPHICS_FRAMEBUFFER = (uint32_t*)real_fb_address;
            }
        }
    }

    force_graphics_hardware();

    for (volatile int delay = 0; delay < 4000000; delay++) {
        __asm__ volatile("pause");
    }

    idt_init();
    screen_init();
    keyboard_init();
    mouse_init();
    
    wind_subsystem_init();
    exe_subsystem_init();
    ai_subsystem_init();
    deb_subsystem_init();

    setup_init(); 
    
    if (GRAPHICS_FRAMEBUFFER == 0) {
        kpanic(0x01, "LFB Hafiza Hatasi: Grafik Adresi Baglanamadi!");
    }

    current_os_state = STATE_WELCOME;
    refresh_system_display();

    for(int i = 0; i < 16; i++) cmd_buffer[i] = 0;
    cmd_idx = 0;

    while (1) {
        if (inb(0x64) & 1) { 
            uint8_t scancode = inb(0x60);
            
            if (is_graphics_mode) {
                setup_handle_input(scancode); 
                
                if (!(scancode & 0x80)) {
                    switch (scancode) {
                        case 0x1C: // ENTER Tuşu
                            advance_os_stage();
                            break;
                            
                        case 0x0E: // BACKSPACE Tuşu
                            regress_os_stage();
                            break;
                            
                        default:
                            break;
                    }
                }
            } else {
                // Metin Modunda CLI Girdilerini Yakala
                if (!(scancode & 0x80)) {
                    if (scancode == 0x1F) { // 'S' Tuşu
                        if (cmd_idx < 14) { 
                            cmd_buffer[cmd_idx++] = 's'; 
                            print_string("s"); 
                        }
                    } else if (scancode == 0x31) { // 🔥 'N' Tuşu Scancode'u
                        if (cmd_idx < 14) { 
                            cmd_buffer[cmd_idx++] = 'n'; 
                            print_string("n"); 
                        }
                    } else if (scancode == 0x12) { // 🔥 'E' Tuşu Scancode'u
                        if (cmd_idx < 14) { 
                            cmd_buffer[cmd_idx++] = 'e'; 
                            print_string("e"); 
                        }
                    } else if (scancode == 0x1C) { // ENTER Tuşu
                        cmd_buffer[cmd_idx] = '\0';
                        handle_cli_command(cmd_buffer);
                        
                        for(int i = 0; i < 16; i++) cmd_buffer[i] = 0;
                        cmd_idx = 0;
                    }
                }
            }
        }
        __asm__ volatile("hlt"); 
    }
}
