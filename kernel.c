/**
 * ==============================================================================
 * 🌟 SKY CORE OS - SAF TERMİNAL ÇEKİRDEĞİ (LEGACY TEXT TERMINAL CORE) 🌟
 * ==============================================================================
 * [Mimari]: 32-Bit Korumalı Mod (Protected Mode) - x86 IA-32 Standartları
 * [Geliştirici]: Feyzula Efe Tuna
 * [Açıklama]: Grafik kartını zorlamayan, klavyeden basılan tuşları yakalayıp 
 * ekrana komut satırı (CLI) olarak basan ilk efsane saf çekirdek.
 * ==============================================================================
 */

#include <stdint.h>

// 1. LİNKER HATALARINI ENGELLEMEK İÇİN KÜRESEL GRAFİK DEĞİŞKENİ
// Diğer dosyalar (kerror.c vb.) bunu arıyor, derleme geçsin diye tanımlıyoruz.
uint32_t* GRAPHICS_FRAMEBUFFER = (uint32_t*)0xE0000000;

// x86 Standart Metin Belleği Adresi (0xB8000)
volatile uint16_t* const VIDEO_MEMORY = (uint16_t*)0xB8000;

int cursor_x = 0;
int cursor_y = 0;

// Basit US-Keyboard Scancode - ASCII Dönüşüm Tablosu (Küçük Harfler)
const char scancode_to_ascii[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',   0,
   'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0, '\\', 'z',
   'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0, '*',   0, ' ',   0
};

// Ekranı metin modunda temizleme (Koyu Mavi Arka Plan, Beyaz Yazı)
void clear_screen(void) {
    for (int i = 0; i < 80 * 25; i++) {
        VIDEO_MEMORY[i] = (0x1F << 8) | ' ';
    }
    cursor_x = 0;
    cursor_y = 0;
}

// Ekrana karakter basma ve satır sonu kontrolü
void print_char(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\b') {
        // Backspace mekanizması
        if (cursor_x > 9) { // "sky-os> " kısmını sildirmemek için
            cursor_x--;
            int index = cursor_y * 80 + cursor_x;
            VIDEO_MEMORY[index] = (0x1F << 8) | ' ';
        }
    } else {
        int index = cursor_y * 80 + cursor_x;
        VIDEO_MEMORY[index] = (0x1F << 8) | c;
        cursor_x++;
        if (cursor_x >= 80) {
            cursor_x = 0;
            cursor_y++;
        }
    }

    // Ekran aşağı kaydırma (Scrolling) koruması
    if (cursor_y >= 25) {
        clear_screen();
    }
}

// Ekrana string bastırma
void print_str(const char* str) {
    while (*str) {
        print_char(*str);
        str++;
    }
}

// Giriş portundan veri okuma (Klavye için)
static inline uint8_t in_byte(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// ==============================================================================
// 🚀 KERNEL ENTRY POINT (TERMİNAL ANA DÖNGÜSÜ)
// ==============================================================================
void kernel_main(void* mboot_ptr, uint32_t magic) {
    (void)mboot_ptr;
    (void)magic;

    clear_screen();
    print_str("======================================================================\n");
    print_str("                 WIND OS / SKY CORE OS - SAF TERMINAL                 \n");
    print_str("======================================================================\n\n");
    print_str("Sistem Metin Modunda Basariyla Baslatildi.\n");
    print_str("Klavye Sürücüsü Aktif. Komut Yazmaya Baslayabilirsiniz.\n\n");
    print_str("sky-os> ");

    uint8_t last_scancode = 0;

    // Saf Terminal Döngüsü
    while (1) {
        // Klavye veri kontrolü (Port 0x64 bit 0 doluysa veri vardır)
        if (in_byte(0x64) & 1) {
            uint8_t scancode = in_byte(0x60);

            // Tuşa basılma kontrolü (0x80'den küçükse tuşa basılmıştır, büyükse bırakılmıştır)
            if (scancode != last_scancode) {
                last_scancode = scancode;

                if (!(scancode & 0x80)) { 
                    // Enter tuşu kontrolü
                    if (scancode == 0x1C) { 
                        print_str("\n[KOMUT CALISTIRILDI]: Bilinmeyen komut.\n");
                        print_str("sky-os> ");
                    } 
                    // Backspace tuşu kontrolü
                    else if (scancode == 0x0E) {
                        print_char('\b');
                    }
                    // Normal karakter basımı
                    else if (scancode < sizeof(scancode_to_ascii)) {
                        char ascii = scancode_to_ascii[scancode];
                        if (ascii != 0) {
                            print_char(ascii);
                        }
                    }
                }
            }
        }
    }
}
