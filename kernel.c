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

volatile OS_UI_STATE current_os_state = STATE_WELCOME;

#define SCREEN_WIDTH         1024
#define SCREEN_HEIGHT        768

// Yenilenen UI Renk Paleti (Ekran Görüntülerine Birebir Uyumlu)
#define COLOR_DEEP_PURPLE    0xFF1A0F2E  // Ana Arka Plan Koyu Mor
#define COLOR_DARK_BLUE      0xFF0D0B18  // Alt/Üst Bar ve Gölgeler
#define COLOR_WHITE          0xFFFFFFFF  
#define COLOR_LIGHT_GRAY     0xFFD0D0D0  
#define COLOR_ACTIVE_BLUE    0xFF2575FC  // Buton Aktif Mavisi
#define COLOR_MAP_BG         0xFF3A3A5E  // Harita Paneli Arka Planı
#define COLOR_MAP_LAND       0xFF8DA399  // Türkiye Haritası Kara Rengi
#define COLOR_TEXT_DARK      0xFF222222  
#define COLOR_TEXT_LIGHT     0xFFF5F5F5  
#define COLOR_WIDGET_BG      0xAA211C38  // Transparan Widget Grisi/Moru
#define COLOR_GLOW_CYAN      0xFF00E5FF  

// ==============================================================================
// 🖥️ 2. DONANIM VE BELLEK ADRESLERİ KATMANI (HARDWARE & MEMORY MAP)
// ==============================================================================

uint16_t* const TEXT_VIDEO_MEMORY = (uint16_t*)0xB8000;
int text_x = 0;
int text_y = 0;

uint32_t* GRAPHICS_FRAMEBUFFER = (uint32_t*)0xFD000000;
int is_graphics_mode = 1; 

char cmd_buffer[16];
int cmd_idx = 0;

// Dış fonksiyon bildirimleri
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
// 🛠️ 3. I/O PORTS VE METİN MODU MOTORU
// ==============================================================================

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
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

// ==============================================================================
// 🎨 4. ÇEKİRDEK İÇİ GRAFİK ÇİZİM MOTORU (GRAPHICS ENGINE)
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

// Sembolik Türkiye Haritası Çizim Yardımcısı (Görsel 2 ve 3)
void draw_turkey_map_vector(int x, int y) {
    draw_filled_rectangle(x, y, 400, 180, COLOR_MAP_BG);
    draw_rectangle_outline(x, y, 400, 180, COLOR_WHITE);
    // Anadolu Ana Gövde
    draw_filled_rectangle(x + 40, y + 40, 320, 100, COLOR_LAND_GRAY);
    // Trakya
    draw_filled_rectangle(x + 10, y + 20, 50, 40, COLOR_LAND_GRAY);
    // İstanbul Pin Noktası
    draw_filled_rectangle(x + 55, y + 45, 8, 8, 0xFFFF0000); 
    draw_string_graphics(x + 70, y + 45, "Istanbul", COLOR_TEXT_LIGHT);
}

// ==============================================================================
// 🖼 5. SİSTEM DURUM RENDER MOTORU (SCREEN DRAWING FUNCTIONS)
// ==============================================================================

// EKRAN 1: İlk Kurulum Ekranı - Hoş Geldiniz
void draw_ui_welcome_screen(void) {
    draw_vertical_gradient(COLOR_DEEP_PURPLE, COLOR_DARK_BLUE);
    
    // Üst Bar
    draw_filled_rectangle(0, 0, SCREEN_WIDTH, 40, COLOR_WIDGET_BG);
    draw_string_graphics(20, 15, "ILK KURULUM EKRANI - HOS GELDINIZ", COLOR_TEXT_LIGHT);
    
    // Orta Beyaz Panel
    int panel_w = 600;
    int panel_h = 350;
    int panel_x = (SCREEN_WIDTH - panel_w) / 2;
    int panel_y = (SCREEN_HEIGHT - panel_h) / 2;
    draw_filled_rectangle(panel_x, panel_y, panel_w, panel_h, COLOR_WHITE);
    draw_rectangle_outline(panel_x, panel_y, panel_w, panel_h, COLOR_DARK_BLUE);
    
    // Pusula Logosu Alanı
    draw_filled_rectangle(panel_x + panel_w/2 - 30, panel_y + 40, 60, 60, COLOR_DARK_BLUE);
    draw_string_graphics(panel_x + panel_w/2 - 25, panel_y + 65, "PUSULA", COLOR_TEXT_LIGHT);
    
    draw_string_graphics(panel_x + 160, panel_y + 140, "ILK KURULUM EKRANI", COLOR_TEXT_DARK);
    draw_string_graphics(panel_x + 175, panel_y + 180, "Sisteme Hos Geldiniz!", COLOR_TEXT_DARK);
    
    // Butonlar
    int btn_y = panel_y + 240;
    draw_filled_rectangle(panel_x + 60, btn_y, 200, 45, COLOR_DARK_BLUE);
    draw_string_graphics(panel_x + 85, btn_y + 15, "Hizli Kurulum (Az)", COLOR_TEXT_LIGHT);
    
    draw_filled_rectangle(panel_x + 340, btn_y, 220, 45, COLOR_DARK_BLUE);
    draw_string_graphics(panel_x + 350, btn_y + 15, "Detayli Kurulum (Baska)", COLOR_TEXT_LIGHT);
    
    // Sol Alt Köşe Etiketi
    draw_filled_rectangle(15, SCREEN_HEIGHT - 65, 140, 40, COLOR_WIDGET_BG);
    draw_string_graphics(25, SCREEN_HEIGHT - 50, "SKY CORE OS v1.5", COLOR_TEXT_LIGHT);
}

// EKRAN 2: Konum & Saat Ayarlama
void draw_ui_location_screen(void) {
    draw_vertical_gradient(COLOR_DEEP_PURPLE, COLOR_DARK_BLUE);
    
    // Üst Başlık
    draw_filled_rectangle(0, 0, SCREEN_WIDTH, 40, COLOR_WIDGET_BG);
    draw_string_graphics(20, 15, "KONUM & SAAT AYARLAMA", COLOR_TEXT_LIGHT);
    
    // Sol Bilgi Paneli
    int panel_x = 50;
    int panel_y = 100;
    draw_filled_rectangle(panel_x, panel_y, 350, 400, COLOR_WIDGET_BG);
    draw_string_graphics(panel_x + 20, panel_y + 30, "SKY CORE OS v1.5'i", COLOR_TEXT_LIGHT);
    draw_string_graphics(panel_x + 20, panel_y + 60, "Sectiginiz Icin", COLOR_TEXT_LIGHT);
    draw_string_graphics(panel_x + 20, panel_y + 90, "Tesekkurler!", COLOR_TEXT_LIGHT);
    
    draw_string_graphics(panel_x + 20, panel_y + 160, "Ag Baglantisi:", COLOR_TEXT_LIGHT);
    draw_string_graphics(panel_x + 20, panel_y + 190, "Lutfen Bir Aga Baglanin.", COLOR_LIGHT_GRAY);
    
    // WiFi Seçeneği ve Atla Butonu
    draw_filled_rectangle(panel_x + 20, panel_y + 240, 310, 45, COLOR_DARK_BLUE);
    draw_string_graphics(panel_x + 40, panel_y + 255, "WiFi A BAGLANA", COLOR_TEXT_LIGHT);
    
    draw_filled_rectangle(panel_x + 20, panel_y + 310, 310, 45, COLOR_DARK_BLUE);
    draw_string_graphics(panel_x + 150, panel_y + 325, "Atla", COLOR_TEXT_LIGHT);

    // Sağ Harita ve Form Alanı
    int map_x = 450;
    draw_turkey_map_vector(map_x, panel_y);
    
    int form_y = panel_y + 210;
    draw_string_graphics(map_x, form_y, "Konum: [Istanbul, Turkiye]", COLOR_TEXT_LIGHT);
    draw_string_graphics(map_x, form_y + 50, "Bolge Saati: [GMT+03:00]", COLOR_TEXT_LIGHT);
    
    // Ayarları Kaydet Butonu
    draw_filled_rectangle(map_x, form_y + 110, 400, 50, COLOR_DARK_BLUE);
    draw_rectangle_outline(map_x, form_y + 110, 400, 50, COLOR_WHITE);
    draw_string_graphics(map_x + 60, form_y + 130, "Konum ve Saat Ayarlarini Kaydet", COLOR_TEXT_LIGHT);
}

// EKRAN 3: Giriş & Tamamlama
void draw_ui_summary_screen(void) {
    draw_vertical_gradient(COLOR_DEEP_PURPLE, COLOR_DARK_BLUE);
    
    // Sol Bölüm (Harita)
    draw_string_graphics(50, 50, "KONUM & SAAT AYARLAMA", COLOR_TEXT_LIGHT);
    draw_turkey_map_vector(50, 100);
    draw_string_graphics(50, 300, "Konum: [Istanbul, Turkiye]  26:03", COLOR_TEXT_LIGHT);
    draw_string_graphics(50, 340, "Bolge Saati: [GMT+03:00]", COLOR_TEXT_LIGHT);
    
    // Sağ Bölüm (Giriş & Tamamlama)
    int box_x = 520;
    int box_y = 100;
    draw_filled_rectangle(box_x, box_y, 440, 450, COLOR_WIDGET_BG);
    draw_rectangle_outline(box_x, box_y, 440, 450, COLOR_WHITE);
    
    draw_string_graphics(box_x + 40, box_y + 40, "GIRIS & TAMAMLAMA", COLOR_GLOW_CYAN);
    draw_string_graphics(box_x + 40, box_y + 100, "Giris Bilgilerini Kontrol Edin.", COLOR_TEXT_LIGHT);
    
    // Hilal / Ay Logosu Alanı
    int moon_x = box_x + 180;
    int moon_y = box_y + 150;
    draw_filled_rectangle(moon_x, moon_y, 80, 80, COLOR_DARK_BLUE);
    draw_string_graphics(moon_x + 15, moon_y + 35, "33.png", COLOR_TEXT_LIGHT);
    
    draw_string_graphics(box_x + 40, box_y + 260, "Tesekkurler, Kullanima Hazir!", COLOR_TEXT_LIGHT);
    draw_string_graphics(box_x + 60, box_y + 310, "Masaustune Gitmek Icin HAZIR", COLOR_LIGHT_GRAY);
    
    // Başlat Butonu
    draw_filled_rectangle(box_x + 40, box_y + 360, 360, 50, COLOR_DARK_BLUE);
    draw_rectangle_outline(box_x + 40, box_y + 360, 360, 50, COLOR_WHITE);
    draw_string_graphics(box_x + 180, box_y + 380, "BASLAT", COLOR_TEXT_LIGHT);
}

// EKRAN 4: Gelişmiş Masaüstü (Son Ekran Görüntüsü)
void draw_ui_main_desktop(void) {
    draw_vertical_gradient(0x02110222, 0x00021102); // Koyu Fırtına Teması
    
    // 1. Üst Sol: Saat, Hava Durumu ve Widget Alanı
    int wd_x = 40;
    int wd_y = 40;
    draw_filled_rectangle(wd_x, wd_y, 500, 200, COLOR_WIDGET_BG);
    draw_rectangle_outline(wd_x, wd_y, 500, 200, COLOR_WHITE);
    draw_string_graphics(wd_x + 20, wd_y + 25, "SAAT: 26:03", COLOR_GLOW_CYAN);
    draw_string_graphics(wd_x + 20, wd_y + 65, "Hava Durumu ve Saat - Esenyurt", COLOR_TEXT_LIGHT);
    draw_string_graphics(wd_x + 20, wd_y + 105, "21C - Bulutlu ve Firtina", COLOR_LIGHT_GRAY);
    
    // 2. Sol Dikey İkonlar (Kamera, Kainera vb.)
    int icon_x = 40;
    int icon_y = 280;
    for(int i = 0; i < 3; i++) {
        draw_filled_rectangle(icon_x, icon_y + (i * 100), 70, 60, COLOR_DARK_BLUE);
        draw_rectangle_outline(icon_x, icon_y + (i * 100), 70, 60, COLOR_WHITE);
    }
    draw_string_graphics(icon_x, icon_y + 70, "Kainera", COLOR_TEXT_LIGHT);
    draw_string_graphics(icon_x, icon_y + 170, "Kamera", COLOR_TEXT_LIGHT);
    
    // 3. Sağ Dikey Panel: UYGULAMALAR ÇEKMECESİ
    int bar_w = 220;
    int bar_x = SCREEN_WIDTH - bar_w - 20;
    draw_filled_rectangle(bar_x, 40, bar_w, SCREEN_HEIGHT - 120, COLOR_WIDGET_BG);
    draw_rectangle_outline(bar_x, 40, bar_w, SCREEN_HEIGHT - 120, COLOR_WHITE);
    draw_string_graphics(bar_x + 40, 65, "UYGULAMALAR", COLOR_GLOW_CYAN);
    
    // Sağ Panel İçi Uygulama İkonları
    for(int i = 0; i < 4; i++) {
        int app_y = 110 + (i * 75);
        draw_filled_rectangle(bar_x + 20, app_y, 180, 45, COLOR_DARK_BLUE);
        if(i == 0) draw_string_graphics(bar_x + 40, app_y + 15, "Terminal", COLOR_TEXT_LIGHT);
        if(i == 1) draw_string_graphics(bar_x + 40, app_y + 15, "Mesajlar", COLOR_TEXT_LIGHT);
        if(i == 2) draw_string_graphics(bar_x + 40, app_y + 15, "Dosya Yoneticisi", COLOR_TEXT_LIGHT);
        if(i == 3) draw_string_graphics(bar_x + 40, app_y + 15, "Haritalar", COLOR_TEXT_LIGHT);
    }

    // 4. Orta Karşılama Popup Kutusu ("GHHOD GERLDİN!")
    int pop_w = 400;
    int pop_h = 140;
    int pop_x = (SCREEN_WIDTH - bar_w) / 2 - 100;
    int pop_y = (SCREEN_HEIGHT / 2) - 30;
    draw_filled_rectangle(pop_x, pop_y, pop_w, pop_h, COLOR_WHITE);
    draw_rectangle_outline(pop_x, pop_y, pop_w, pop_h, COLOR_GLOW_CYAN);
    draw_filled_rectangle(pop_x + 15, pop_y + 40, 50, 50, COLOR_DARK_BLUE); // Kalem/Fırça simgesi alanı
    draw_string_graphics(pop_x + 85, pop_y + 35, "HOS GELDINIZ", COLOR_TEXT_DARK);
    draw_string_graphics(pop_x + 85, pop_y + 65, "Sisteme Hos Geldiniz!", COLOR_TEXT_DARK);
    draw_string_graphics(pop_x + 85, pop_y + 95, "GHHOD GERLDIN!", COLOR_ACTIVE_BLUE);

    // 5. Alt Sabit Görev Çubuğu (Dock)
    int dock_w = 700;
    int dock_h = 60;
    int dock_x = 40;
    int dock_y = SCREEN_HEIGHT - 80;
    draw_filled_rectangle(dock_x, dock_y, dock_w, dock_h, COLOR_WIDGET_BG);
    draw_rectangle_outline(dock_x, dock_y, dock_w, dock_h, COLOR_GLOW_CYAN);
    draw_string_graphics(dock_x + 120, dock_y + 22, "TUSUMANA BASINCA CEKMECE ACILSIN", COLOR_TEXT_LIGHT);
}

// ==============================================================================
// 🗺️ 6. UI DURUM GEÇİŞ YÖNETİCİSİ VE KERNEL MAIN
// ==============================================================================

void refresh_system_display(void) {
    fill_screen(COLOR_DARK_BLUE);
    switch (current_os_state) {
        case STATE_WELCOME:  draw_ui_welcome_screen();  break;
        case STATE_LOCATION: draw_ui_location_screen(); break;
        case STATE_SUMMARY:  draw_ui_summary_screen();  break;
        case STATE_DESKTOP:  draw_ui_main_desktop();  break;
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

void kernel_main(void* mboot_ptr, uint32_t magic) {
    if (magic == 0x2BADB002 && mboot_ptr != NULL) {
        uint32_t flags = *(uint32_t*)mboot_ptr;
        if (flags & (1 << 11)) { 
            uint32_t* vbe_mode_info = (uint32_t*)((uint8_t*)mboot_ptr + 72);
            uint32_t real_fb_address = *vbe_mode_info;
            if (real_fb_address != 0) GRAPHICS_FRAMEBUFFER = (uint32_t*)real_fb_address;
        }
    }

    force_graphics_hardware();
    for (volatile int delay = 0; delay < 2000000; delay++) { __asm__ volatile("pause"); }

    idt_init();
    screen_init();
    keyboard_init();
    mouse_init();
    
    wind_subsystem_init();
    exe_subsystem_init();
    ai_subsystem_init();
    deb_subsystem_init();

    setup_init(); 
    current_os_state = STATE_WELCOME;
    refresh_system_display();

    while (1) {
        if (inb(0x64) & 1) { 
            uint8_t scancode = inb(0x60);
            if (is_graphics_mode) {
                setup_handle_input(scancode); 
                if (!(scancode & 0x80)) {
                    if (scancode == 0x1C) advance_os_stage();       // ENTER -> İleri
                    else if (scancode == 0x0E) regress_os_stage();  // BACKSPACE -> Geri
                }
            }
        }
        __asm__ volatile("hlt"); 
    }
}

// Linker Susturucu Köprüler (Stubs)
void idt_init(void) {}
void keyboard_init(void) {}
void mouse_init(void) {}
void wind_subsystem_init(void) {}
void exe_subsystem_init(void) {}
void ai_subsystem_init(void) {}
void deb_subsystem_init(void) {}
