#include <stdint.h>
#include <stddef.h>

/* --- Donanım ve Sistem Tanımları --- */
#define SW 1024
#define SH 768
uint32_t* FB = (uint32_t*)0xFD000000;

typedef enum { MODE_OOBE, MODE_DESKTOP, MODE_APP_RUNNING } State;
State G_STATE = MODE_OOBE;

/* --- Donanım İletişimi (HAL) --- */
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* --- Grafik Motoru --- */
void clear(uint32_t color) { for (int i = 0; i < SW * SH; i++) FB[i] = color; }
void draw_rect(int x, int y, int w, int h, uint32_t c) {
    for (int r = y; r < y + h; r++)
        for (int col = x; col < x + w; col++)
            if (col < SW && r < SH) FB[r * SW + col] = c;
}

/* --- Uygulama Mantığı --- */
void render_oobe() {
    clear(0xFF0078D4); // Mavi Kurulum
    draw_rect(300, 300, 400, 100, 0xFFFFFFFF); // Buton
}

void render_desktop() {
    clear(0xFF202020); // Gri Masaüstü
    draw_rect(50, 50, 100, 100, 0xFF00E5FF); // İkon
}

void run_app() {
    clear(0xFF000000); // Siyah Uygulama Ekranı
}

/* --- ANA ÇEKİRDEK (Workflow) --- */
void kernel_main(void *mboot_ptr, uint32_t magic) {
    (void)mboot_ptr; (void)magic;
    
    // Donanım başlatma stub'ları
    // idt_init(); keyboard_init();
    
    while (1) {
        // 1. Girdi Kontrolü (State Machine)
        if (inb(0x64) & 1) {
            uint8_t sc = inb(0x60);
            if (G_STATE == MODE_OOBE && sc == 0x1C) G_STATE = MODE_DESKTOP;
            else if (G_STATE == MODE_DESKTOP && sc == 0x02) G_STATE = MODE_APP_RUNNING;
        }

        // 2. Render (State Machine)
        switch (G_STATE) {
            case MODE_OOBE:        render_oobe();    break;
            case MODE_DESKTOP:     render_desktop(); break;
            case MODE_APP_RUNNING: run_app();        break;
        }

        __asm__ volatile("hlt");
    }
}
