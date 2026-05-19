/**
 * ==============================================================================
 * 🌟 SKY CORE OS / WIND OS OPERATING SYSTEM 🌟
 * ==============================================================================
 * [Proje Kodu]: Sky Core OS (Saf Metin Modu Çekirdeği - Linker Uyumlu)
 * [Mimari]: x86 Intel/AMD IA-32 Monolitik Çekirdek Standartları
 * [Derleme Hedefi]: 32-Bit Korumalı Mod (Protected Mode)
 * ==============================================================================
 */

#include <stdint.h>
#include <stddef.h>

// Metin modu video bellek adresi (80x25 karakter standart VGA)
uint16_t* const TEXT_VIDEO_MEMORY = (uint16_t*)0xB8000;
int text_x = 0;
int text_y = 0;

// Orijinal kerror.c ve vga_force.c dosyalarının hata vermeden derlenmesi için sembolik tanım
uint32_t* GRAPHICS_FRAMEBUFFER = NULL;

// Çakışma yaratmamak için sadece I/O fonksiyonu
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Ekrana yazı yazdırma motoru
void print_string(const char* str) {
    while (*str) {
        if (*str == '\n') {
            text_x = 0;
            text_y++;
        } else {
            // Arka plan siyah (0), ön plan beyaz (0x0F) olacak şekilde karakteri bas
            TEXT_VIDEO_MEMORY[text_y * 80 + text_x] = (0x0F << 8) | *str;
            text_x++;
            if (text_x >= 80) { 
                text_x = 0; 
                text_y++; 
            }
        }
        str++;
    }
}

// Ekranı temizleme fonksiyonu
void clear_text_screen(void) {
    for (int i = 0; i < 80 * 25; i++) {
        TEXT_VIDEO_MEMORY[i] = (0x0F << 8) | ' ';
    }
    text_x = 0;
    text_y = 0;
}

// Ana çekirdek giriş noktası
void kernel_main(void* mboot_ptr, uint32_t magic) {
    (void)mboot_ptr;
    (void)magic;

    // Ekranı temizle ve açılış mesajını ver
    clear_text_screen();
    print_string("====================================================\n");
    print_string("            WIND OS / SKY CORE OS v1.5              \n");
    print_string("====================================================\n\n");
    print_string("[-] Grafik modu devre disi birakildi.\n");
    print_string("[+] Saf metin modu (Terminal) basariyla yuklendi.\n");
    print_string("[+] Sistem girdileri bekleniyor...\n\n");
    print_string("skycore@kernel:~$ ");

    // Sonsuz döngü ve klavye dinleme
    while (1) {
        if (inb(0x64) & 1) { 
            uint8_t scancode = inb(0x60);
            (void)scancode; 
        }
        __asm__ volatile("hlt"); 
    }
}
