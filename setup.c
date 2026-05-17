#include "setup.h"

// Çekirdekteki donanımsal port yazma fonksiyonu
extern void outb(uint16_t port, uint8_t val);

// Mevcut kurulum aşaması takibi
static SetupStage current_stage = STAGE_COUNTRY;

// Donanımsal VGA DAC registerlarını kullanarak özel modern renk paletimizi yüklüyoruz
void setup_init_palette(void) {
    // Renk 16: Arka Plan (Yumuşak Açık Gri/Mavi) -> VGA max 63 ölçeğinde
    outb(0x3C8, COLOR_WIND_BG);
    outb(0x3C9, 58); outb(0x3C9, 60); outb(0x3C9, 62);

    // Renk 17: Saf Beyaz Kart Alanları
    outb(0x3C8, COLOR_WIND_CARD);
    outb(0x3C9, 63); outb(0x3C9, 63); outb(0x3C9, 63);

    // Renk 18: Koyu Yazı Rengi
    outb(0x3C8, COLOR_WIND_TEXT);
    outb(0x3C9, 8);  outb(0x3C9, 10); outb(0x3C9, 15);

    // Renk 19: Wind OS Modern Mavi Buton Rengi
    outb(0x3C8, COLOR_WIND_PRIMARY);
    outb(0x3C9, 0);  outb(0x3C9, 30); outb(0x3C9, 53);

    // Renk 20: Canlı Turkuaz Rüzgar Esintisi
    outb(0x3C8, COLOR_WIND_TURQ);
    outb(0x3C9, 0);  outb(0x3C9, 50); outb(0x3C9, 45);

    // Renk 21: Koyu Dalga Çizgileri
    outb(0x3C8, COLOR_WIND_DARK_LINE);
    outb(0x3C9, 5);  outb(0x3C9, 4);  outb(0x3C9, 8);
}

// Belirtilen koordinata piksel basma fonksiyonu
void setup_put_pixel(int x, int y, uint8_t color) {
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
        VIDEO_MEMORY[y * SCREEN_WIDTH + x] = color;
    }
}

// Dikdörtgen / Arayüz Kutusu Çizici
void setup_draw_rect(int x, int y, int w, int h, uint8_t color) {
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            setup_put_pixel(x + j, y + i, color);
        }
    }
}

// Tasarımdaki Rüzgar Esintili Logoyu Piksellerle Çizen Sanat Fonksiyonu
void draw_wind_logo(int x, int y) {
    // Sol taraftaki estetik koyu dalgalar
    setup_draw_rect(x + 5,  y + 10, 15, 2, COLOR_WIND_DARK_LINE);
    setup_draw_rect(x + 2,  y + 12, 3,  8, COLOR_WIND_DARK_LINE);
    setup_draw_rect(x + 5,  y + 20, 25, 2, COLOR_WIND_DARK_LINE);
    setup_draw_rect(x + 27, y + 22, 3,  6, COLOR_WIND_DARK_LINE);
    
    // Üst tarafa uçuşan turkuaz rüzgar çizgileri
    setup_draw_rect(x + 16, y + 4,  14, 2, COLOR_WIND_TURQ);
    setup_draw_rect(x + 22, y + 14, 10, 2, COLOR_WIND_TURQ);
}

// Wind OS Kurulum Sihirbazının Beyaz Arka Plan Kartını Çizer
void draw_setup_window_base(void) {
    // Tüm ekranı modern arka plan rengiyle temizle
    setup_draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_WIND_BG);
    
    // Ortadaki o şık beyaz geniş penceremiz
    setup_draw_rect(12, 12, SCREEN_WIDTH - 24, SCREEN_HEIGHT - 40, COLOR_WIND_CARD);
    
    // Köşelere estetik logo ve esintileri yerleştir
    draw_wind_logo(20, 20);
    
    // Sağ alttaki aksiyon butonu (Mavi renkli "Sonraki / İleri")
    setup_draw_rect(240, 145, 55, 16, COLOR_WIND_PRIMARY);
}

// Kurulum adımlarını ekrana basan ana çizim fonksiyonu
void setup_render(void) {
    draw_setup_window_base();

    switch (current_stage) {
        case STAGE_COUNTRY:
            // "Bu doğru ülke mi?" - Liste simülasyonu, seçili olanı mavi yap
            setup_draw_rect(130, 60, 120, 14, COLOR_WIND_PRIMARY); 
            setup_draw_rect(135, 80, 90, 2, COLOR_WIND_BG);
            setup_draw_rect(135, 90, 90, 2, COLOR_WIND_BG);
            break;
            
        case STAGE_KEYBOARD:
            // "Klavye Düzeni" - Türkçe Q seçeneği ve klavye ikon taslağı
            setup_draw_rect(130, 55, 120, 14, COLOR_WIND_PRIMARY);
            setup_draw_rect(45, 80, 45, 25, COLOR_WIND_TEXT);
            setup_draw_rect(49, 84, 37, 5,  COLOR_WIND_CARD);
            break;
            
        case STAGE_NETWORK:
            // "Ağ Bağlantısı" - Wi-Fi dalgaları simülasyonu
            setup_draw_rect(60, 75, 26, 3, COLOR_WIND_TURQ);
            setup_draw_rect(65, 82, 16, 3, COLOR_WIND_TURQ);
            setup_draw_rect(130, 80, 120, 14, COLOR_WIND_BG); // Şifre kutusu
            break;
            
        case STAGE_HOSTNAME:
            // "Wind OS istasyonunuza bir ad verelim" - Cihaz ismi giriş ekranı
            setup_draw_rect(45, 75, 40, 25, COLOR_WIND_TEXT); // Laptop ikonu
            setup_draw_rect(40, 100, 50, 3,  COLOR_WIND_DARK_LINE);
            setup_draw_rect(130, 85, 120, 14, COLOR_WIND_BG); // Input alanı
            setup_draw_rect(130, 98, 120, 2,  COLOR_WIND_PRIMARY); // Aktif odak çizgisi
            break;
            
        case STAGE_CUSTOMIZE:
            // "Wind OS deneyiminizi özelleştirin" - Checkbox listesi
            setup_draw_rect(130, 50, 9, 9, COLOR_WIND_PRIMARY); // Seçili (Creative Swirls)
            setup_draw_rect(130, 70, 9, 9, COLOR_WIND_TEXT);    // Boş (Gaming Gusts)
            setup_draw_rect(131, 71, 7, 7, COLOR_WIND_CARD);
            break;
            
        case STAGE_COMPLETE:
            // Her şey bittiğinde tüm ekranı kaplayan Wind OS Mavisi
            setup_draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_WIND_PRIMARY);
            draw_wind_logo(145, 85); // Ortada parlayan logomuz
            break;
    }
}

// Grafik modunu tetikleyen ve kurulumu başlatan giriş fonksiyonu
void setup_init(void) {
    // Donanım register emri ile VGA Mode 13h (320x200 Grafik) moduna geçiş
    outb(0x3C2, 0x63);
    uint8_t vga_regs[] = {
        0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
        0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x9C, 0x0E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3, 0xFF
    };
    for (uint8_t i = 0; i < 24; i++) {
        outb(0x3D4, i);
        outb(0x3D5, vga_regs[i]);
    }

    // Renk paletini yükle ve ilk ekranı çiz
    setup_init_palette();
    setup_render();
}

// Klavyeden basılan tuşları dinleyen ve Enter (0x1C) ile aşama atlatan fonksiyon
void setup_handle_input(uint8_t scancode) {
    if (scancode == 0x1C) { // ENTER SCANCODE
        if (current_stage < STAGE_COMPLETE) {
            current_stage++;
            setup_render(); // Ekranı güncelle
        }
    }
}
