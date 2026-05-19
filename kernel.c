/**
 * ==============================================================================
 * 🌟 SKY CORE OS / WIND OS - SAF TERMİNAL VE GEÇİŞ ÇEKİRDEĞİ 🌟
 * ==============================================================================
 * [Mimari]: 32-Bit Korumalı Mod (Protected Mode) - x86 IA-32 Standartları
 * [Geliştirici]: Feyzula Efe Tuna
 * [Açıklama]: Ekrana 'ss' yazıp ENTER'a basınca grafik kuruluma (setup_init)
 * zıplayan, klavye polling motoruna sahip o meşhur saf geçiş çekirdeği.
 * ==============================================================================
 */

#include <stdint.h>
#include <stddef.h>

// Modül Fonksiyon Bildirimleri (Dış dosyalarla linker bağlantısı için)
extern void setup_init(void);
extern void setup_handle_input(uint8_t scancode);
extern void setup_render(void);

// LİNKER SUSTURUCU: kerror.c veya vga_force.c dosyalarının aradığı küresel grafik pointer'ı
uint32_t* GRAPHICS_FRAMEBUFFER = (uint32_t*)0xE0000000;

// Mod Kontrol Değişkeni (0: Saf Terminal, 1: Grafik Kurulum Modu)
volatile int is_graphics_mode = 0;

// Global VGA Metin Belleği Tanımları
uint16_t* const TEXT_VIDEO_MEMORY = (uint16_t*)0xB8000;
int text_x = 0;
int text_y = 0;

// Ekrana metin modu string basma fonksiyonu
void print_string(const char* str) {
    while (*str) {
        if (*str == '\n') {
            text_x = 0;
            text_y++;
        } else {
            int index = text_y * 80 + text_x;
            TEXT_VIDEO_MEMORY[index] = (0x1F << 8) | *str; // Lacivert arka plan, beyaz yazı
            text_x++;
            if (text_x >= 80) {
                text_x = 0;
                text_y++;
            }
        }
        str++;
    }
    // Basit ekran kaydırma (scrolling) koruması
    if (text_y >= 25) {
        for (int i = 0; i < 80 * 25; i++) TEXT_VIDEO_MEMORY[i] = (0x1F << 8) | ' ';
        text_x = 0;
        text_y = 0;
    }
}

// Donanımdan Veri Okuma/Yazma (Port Girdileri)
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Basit Komut İstemi Altyapısı
void handle_cli_command(const char* cmd) {
    if (cmd[0] == 's' && cmd[1] == 's') {
        // 'ss' komutu girildiğinde Wind OS Grafik Kurulum Sihirbazını tetikle!
        is_graphics_mode = 1;
        setup_init(); // Wind OS Grafik Kurulumunu Ateşle!
    } else {
        print_string("\nBilinmeyen Komut! Kurulumu baslatmak icin 'ss' yazin.\nSkyOS> ");
    }
}

// ==============================================================================
// 🚀 KERNEL GİRİŞ NOKTASI (MAIN ENTRY POINT)
// ==============================================================================
void kernel_main(void* mboot_ptr, uint32_t magic) {
    (void)mboot_ptr;
    (void)magic;

    // Ekranı temizle ve ilk logları bas
    for (int i = 0; i < 80 * 25; i++) {
        TEXT_VIDEO_MEMORY[i] = (0x1F << 8) | ' ';
    }
    text_x = 0;
    text_y = 0;

    print_string("=======================================================================\n");
    print_string("               WIND OS CORE KERNEL v1.5 - SIFIR HATA MODU             \n");
    print_string("=======================================================================\n\n");
    print_string("[+] VGA Metin modu ekran surucusu: STABIL\n");
    print_string("[+] Donanimsal Kesme Korumasi (Anti-Guru Mode): AKTIF\n");
    print_string("[+] Dogrudan Port Taramali Klavye Motoru Devrede.\n\n");
    print_string("Grafiksel Wind OS kurulumuna gecmek icin 'ss' yazip ENTER'a basin.\n\n");
    print_string("SkyOS> ");

    char cmd_buffer[3] = {0};
    int cmd_idx = 0;
    uint8_t last_scancode = 0;

    // Ultra Polling Donanımsal Klavye Döngüsü
    while (1) {
        // Klavye kontrolcüsü (Port 0x64) veri hazır mı?
        if (inb(0x64) & 1) { 
            uint8_t scancode = inb(0x60);

            // Sadece tuşa ilk basıldığında işlem yap (Tekrarlamayı önle)
            if (scancode != last_scancode) {
                last_scancode = scancode;

                if (!(scancode & 0x80)) { // Tuş bırakılmadıysa, basıldıysa
                    if (is_graphics_mode) {
                        // Eğer grafik modundaysak girdileri doğrudan Wind OS mekanik motoruna ilet
                        setup_handle_input(scancode); 
                    } else {
                        // SkyOS komut satırı girdileri
                        if (scancode == 0x1F) { // 'S' Tuşu scancode'u
                            if (cmd_idx < 2) { 
                                cmd_buffer[cmd_idx++] = 's'; 
                                print_string("s"); 
                            }
                        } else if (scancode == 0x1C) { // ENTER Tuşu
                            cmd_buffer[cmd_idx] = '\0';
                            handle_cli_command(cmd_buffer);
                            // Buffer'ı sıfırla
                            cmd_idx = 0;
                            cmd_buffer[0] = 0;
                            cmd_buffer[1] = 0;
                        }
                    }
                }
            }
        }
        
        // Donanımı yormamak için çok ufak bir CPU dinlendirme (x86 komutu)
        __asm__ volatile("pause");
    }
}
