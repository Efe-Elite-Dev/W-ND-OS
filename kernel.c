/**
 * ==============================================================================
 * 🌟 SKY CORE OS / WIND OS - SAF ÇEKİRDEK (PURE CORE VORTEX KERNEL) 🌟
 * ==============================================================================
 * [Mimari]: 32-Bit Korumalı Mod (Protected Mode) - x86 IA-32 Monolitik Standartları
 * [Geliştirici]: Feyzula Efe Tuna
 * [Açıklama]: UI bileşenleri içermeyen, sadece donanım ilklendirmesi yapan saf core.
 * ==============================================================================
 */

#include <stdint.h>
#include <stddef.h>

#define SCREEN_WIDTH         1024
#define SCREEN_HEIGHT        768
#define COLOR_DARK_BLUE      0xFF0D0B18
#define COLOR_WHITE          0xFFFFFFFF

// ==============================================================================
// 🖥️ 1. DONANIM VE BELLEK ADRESLERİ KATMANI (MEMORY MAPPING)
// ==============================================================================
uint16_t* const TEXT_VIDEO_MEMORY = (uint16_t*)0xB8000; // x86 Standart Metin Belleği
int text_x = 0;
int text_y = 0;

// VirtualBox / QEMU için dinamik Linear Framebuffer adresi (Varsayılan: 0xE0000000)
uint32_t* GRAPHICS_FRAMEBUFFER = (uint32_t*)0xE0000000;

// Dışarıdan (assembly veya setup dosyalarından) gelecek fonksiyonlar
extern void force_graphics_hardware(void);

// Linker (Bağlayıcı) hatalarını engellemek için saf stub alt sistemler
void idt_init(void) {} 
void keyboard_init(void) {}
void mouse_init(void) {}
void screen_init(void) {}
void wind_subsystem_init(void) {}
void exe_subsystem_init(void) {}
void ai_subsystem_init(void) {}
void deb_subsystem_init(void) {}
void setup_init(void) {}

// ==============================================================================
// 🛠️ 2. SAF I/O PORTS VE METİN MODU LOG MOTORU
// ==============================================================================
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void clear_text_screen(void) {
    for (int i = 0; i < 80 * 25; i++) {
        TEXT_VIDEO_MEMORY[i] = (0x0F << 8) | ' ';
    }
    text_x = 0;
    text_y = 0;
}

void print_string(const char* str) {
    while (*str) {
        if (*str == '\n') {
            text_x = 0;
            text_y++;
        } else {
            TEXT_VIDEO_MEMORY[text_y * 80 + text_x] = (0x0E << 8) | *str; // Sarı renkli log metni
            text_x++;
            if (text_x >= 80) { text_x = 0; text_y++; }
        }
        str++;
    }
}

// ==============================================================================
// 🎨 3. SAF GRAFİK İLKEL ÇİZİM FONKSİYONLARI (PRIMITIVES)
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

// ==============================================================================
// 🚀 4. KERNEL ENTRY POINT (SAF MİMARİ GİRİŞİ)
// ==============================================================================
void kernel_main(void* mboot_ptr, uint32_t magic) {
    // 1. Metin ekranını temizle ve ilk yaşam sinyalini ver
    clear_text_screen();
    print_string("Sky Core OS: Saf cekirdek yukleniyor...\n");

    // 2. Multiboot VBE standart kontrolü (Dinamik LFB Adres Alımı)
    if (magic == 0x2BADB002 && mboot_ptr != NULL) {
        uint32_t flags = *(uint32_t*)mboot_ptr;
        if (flags & (1 << 11)) { // VBE info mevcut mu?
            uint32_t* vbe_mode_info = (uint32_t*)((uint8_t*)mboot_ptr + 72);
            uint32_t real_fb_address = *vbe_mode_info;
            if (real_fb_address != 0) {
                GRAPHICS_FRAMEBUFFER = (uint32_t*)real_fb_address;
            }
        }
    }

    print_string("Sky Core OS: Donanim baglamlari hazirlaniyor...\n");
    for (volatile int delay = 0; delay < 2000000; delay++) { __asm__ volatile("pause"); }

    // Çekirdek alt sistemlerini çağır
    screen_init();
    setup_init();

    // 3. Grafik moduna geç ve arka planı boya (Saf çekirdeğin çalıştığının kanıtı)
    fill_screen(COLOR_DARK_BLUE);

    // 4. Sonsuz Klavye Polling Döngüsü (Kesmesiz / hlt içermez, kilitlenmeyi önler)
    while (1) {
        if (inb(0x64) & 1) { // Klavye tamponunda veri var mı?
            uint8_t scancode = inb(0x60); // Scancode'u oku (İleride input işlemleri için hazır)
            (void)scancode; 
        }
    }
}
