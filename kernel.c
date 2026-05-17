#include <stdint.h>
#include <stddef.h>

// Modül Fonksiyon Bildirimleri
void setup_init(void);
void setup_handle_input(uint8_t scancode);
void setup_render(void);

// Global VGA Metin Belleği Tanımları
uint16_t* const TEXT_VIDEO_MEMORY = (uint16_t*)0xB8000;
int text_x = 0;
int text_y = 0;
int is_graphics_mode = 0; // 0 = SkyOS Metin Modu, 1 = WindOS Grafik Modu

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
        setup_init();
    } else {
        print_string("\nBilinmeyen Komut! Kurulumu baslatmak icin 'ss' yazin.\nSkyOS> ");
    }
}

void kernel_main(void* mboot_ptr, uint32_t magic) {
    (void)mboot_ptr;
    (void)magic;
    
    clear_text_screen();
    print_string("=======================================================================\n");
    print_string("          Sky-OS Safe AI Kernel - Ultra Polling Klavye Aktif!\n");
    print_string("=======================================================================\n\n");
    print_string("[+] VGA Metin modu ekran surucusu: STABIL\n");
    print_string("[+] Donanimsal Kesme Korumasi (Anti-Guru Mode): AKTIF\n");
    print_string("[+] Do-frudan Port Taramali Klavye Motoru Devrede.\n\n");
    print_string("Grafiksel Wind OS kurulumuna gecmek icin 'ss' yazip ENTER'a basin.\n\n");
    print_string("SkyOS> ");

    char cmd_buffer[3] = {0};
    int cmd_idx = 0;

    // Ultra Polling Donanımsal Klavye Döngüsü
    while (1) {
        // Klavye kontrolcüsü (Port 0x64) veri hazır mı?
        if (inb(0x64) & 1) {
            uint8_t scancode = inb(0x60);
            
            if (is_graphics_mode) {
                // Eğer grafik modundaysak girdileri doğrudan Wind OS mekanik motoruna ilet
                setup_handle_input(scancode);
            } else {
                // SkyOS komut satırı girdileri
                if (scancode == 0x1F) { // 'S' Tuşu scancode
                    if (cmd_idx < 2) { cmd_buffer[cmd_idx++] = 's'; print_string("s"); }
                } else if (scancode == 0x1C) { // ENTER Tuşu
                    cmd_buffer[cmd_idx] = '\0';
                    handle_cli_command(cmd_buffer);
                    cmd_idx = 0;
                }
            }
        }
        
        // Donanımı yormamak için çok ufak bir CPU dinlendirme
        __asm__ volatile("pause");
    }
}
