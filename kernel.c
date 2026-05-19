/**
 * ==============================================================================
 * 🌟 WIND OS / SKY CORE OS v1.5 - MONOLITHIC GUI ULTRA KERNEL 🌟
 * WIND OS / SKY CORE OS v1.5 - Vortex Kernel
 * ==============================================================================
 * Mimari   : x86 Intel/AMD IA-32 Korumalı Mod
 * Hedef    : 1024x768 x 32bpp VBE Linear Framebuffer
 * Geliştirici: Feyzula Efe Tuna
 * ==============================================================================
 *
 * LINKER KURALLARI (DEĞİŞTİRME):
 *  - GRAPHICS_FRAMEBUFFER  : burada tanımlı, kerror.c / vga_force.c extern kullanır
 *  - is_graphics_mode      : burada tanımlı, vga_force.c extern kullanır
 *  - screen_init           : screen.c'de tanımlı  → buraya YAZMA
 *  - setup_init            : setup.c'de tanımlı   → buraya YAZMA
 *  - setup_handle_input    : setup.c'de tanımlı   → buraya YAZMA
 *  - trigger_next_stage    : setup.c'de tanımlı   → buraya YAZMA
 * ==============================================================================
 */

 
#include <stdint.h>
#include <stddef.h>
#include "globals.h" // Ortak değişken tanımları

// Global Değişken Tanımları (Burada static yok, yani diğer dosyalar görebilir)
 
/* ============================================================
 * 1. GLOBAL DEĞİŞKENLER
 *    Bu değişkenler tüm proje genelinde paylaşılır.
 *    kerror.c ve vga_force.c, extern ile bu adresleri kullanır.
 * ============================================================ */
uint32_t* GRAPHICS_FRAMEBUFFER = (uint32_t*)0xFD000000;
int is_graphics_mode = 1;

// Harici fonksiyon bildirimleri
extern void force_graphics_hardware(void);

// 🪐 1. SİSTEM DURUM MAKİNESİ
typedef enum {
    SKY_STATE_OOBE_REGION = 0,
    SKY_STATE_OOBE_KEYBOARD = 1,
    SKY_STATE_OOBE_NETWORK = 2,
    SKY_STATE_OOBE_NAME = 3,
    SKY_STATE_OOBE_PRIVACY = 4,
    SKY_STATE_OOBE_CUSTOMIZE = 5,
    SKY_STATE_WIND_DESKTOP = 6
} SKY_KERNEL_UI_STATE;

static volatile SKY_KERNEL_UI_STATE current_sky_state = SKY_STATE_OOBE_REGION;

#define SCREEN_WIDTH         1024
#define SCREEN_HEIGHT        768

// 🛠️ 2. LOW-LEVEL I/O PORT SÜRÜCÜSÜ
static inline uint8_t sky_inb(uint16_t port) {
int       is_graphics_mode     = 1;
 
/* ============================================================
 * 2. DIŞ BAĞLANTI BİLDİRİMLERİ
 * ============================================================ */
extern void force_graphics_hardware(void);   /* vga_force.c */
extern void screen_init(void);               /* screen.c    */
extern void setup_init(void);                /* setup.c     */
extern void setup_handle_input(uint8_t sc);  /* setup.c     */
 
/* ============================================================
 * 3. DONANIM I/O
 * ============================================================ */
static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// 🖼️ 3. ALGORİTMİK GÖRSEL MOTORU
static void generate_sky_oobe_bg(uint32_t* buffer, uint32_t highlight_color) {
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        uint8_t r = 240 - (y * 40 / SCREEN_HEIGHT);
        uint8_t g = 244 - (y * 30 / SCREEN_HEIGHT);
        uint8_t b = 248 - (y * 20 / SCREEN_HEIGHT);
        uint32_t bg_pixel = (0xFF << 24) | (r << 16) | (g << 8) | b;
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            int idx = y * SCREEN_WIDTH + x;
            if (x > 180 && x < 844 && y > 100 && y < 668) {
                if (highlight_color != 0 && x > 250 && x < 400 && y > 300 && y < 450)
                    buffer[idx] = highlight_color;
                else
                    buffer[idx] = 0xFFFFFFFF;
            } else {
                buffer[idx] = bg_pixel;
            }
        }
 
/* Metin modu bypass — grafik modundayken print çağrılarını yut */
void print_string(const char *str) { (void)str; }
 
/* ============================================================
 * 4. EKRaN BOYUTLARI
 * ============================================================ */
#define SW 1024
#define SH 768
 
/* ============================================================
 * 5. GRAFİK ÇİZİM MOTORU
 * ============================================================ */
static inline void put_px(int x, int y, uint32_t c)
{
    if ((unsigned)x < SW && (unsigned)y < SH)
        GRAPHICS_FRAMEBUFFER[y * SW + x] = c;
}
 
static void fill_screen(uint32_t c)
{
    int n = SW * SH;
    for (int i = 0; i < n; i++) GRAPHICS_FRAMEBUFFER[i] = c;
}
 
static void draw_rect(int x, int y, int w, int h, uint32_t c)
{
    for (int r = y; r < y + h; r++)
        for (int col = x; col < x + w; col++)
            put_px(col, r, c);
}
 
static void draw_rect_outline(int x, int y, int w, int h, uint32_t c)
{
    for (int i = x; i < x + w; i++) { put_px(i, y, c); put_px(i, y+h-1, c); }
    for (int i = y; i < y + h; i++) { put_px(x, i, c); put_px(x+w-1, i, c); }
}
 
/* Dikey gradyan */
static void gradient_v(uint32_t top, uint32_t bot)
{
    for (int y = 0; y < SH; y++) {
        int t = SH - y, b = y;
        uint8_t r = (uint8_t)(((top>>16&0xFF)*t + (bot>>16&0xFF)*b) / SH);
        uint8_t g = (uint8_t)(((top>>8 &0xFF)*t + (bot>>8 &0xFF)*b) / SH);
        uint8_t bl= (uint8_t)(((top    &0xFF)*t + (bot    &0xFF)*b) / SH);
        uint32_t px = (0xFF<<24)|(r<<16)|(g<<8)|bl;
        for (int x = 0; x < SW; x++) GRAPHICS_FRAMEBUFFER[y*SW+x] = px;
    }
}

static void generate_sky_desktop_bg(uint32_t* buffer) {
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        uint8_t r = 13 - (y * 8 / SCREEN_HEIGHT);
        uint8_t g = 11 - (y * 6 / SCREEN_HEIGHT);
        uint8_t b = 24 - (y * 10 / SCREEN_HEIGHT);
        uint32_t dark_bg = (0xFF << 24) | (r << 16) | (g << 8) | b;
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            int idx = y * SCREEN_WIDTH + x;
            if (x >= SCREEN_WIDTH - 240 && y > 40 && y < SCREEN_HEIGHT - 80) buffer[idx] = 0xAA211C38;
            else if (y >= SCREEN_HEIGHT - 80 && x > 40 && x < 740) buffer[idx] = 0xAA1C1830;
            else if (x > 40 && x < 540 && y > 40 && y < 240) buffer[idx] = 0xAA262142;
            else buffer[idx] = dark_bg;
 
/* Çok basit 5x7 bitmap font — sadece büyük harf + rakam + bazı semboller */
static const uint8_t FONT5[128][7] = {
    /* Sadece ASCII 32-90 (boşluk, !"#...Z) — tüm tablo 128 giriş */
    [' '] = {0,0,0,0,0,0,0},
    ['!'] = {0x04,0x04,0x04,0x04,0x00,0x04,0x00},
    ['"'] = {0x0A,0x0A,0x00,0x00,0x00,0x00,0x00},
    ['&'] = {0x0C,0x12,0x0C,0x1B,0x12,0x0D,0x00},
    ['.'] = {0x00,0x00,0x00,0x00,0x00,0x04,0x00},
    [','] = {0x00,0x00,0x00,0x00,0x04,0x04,0x08},
    [':'] = {0x00,0x04,0x00,0x00,0x04,0x00,0x00},
    ['-'] = {0x00,0x00,0x1F,0x00,0x00,0x00,0x00},
    ['/'] = {0x01,0x02,0x04,0x08,0x10,0x00,0x00},
    ['0'] = {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E},
    ['1'] = {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E},
    ['2'] = {0x0E,0x11,0x01,0x06,0x08,0x10,0x1F},
    ['3'] = {0x1F,0x02,0x04,0x02,0x01,0x11,0x0E},
    ['4'] = {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02},
    ['5'] = {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E},
    ['6'] = {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E},
    ['7'] = {0x1F,0x01,0x02,0x04,0x08,0x08,0x08},
    ['8'] = {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E},
    ['9'] = {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C},
    ['A'] = {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11},
    ['B'] = {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E},
    ['C'] = {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E},
    ['D'] = {0x1E,0x11,0x11,0x11,0x11,0x11,0x1E},
    ['E'] = {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F},
    ['F'] = {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10},
    ['G'] = {0x0E,0x11,0x10,0x17,0x11,0x11,0x0E},
    ['H'] = {0x11,0x11,0x11,0x1F,0x11,0x11,0x11},
    ['I'] = {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E},
    ['J'] = {0x07,0x02,0x02,0x02,0x02,0x12,0x0C},
    ['K'] = {0x11,0x12,0x14,0x18,0x14,0x12,0x11},
    ['L'] = {0x10,0x10,0x10,0x10,0x10,0x10,0x1F},
    ['M'] = {0x11,0x1B,0x15,0x15,0x11,0x11,0x11},
    ['N'] = {0x11,0x19,0x15,0x13,0x11,0x11,0x11},
    ['O'] = {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E},
    ['P'] = {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10},
    ['Q'] = {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D},
    ['R'] = {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11},
    ['S'] = {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E},
    ['T'] = {0x1F,0x04,0x04,0x04,0x04,0x04,0x04},
    ['U'] = {0x11,0x11,0x11,0x11,0x11,0x11,0x0E},
    ['V'] = {0x11,0x11,0x11,0x11,0x11,0x0A,0x04},
    ['W'] = {0x11,0x11,0x15,0x15,0x15,0x1B,0x11},
    ['X'] = {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11},
    ['Y'] = {0x11,0x11,0x0A,0x04,0x04,0x04,0x04},
    ['Z'] = {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F},
    ['a'] = {0x00,0x00,0x0E,0x01,0x0F,0x11,0x0F},
    ['b'] = {0x10,0x10,0x1E,0x11,0x11,0x11,0x1E},
    ['c'] = {0x00,0x00,0x0E,0x11,0x10,0x11,0x0E},
    ['d'] = {0x01,0x01,0x0F,0x11,0x11,0x11,0x0F},
    ['e'] = {0x00,0x00,0x0E,0x11,0x1F,0x10,0x0E},
    ['f'] = {0x06,0x09,0x08,0x1C,0x08,0x08,0x08},
    ['g'] = {0x00,0x0F,0x11,0x11,0x0F,0x01,0x0E},
    ['h'] = {0x10,0x10,0x1E,0x11,0x11,0x11,0x11},
    ['i'] = {0x04,0x00,0x0C,0x04,0x04,0x04,0x0E},
    ['j'] = {0x02,0x00,0x06,0x02,0x02,0x12,0x0C},
    ['k'] = {0x10,0x10,0x12,0x14,0x18,0x14,0x12},
    ['l'] = {0x0C,0x04,0x04,0x04,0x04,0x04,0x0E},
    ['m'] = {0x00,0x00,0x1A,0x15,0x15,0x11,0x11},
    ['n'] = {0x00,0x00,0x1E,0x11,0x11,0x11,0x11},
    ['o'] = {0x00,0x00,0x0E,0x11,0x11,0x11,0x0E},
    ['p'] = {0x00,0x1E,0x11,0x11,0x1E,0x10,0x10},
    ['q'] = {0x00,0x0F,0x11,0x11,0x0F,0x01,0x01},
    ['r'] = {0x00,0x00,0x16,0x19,0x10,0x10,0x10},
    ['s'] = {0x00,0x00,0x0E,0x10,0x0E,0x01,0x1E},
    ['t'] = {0x08,0x08,0x1C,0x08,0x08,0x09,0x06},
    ['u'] = {0x00,0x00,0x11,0x11,0x11,0x11,0x0F},
    ['v'] = {0x00,0x00,0x11,0x11,0x11,0x0A,0x04},
    ['w'] = {0x00,0x00,0x11,0x15,0x15,0x15,0x0A},
    ['x'] = {0x00,0x00,0x11,0x0A,0x04,0x0A,0x11},
    ['y'] = {0x00,0x11,0x11,0x0F,0x01,0x11,0x0E},
    ['z'] = {0x00,0x00,0x1F,0x02,0x04,0x08,0x1F},
};
 
static void draw_char(int x, int y, char ch, uint32_t col, int scale)
{
    if ((unsigned char)ch >= 128) return;
    const uint8_t *bm = FONT5[(unsigned char)ch];
    for (int row = 0; row < 7; row++)
        for (int bit = 4; bit >= 0; bit--)
            if (bm[row] & (1 << bit))
                for (int sy = 0; sy < scale; sy++)
                    for (int sx = 0; sx < scale; sx++)
                        put_px(x + (4-bit)*scale + sx, y + row*scale + sy, col);
}
 
static void draw_str(int x, int y, const char *s, uint32_t col, int scale)
{
    while (*s) { draw_char(x, y, *s, col, scale); x += 6*scale; s++; }
}
 
/* ============================================================
 * 6. UI EKRANLARı
 * ============================================================ */
 
/* Ortak: açık degrade zemin + beyaz kart */
static void draw_oobe_base(void)
{
    gradient_v(0xFFE8F4FD, 0xFFD0E8F8);          /* açık mavi-beyaz */
    draw_rect(190, 90, 644, 560, 0x22000000);    /* kart gölgesi */
    draw_rect(186, 86, 644, 560, 0x11000000);
    draw_rect(192, 94, 640, 556, 0xFFFFFFFF);    /* beyaz kart */
    draw_rect_outline(192, 94, 640, 556, 0xFFDDE5EE);
}
 
/* Ortak: "İleri" mavi butonu */
static void draw_next_btn(int bx, int by)
{
    draw_rect(bx, by, 180, 46, 0xFF0078D4);
    draw_rect_outline(bx, by, 180, 46, 0xFF005A9E);
    draw_str(bx+50, by+15, "Evet", 0xFFFFFFFF, 2);
}
 
/* ─────────── EKRAN 1 : Ülke / Bölge ─────────── */
static void render_oobe_region(void)
{
    draw_oobe_base();
    /* Başlık */
    draw_str(270, 130, "Bu dogru ulke", 0xFF1A1A2E, 2);
    draw_str(270, 155, "veya bolge mi?", 0xFF1A1A2E, 2);
    /* Seçim kutusu */
    draw_rect(280, 230, 440, 50, 0xFFF0F6FF);
    draw_rect_outline(280, 230, 440, 50, 0xFF0078D4);
    draw_str(294, 248, "Turkiye", 0xFF1A1A2E, 2);
    /* Açıklama */
    draw_str(230, 460, "ENTER - Ileri    BACKSPACE - Geri", 0xFF666680, 1);
    draw_next_btn(590, 570);
}
 
/* ─────────── EKRAN 2 : Klavye Düzeni ─────────── */
static void render_oobe_keyboard(void)
{
    draw_oobe_base();
    draw_str(250, 130, "Bu dogru klavye", 0xFF1A1A2E, 2);
    draw_str(250, 155, "duzeni mi?", 0xFF1A1A2E, 2);
 
    /* Klavye görseli (sembolik) */
    int kx=220, ky=230;
    for (int row=0; row<4; row++) {
        int keys = (row==0)?12:(row==1)?11:(row==2)?10:9;
        int kw=42, kh=38, gap=4;
        for (int k=0; k<keys; k++) {
            draw_rect(kx + k*(kw+gap) + row*8, ky+row*(kh+gap), kw, kh, 0xFFEFF3F8);
            draw_rect_outline(kx + k*(kw+gap) + row*8, ky+row*(kh+gap), kw, kh, 0xFFB0BEC5);
        }
    }
 
    draw_str(250, 480, "Secim: Turkce Q Klavyesi", 0xFF444466, 1);
    draw_str(230, 460, "ENTER - Ileri    BACKSPACE - Geri", 0xFF666680, 1);
    draw_next_btn(590, 570);
}

// 🎨 4. GÜNCELLEME MOTORU
static void update_sky_ui_layer(void) {
    if (GRAPHICS_FRAMEBUFFER == NULL) return;
    switch (current_sky_state) {
        case SKY_STATE_OOBE_REGION:    generate_sky_oobe_bg(GRAPHICS_FRAMEBUFFER, 0xFF2575FC); break;
        case SKY_STATE_OOBE_KEYBOARD:  generate_sky_oobe_bg(GRAPHICS_FRAMEBUFFER, 0xFF4A5568); break;
        case SKY_STATE_OOBE_NETWORK:   generate_sky_oobe_bg(GRAPHICS_FRAMEBUFFER, 0xFF00E5FF); break;
        case SKY_STATE_OOBE_NAME:      generate_sky_oobe_bg(GRAPHICS_FRAMEBUFFER, 0xFF9F7AEA); break;
        case SKY_STATE_OOBE_PRIVACY:   generate_sky_oobe_bg(GRAPHICS_FRAMEBUFFER, 0xFF38A169); break;
        case SKY_STATE_OOBE_CUSTOMIZE: generate_sky_oobe_bg(GRAPHICS_FRAMEBUFFER, 0xFFED8936); break;
        case SKY_STATE_WIND_DESKTOP:   generate_sky_desktop_bg(GRAPHICS_FRAMEBUFFER); break;
 
/* ─────────── EKRAN 3 : Ağ Bağlantısı ─────────── */
static void render_oobe_network(void)
{
    draw_oobe_base();
    draw_str(235, 130, "Bir aga baglanin", 0xFF1A1A2E, 2);
 
    /* Wi-Fi ikonları */
    int wx=452, wy=220;
    for (int a=0; a<4; a++) {
        int r=(a+1)*30, off=a*8;
        draw_rect_outline(wx-r, wy-r/2+off, r*2, r, 0xFF0078D4+a*0x111111);
    }
    draw_rect(wx-4, wy+70, 8, 20, 0xFF0078D4);
 
    /* Ağ listesi */
    const char *nets[3]={"Ev Wifi","Ofis-5G","Misafir"};
    for (int i=0; i<3; i++) {
        draw_rect(280, 340+i*60, 440, 46, i==0?0xFFE8F4FD:0xFFF5F5F5);
        draw_rect_outline(280, 340+i*60, 440, 46, i==0?0xFF0078D4:0xFFDDDDDD);
        draw_str(300, 356+i*60, nets[i], 0xFF1A1A2E, 2);
    }
 
    draw_str(230, 560, "ENTER - Ileri    BACKSPACE - Geri", 0xFF666680, 1);
    draw_next_btn(590, 570);
}
 
/* ─────────── EKRAN 4 : Bilgisayar Adı ─────────── */
static void render_oobe_name(void)
{
    draw_oobe_base();
    draw_str(220, 130, "Bilgisayariniza", 0xFF1A1A2E, 2);
    draw_str(220, 155, "bir ad verin", 0xFF1A1A2E, 2);
 
    /* Laptop ikonu */
    draw_rect(390, 220, 220, 140, 0xFF37474F); /* ekran */
    draw_rect(398, 228, 204, 120, 0xFF263238);
    draw_rect(360, 360, 280, 16, 0xFF546E7A); /* kasa */
    draw_rect(350, 376, 300, 8, 0xFF455A64);
 
    /* Giriş kutusu */
    draw_rect(230, 420, 560, 50, 0xFFFFFFFF);
    draw_rect_outline(230, 420, 560, 50, 0xFF0078D4);
    draw_str(244, 438, "Wind-PC", 0xFF1A1A2E, 2);
 
    draw_str(230, 510, "ENTER - Ileri    BACKSPACE - Geri", 0xFF666680, 1);
    draw_next_btn(590, 570);
}

// Navigasyon (İsimleri çakışmasın diye değiştirildi)
static void sky_process_next(void) {
    if (current_sky_state < SKY_STATE_WIND_DESKTOP) {
        current_sky_state++;
        update_sky_ui_layer();
 
/* ─────────── EKRAN 5 : Gizlilik ─────────── */
static void render_oobe_privacy(void)
{
    draw_oobe_base();
    draw_str(230, 130, "Gizlilik ayarlari", 0xFF1A1A2E, 2);
 
    const char *items[5]={
        "Konum servislerini ac",
        "Teshis verisi gonder",
        "Kisisellestirilmis deneyim",
        "Reklam kimligini kullan",
        "Ag baglantilarini optimize et"
    };
    for (int i=0; i<5; i++) {
        draw_rect(230, 220+i*52, 540, 44, 0xFFF9F9F9);
        draw_rect_outline(230, 220+i*52, 540, 44, 0xFFDDDDDD);
        /* Toggle kutusu */
        draw_rect(700, 232+i*52, 50, 22, 0xFF0078D4);
        draw_rect(722, 234+i*52, 18, 18, 0xFFFFFFFF);
        draw_str(244, 234+i*52, items[i], 0xFF333333, 1);
    }
 
    draw_str(230, 510, "ENTER - Ileri    BACKSPACE - Geri", 0xFF666680, 1);
    draw_next_btn(590, 570);
}

static void sky_process_prev(void) {
    if (current_sky_state > SKY_STATE_OOBE_REGION) {
        current_sky_state--;
        update_sky_ui_layer();
 
/* ─────────── EKRAN 6 : Özelleştirme ─────────── */
static void render_oobe_customize(void)
{
    draw_oobe_base();
    draw_str(200, 130, "Deneyiminizi ozellestirin", 0xFF1A1A2E, 2);
    draw_str(230, 165, "Size en uygun kullanim amacini secin:", 0xFF444466, 1);
 
    const char *cats[4]={"Oyun","Isletme","Okullar","Kisisel"};
    uint32_t cols[4]={0xFF7C3AED,0xFF0078D4,0xFF059669,0xFFEA580C};
    for (int i=0; i<4; i++) {
        int cx=232+i*158, cy=240;
        draw_rect(cx, cy, 140, 140, cols[i]);
        draw_rect_outline(cx, cy, 140, 140, 0xFF333333);
        draw_str(cx+8, cy+120, cats[i], 0xFFFFFFFF, 2);
    }
 
    draw_str(300, 440, "Birini secin ve ilerine basın", 0xFF444466, 1);
    draw_str(230, 510, "ENTER - Ileri    BACKSPACE - Geri", 0xFF666680, 1);
    draw_next_btn(590, 570);
}
 
/* ─────────── EKRAN 7 : MASAÜSTÜ ─────────── */
static void render_desktop(void)
{
    /* Koyu mor gradyan zemin */
    gradient_v(0xFF1A0F2E, 0xFF0D0B18);
 
    /* ── Üst görev çubuğu ── */
    draw_rect(0, 0, SW, 38, 0xAA14102A);
    draw_str(10, 12, "Wind OS v1.5", 0xFF00E5FF, 1);
    draw_str(900, 12, "26:03", 0xFF00E5FF, 1);
 
    /* ── Sol üst: Hava & Saat Widget ── */
    draw_rect(30, 52, 480, 180, 0xAA211C38);
    draw_rect_outline(30, 52, 480, 180, 0xFF444466);
    draw_str(50, 72,  "SAAT: 26:03", 0xFF00E5FF, 2);
    draw_str(50, 112, "Hava Durumu - Esenyurt", 0xFFF5F5F5, 1);
    draw_str(50, 132, "21C  Bulutlu ve Firtina", 0xFFD0D0D0, 1);
 
    /* Ay ikonu (sembolik daire) */
    int mx=432, my=115, mr=42;
    for (int y2=-mr; y2<=mr; y2++)
        for (int x2=-mr; x2<=mr; x2++)
            if (x2*x2+y2*y2 <= mr*mr)
                put_px(mx+x2, my+y2, 0xFFFFD700);
    for (int y2=-mr+6; y2<=mr; y2++)
        for (int x2=-mr+14; x2<=mr+14; x2++)
            if (x2*x2+y2*y2 <= mr*mr)
                put_px(mx+x2, my+y2, 0xFF1A0F2E);
 
    /* ── Sol orta: İkon dock ── */
    const char *dock_labels[4]={"Kamera","Mesaj","Dosya","Terminal"};
    for (int i=0; i<4; i++) {
        draw_rect(30, 250+i*90, 70, 60, 0xFF0D0B18);
        draw_rect_outline(30, 250+i*90, 70, 60, 0xFF444466);
        draw_str(32, 278+i*90, dock_labels[i], 0xFFF5F5F5, 1);
    }
 
    /* ── Sağ panel: Uygulama çekmecesi ── */
    int bx=SW-230, bw=210;
    draw_rect(bx, 44, bw, SH-124, 0xAA211C38);
    draw_rect_outline(bx, 44, bw, SH-124, 0xFF444466);
    draw_str(bx+30, 62, "UYGULAMALAR", 0xFF00E5FF, 1);
 
    const char *apps[8]={
        "Terminal","Mesajlar","Dosyalar","Haritalar",
        "Ayarlar","Galeri","Muzik","Tarayici"
    };
    for (int i=0; i<8; i++) {
        draw_rect(bx+14, 90+i*68, bw-28, 52, 0xFF0D0B18);
        draw_rect_outline(bx+14, 90+i*68, bw-28, 52, 0xFF333355);
        draw_str(bx+24, 110+i*68, apps[i], 0xFFF5F5F5, 1);
    }
 
    /* ── Orta: Hoş geldiniz popup ── */
    int px=240, py=300, pw=380, ph=110;
    draw_rect(px, py, pw, ph, 0xFFFFFFFF);
    draw_rect_outline(px, py, pw, ph, 0xFF00E5FF);
    draw_str(px+30, py+20, "HOS GELDINIZ!", 0xFF1A1A2E, 2);
    draw_str(px+30, py+55, "Sisteme Hos Geldiniz.", 0xFF333355, 1);
    draw_str(px+30, py+75, "Wind OS v1.5 aktif.", 0xFF0078D4, 1);
 
    /* ── Alt dock ── */
    draw_rect(20, SH-72, 700, 56, 0xAA211C38);
    draw_rect_outline(20, SH-72, 700, 56, 0xFF00E5FF);
    draw_str(35, SH-52, "TUSUMANA BASINCA CEKMECE ACILSIN", 0xFFF5F5F5, 1);
}
 
/* ============================================================
 * 7. DURUM MAKİNESİ
 * ============================================================ */
typedef enum {
    STATE_REGION = 0,
    STATE_KEYBOARD,
    STATE_NETWORK,
    STATE_NAME,
    STATE_PRIVACY,
    STATE_CUSTOMIZE,
    STATE_DESKTOP
} OS_STATE;
 
static volatile OS_STATE g_state = STATE_REGION;
 
static void refresh(void)
{
    switch (g_state) {
        case STATE_REGION:    render_oobe_region();   break;
        case STATE_KEYBOARD:  render_oobe_keyboard(); break;
        case STATE_NETWORK:   render_oobe_network();  break;
        case STATE_NAME:      render_oobe_name();     break;
        case STATE_PRIVACY:   render_oobe_privacy();  break;
        case STATE_CUSTOMIZE: render_oobe_customize();break;
        case STATE_DESKTOP:   render_desktop();       break;
    }
}

// 🚀 5. ANA GİRİŞ NOKTASI
void kernel_main(void* mboot_ptr, uint32_t magic) {
 
static void go_next(void) { if (g_state < STATE_DESKTOP) { g_state++; refresh(); } }
static void go_prev(void) { if (g_state > STATE_REGION)  { g_state--; refresh(); } }
 
/* ============================================================
 * 8. KERNEL GİRİŞ NOKTASI
 * ============================================================ */
void kernel_main(void *mboot_ptr, uint32_t magic)
{
    (void)magic;
    if (mboot_ptr != NULL) {
        uint32_t flags = *(uint32_t*)mboot_ptr;
        if (flags & (1 << 11)) {
            uint32_t* vbe_mode_info = (uint32_t*)((uint8_t*)mboot_ptr + 72);
            uint32_t addr = *vbe_mode_info;
            if (addr != 0) GRAPHICS_FRAMEBUFFER = (uint32_t*)addr;
 
    /* Multiboot VBE → gerçek framebuffer adresi */
    if (mboot_ptr) {
        uint32_t flags = *(uint32_t *)mboot_ptr;
        if (flags & (1u << 11)) {
            /* Multiboot yapısında framebuffer adresi offset 88'de (dword 22) */
            uint32_t fb_addr = ((uint32_t *)mboot_ptr)[22];
            if (fb_addr) GRAPHICS_FRAMEBUFFER = (uint32_t *)fb_addr;
        }
    }

 
    is_graphics_mode = 1;
    force_graphics_hardware();
    update_sky_ui_layer();

 
    g_state = STATE_REGION;
    refresh();
 
    while (1) {
        if (sky_inb(0x64) & 1) {
            uint8_t code = sky_inb(0x60);
            if (!(code & 0x80)) {
                if (code == 0x1C) sky_process_next();
                else if (code == 0x0E) sky_process_prev();
        if (inb(0x64) & 1) {
            uint8_t sc = inb(0x60);
            if (!(sc & 0x80)) {
                if      (sc == 0x1C) go_next(); /* ENTER     */
                else if (sc == 0x0E) go_prev(); /* BACKSPACE */
            }
        }
        __asm__ volatile("pause");
    }
}

// Stublar
void idt_init(void) {}
void keyboard_init(void) {}
void mouse_init(void) {}
 
/* ============================================================
 * 9. STUB'LAR  (sadece screen.c, setup.c'de OLMAYAN fonksiyonlar)
 *    screen_init, setup_init, setup_handle_input → YAZMA, orada tanımlı!
 *    trigger_next_stage                          → YAZMA, setup.c'de var!
 * ============================================================ */
void idt_init(void)            {}
void keyboard_init(void)       {}
void mouse_init(void)          {}
void wind_subsystem_init(void) {}
void exe_subsystem_init(void) {}
void ai_subsystem_init(void) {}
void deb_subsystem_init(void) {}
void exe_subsystem_init(void)  {}
void ai_subsystem_init(void)   {}
void deb_subsystem_init(void)  {}
