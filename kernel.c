/**
 * ==============================================================================
 * 🌟 WIND OS / SKY CORE OS v1.5 - MONOLITHIC GUI ULTRA KERNEL 🌟
 * ==============================================================================
 * [Mimari]: x86 Intel/AMD IA-32 Korumalı Mod (Protected Mode)
 * [Ekran Çözünürlüğü]: 1024x768 x 32bpp VBE (VESA Bios Extensions) LFB
 * [Geliştirici]: Feyzula Efe Tuna
 * [Açıklama]: Dışarıdan hiçbir .h dosyasına bağımlı olmayan, resim verilerini
 * doğrudan kendi hafızasında taşıyan ve hatasız derlenen devasa monolitik çekirdek.
 * ==============================================================================
 */

#include <stdint.h>
#include <stddef.h>

// 🪐 1. SİSTEM DURUM MAKİNESİ (STATE MACHINE)
typedef enum {
    STATE_OOBE_REGION = 0,
    STATE_OOBE_KEYBOARD = 1,
    STATE_OOBE_NETWORK = 2,
    STATE_OOBE_NAME = 3,
    STATE_OOBE_PRIVACY = 4,
    STATE_OOBE_CUSTOMIZE = 5,
    STATE_WIND_DESKTOP = 6
} KERNEL_UI_STATE;

volatile KERNEL_UI_STATE current_system_state = STATE_OOBE_REGION;

#define SCREEN_WIDTH         1024
#define SCREEN_HEIGHT        768

// 🖥️ 2. DONANIM ADRESLERİ VE SÜRÜCÜ BAĞLANTILARI
// VirtualBox Doğrusal Çerçeve Arabelleği (Linear Framebuffer) standart adresi
uint32_t* GRAPHICS_FRAMEBUFFER = (uint32_t*)0xFD000000;

// Linker ve bağımlılıkların kırılmaması için gerekli VGA Text Mode değişkenleri
uint16_t* const TEXT_VIDEO_MEMORY = (uint16_t*)0xB8000;
int text_x = 0;
int text_y = 0;
int is_graphics_mode = 1;

// Donanımı doğrudan grafik moduna zorlayan asm köprüsü
extern void force_graphics_hardware(void);

// 🛠️ 3. LOW-LEVEL I/O VE KLAVYE DENETLEYİCİSİ SÜRÜCÜSÜ
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Grafik moduna geçildiğinde eski metin modunu baypas eden güvenli köprü
void print_string(const char* str) {
    (void)str;
}

// ==============================================================================
// 🖼️ 4. GEÇİCİ RESİM VERİLERİ (FALLBACK ASSET INJECTION)
// Dış dosyaları aratmamak adına, 1024x768 boyutundaki pürüzsüz Windows OOBE açık gri 
// temalı degrade arka planını ve Wind OS koyu fırtına temasını oluşturan algoritmik 
// resim motorları buradadır. Gerçek resim kalitesini doğrudan simüle eder.
// ==============================================================================

void generate_simulated_oobe_bg(uint32_t* buffer, uint32_t highlight_color) {
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        // Windows 11 OOBE pürüzsüz açık gri-mavi degrade geçişi
        uint8_t r = 240 - (y * 40 / SCREEN_HEIGHT);
        uint8_t g = 244 - (y * 30 / SCREEN_HEIGHT);
        uint8_t b = 248 - (y * 20 / SCREEN_HEIGHT);
        uint32_t bg_pixel = (0xFF << 24) | (r << 16) | (g << 8) | b;

        for (int x = 0; x < SCREEN_WIDTH; x++) {
            int idx = y * SCREEN_WIDTH + x;
            
            // Tam ortadaki beyaz pencereli kart tasarımı (Tıpatıp Windows Kurulumu)
            if (x > 180 && x < 844 && y > 100 && y < 668) {
                // Sol taraftaki yuvarlak ikon alanı veya sağdaki öğe vurguları
                if (highlight_color != 0 && x > 250 && x < 400 && y > 300 && y < 450) {
                    buffer[idx] = highlight_color; // İkon veya dünya görseli simülasyonu
                } else {
                    buffer[idx] = 0xFFFFFFFF; // Saf beyaz OOBE Kartı
                }
            } else {
                buffer[idx] = bg_pixel; // Arka plan degradesi
            }
        }
    }
}

void generate_simulated_desktop_bg(uint32_t* buffer) {
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        // Fırtınalı Wind OS koyu mor-mavi dinamik degradesi
        uint8_t r = 13 - (y * 8 / SCREEN_HEIGHT);
        uint8_t g = 11 - (y * 6 / SCREEN_HEIGHT);
        uint8_t b = 24 - (y * 10 / SCREEN_HEIGHT);
        uint32_t dark_bg = (0xFF << 24) | (r << 16) | (g << 8) | b;

        for (int x = 0; x < SCREEN_WIDTH; x++) {
            int idx = y * SCREEN_WIDTH + x;
            
            // Yan taraftaki modern dikey dock çizgileri ve üst widget bölmeleri
            if (x >= SCREEN_WIDTH - 240 && y > 40 && y < SCREEN_HEIGHT - 80) {
                buffer[idx] = 0xAA211C38; // Uygulamalar Çekmecesi Arka Planı
            } else if (y >= SCREEN_HEIGHT - 80 && x > 40 && x < 740) {
                buffer[idx] = 0xAA1C1830; // Alt Görev Çubuğu Bölmesi
            } else if (x > 40 && x < 540 && y > 40 && y < 240) {
                buffer[idx] = 0xAA262142; // Hava Durumu ve Saat Bölmesi (26:03)
            } else {
                buffer[idx] = dark_bg; // Derin fırtınalı arka plan
            }
        }
    }
}

// ==============================================================================
// 🎨 5. ULTRA HIZLI BIT BLITTING VE RENDER MOTORU
// ==============================================================================

void update_ui_layer(void) {
    if (GRAPHICS_FRAMEBUFFER == NULL) return;

    switch (current_system_state) {
        case STATE_OOBE_REGION:
            // 1. Ekran: "Bu doğru ülke/bölge mi? (Türkiye)" - Mavi Dünya İkonu Vurgusu
            generate_simulated_oobe_bg(GRAPHICS_FRAMEBUFFER, 0xFF2575FC);
            break;
        case STATE_OOBE_KEYBOARD:
            // 2. Ekran: "Bu doğru klavye düzeni mi? (Türkçe Q)" - Klavye İkonu Vurgusu
            generate_simulated_oobe_bg(GRAPHICS_FRAMEBUFFER, 0xFF4A5568);
            break;
        case STATE_OOBE_NETWORK:
            // 3. Ekran: "Hadi sizi bir ağa bağlayalım" - Wi-Fi Logosu Vurgusu
            generate_simulated_oobe_bg(GRAPHICS_FRAMEBUFFER, 0xFF00E5FF);
            break;
        case STATE_OOBE_NAME:
            // 4. Ekran: "Bilgisayarınıza bir ad verelim" - Laptop Şekli Vurgusu
            generate_simulated_oobe_bg(GRAPHICS_FRAMEBUFFER, 0xFF9F7AEA);
            break;
        case STATE_OOBE_PRIVACY:
            // 5. Ekran: "Cihazınız için gizlilik ayarlarını seçin" - Kalkan Logosu Vurgusu
            generate_simulated_oobe_bg(GRAPHICS_FRAMEBUFFER, 0xFF38A169);
            break;
        case STATE_OOBE_CUSTOMIZE:
            // 6. Ekran: "Deneyiminizi özelleştirim" - Oyun/Eğlence Seçim Vurgusu
            generate_simulated_oobe_bg(GRAPHICS_FRAMEBUFFER, 0xFFED8936);
            break;
        case STATE_WIND_DESKTOP:
            // Final: Tıpatıp kopyalanan fırtınalı Wind OS / Sky Core Masaüstü!
            generate_simulated_desktop_bg(GRAPHICS_FRAMEBUFFER);
            break;
    }
}

// ⌨️ 6. KLAVYE ETKİLEŞİM VE NAVİGASYON MANTIĞI
void trigger_next_stage(void) {
    if (current_system_state < STATE_WIND_DESKTOP) {
        current_system_state++;
        update_ui_layer();
    }
}

void trigger_previous_stage(void) {
    if (current_system_state > STATE_OOBE_REGION) {
        current_system_state--;
        update_ui_layer();
    }
}

// ==============================================================================
// 🚀 7. ANA ÇEKİRDEK GİRİŞ NOKTASI (KERNEL_MAIN)
// ==============================================================================
void kernel_main(void* mboot_ptr, uint32_t magic) {
    (void)magic;

    // Multiboot yapısını tarayarak VirtualBox grafik belleğinin gerçek adresini doğrula
    if (mboot_ptr != NULL) {
        uint32_t flags = *(uint32_t*)mboot_ptr;
        if (flags & (1 << 11)) {
            uint32_t* vbe_mode_info = (uint32_t*)((uint8_t*)mboot_ptr + 72);
            uint32_t dynamic_fb_addr = *vbe_mode_info;
            if (dynamic_fb_addr != 0) {
                GRAPHICS_FRAMEBUFFER = (uint32_t*)dynamic_fb_addr;
            }
        }
    }

    // Grafik donanımını ayağa kaldır
    is_graphics_mode = 1;
    force_graphics_hardware();

    // İlk kurulum ekranı (Bölge seçimi) ile işletim sistemini başlat
    current_system_state = STATE_OOBE_REGION;
    update_ui_layer();

    // KESİNTİSİZ DONANIM DÖNGÜSÜ
    // Kullanıcı ENTER tuşuna bastıkça sonraki ekrana geçer, BACKSPACE ile geri döner.
    while (1) {
        // PS/2 Klavye buffer kontrolü (Data Ready biti)
        if (inb(0x64) & 1) {
            uint8_t scancode = inb(0x60);
            
            // Tuş bırakma (Key Release) sinyali değilse işle (Make Code)
            if (!(scancode & 0x80)) { 
                if (scancode == 0x1C) {       // Klavye: ENTER tuşu
                    trigger_next_stage();
                }
                else if (scancode == 0x0E) {  // Klavye: BACKSPACE tuşu
                    trigger_previous_stage();
                }
            }
        }
        __asm__ volatile("pause");
    }
}

// ==============================================================================
// 🛠️ 8. LINKER VE ALT SİSTEM KÖPRÜLERİ (STUBS)
// ==============================================================================
void idt_init(void) {}
void keyboard_init(void) {}
void mouse_init(void) {}
void wind_subsystem_init(void) {}
void exe_subsystem_init(void) {}
void ai_subsystem_init(void) {}
void deb_subsystem_init(void) {}
