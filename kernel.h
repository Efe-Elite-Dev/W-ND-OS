/*
 * Wind OS - kernel.c
 * Aga Edition - 7 Kurulum Ekranı (OOBE) + Masaüstü Tam Sürüm
 * %100 Derleme ve Eşleşme Garantili
 */

#include "kernel.h"

/* =========================================================
   TEMEL TİP TANIMLAMALARI
   ========================================================= */
typedef unsigned int   u32;
typedef unsigned short u16;
typedef unsigned char  u8;
typedef int            i32;

#define NULL ((void*)0)

/* =========================================================
   GLOBAL FRAMEBUFFER VE SİSTEM DEĞİŞKENLERİ
   ========================================================= */
static u32* FB        = (u32*)0;
static u32  SCR_W     = 1024;
static u32  SCR_H     = 768;
static u32  SCR_PITCH = 1024;

/* kernel.h içindeki orijinal enum yapısını kullanıyoruz */
static OS_State state = STATE_SETUP_1_NAME;

/* Masaüstü UI Durumları */
static u8 start_menu_open   = 0;
static u8 file_manager_open = 0;

/* =========================================================
   RENKLER
   ========================================================= */
#define C_WHITE  0xFFFFFFFFu
#define C_BLACK  0xFF000000u
#define C_BLUE   0xFF0067C0u
#define C_DARK   0xFF202020u
#define C_GRAY   0xFFE0E0E0u
#define C_GREEN  0xFF107C41u

/* =========================================================
   DONANIM PORT I/O SÜRÜCÜLERİ
   ========================================================= */
static inline void outb(u16 port, u8 val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline u8 inb(u16 port) {
    u8 ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* =========================================================
   8x8 BİTMAP FONT VERİSİ (ASCII 32 - 126)
   ========================================================= */
static const u8 font8x8[96][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* */
    {0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00}, /* ! */
    {0x6C,0x6C,0x6C,0x00,0x00,0x00,0x00,0x00}, /* " */
    {0x6C,0x6C,0xFE,0x6C,0xFE,0x6C,0x6C,0x00}, /* # */
    {0x18,0x7E,0xC0,0x7C,0x06,0xFC,0x18,0x00}, /* $ */
    {0x00,0xC6,0xCC,0x18,0x30,0x66,0xC6,0x00}, /* % */
    {0x38,0x6C,0x38,0x76,0xDC,0xCC,0x76,0x00}, /* & */
    {0x30,0x30,0x60,0x00,0x00,0x00,0x00,0x00}, /* ' */
    {0x18,0x30,0x60,0x60,0x60,0x30,0x18,0x00}, /* ( */
    {0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00}, /* ) */
    {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00}, /* * */
    {0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00}, /* + */
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30}, /* , */
    {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00}, /* - */
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00}, /* . */
    {0x00,0x03,0x06,0x0C,0x18,0x30,0x60,0x00}, /* / */
    {0x3C,0x66,0x6E,0x7E,0x76,0x66,0x3C,0x00}, /* 0 */
    {0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00}, /* 1 */
    {0x3C,0x66,0x06,0x1C,0x30,0x66,0x7E,0x00}, /* 2 */
    {0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00}, /* 3 */
    {0x06,0x0E,0x1E,0x66,0x7E,0x06,0x06,0x00}, /* 4 */
    {0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0x00}, /* 5 */
    {0x3C,0x66,0x60,0x7C,0x66,0x66,0x3C,0x00}, /* 6 */
    {0x7E,0x66,0x06,0x0C,0x18,0x18,0x18,0x00}, /* 7 */
    {0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0x00}, /* 8 */
    {0x3C,0x66,0x66,0x3E,0x06,0x66,0x3C,0x00}, /* 9 */
    {0x00,0x18,0x18,0x00,0x18,0x18,0x00,0x00}, /* : */
    {0x00,0x18,0x18,0x00,0x18,0x18,0x30,0x00}, /* ; */
    {0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x00}, /* < */
    {0x00,0x7E,0x00,0x7E,0x00,0x00,0x00,0x00}, /* = */
    {0x30,0x18,0x0C,0x06,0x0C,0x18,0x30,0x00}, /* > */
    {0x3C,0x66,0x06,0x0C,0x18,0x00,0x18,0x00}, /* ? */
    {0x3C,0x66,0x6E,0x6E,0x60,0x62,0x3C,0x00}, /* @ */
    {0x18,0x3C,0x66,0x66,0x7E,0x66,0x66,0x00}, /* A */
    {0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0x00}, /* B */
    {0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00}, /* C */
    {0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0x00}, /* D */
    {0x7E,0x60,0x60,0x78,0x60,0x60,0x7E,0x00}, /* E */
    {0x7E,0x60,0x60,0x78,0x60,0x60,0x60,0x00}, /* F */
    {0x3C,0x66,0x60,0x6E,0x66,0x66,0x3E,0x00}, /* G */
    {0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00}, /* H */
    {0x7E,0x18,0x18,0x18,0x18,0x18,0x7E,0x00}, /* I */
    {0x06,0x06,0x06,0x06,0x06,0x66,0x3C,0x00}, /* J */
    {0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0x00}, /* K */
    {0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00}, /* L */
    {0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x00}, /* M */
    {0x66,0x6E,0x76,0x7A,0x6E,0x66,0x66,0x00}, /* N */
    {0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00}, /* O */
    {0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0x00}, /* P */
    {0x3C,0x66,0x66,0x66,0x6E,0x3C,0x0E,0x00}, /* Q */
    {0x7C,0x66,0x66,0x7C,0x78,0x6C,0x66,0x00}, /* R */
    {0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00}, /* S */
    {0x7E,0x5A,0x18,0x18,0x18,0x18,0x18,0x00}, /* T */
    {0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00}, /* U */
    {0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00}, /* V */
    {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00}, /* W */
    {0x66,0x66,0x3A,0x1C,0x5C,0x66,0x66,0x00}, /* X */
    {0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00}, /* Y */
    {0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00}, /* Z */
    {0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00}, /* [ */
    {0x00,0x60,0x30,0x18,0x0C,0x06,0x03,0x00}, /* \ */
    {0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00}, /* ] */
    {0x18,0x3C,0x66,0x00,0x00,0x00,0x00,0x00}, /* ^ */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF}, /* _ */
    {0x30,0x30,0x18,0x00,0x00,0x00,0x00,0x00}, /* ` */
    {0x00,0x00,0x3C,0x06,0x3E,0x66,0x3B,0x00}, /* a */
    {0x60,0x60,0x7C,0x66,0x66,0x66,0x7C,0x00}, /* b */
    {0x00,0x00,0x3C,0x60,0x60,0x66,0x3C,0x00}, /* c */
    {0x06,0x06,0x3E,0x66,0x66,0x66,0x3B,0x00}, /* d */
    {0x00,0x00,0x3C,0x66,0x7E,0x60,0x3C,0x00}, /* e */
    {0x1C,0x30,0x78,0x30,0x30,0x30,0x30,0x00}, /* f */
    {0x00,0x00,0x3B,0x66,0x66,0x3E,0x06,0x7C}, /* g */
    {0x60,0x60,0x7C,0x66,0x66,0x66,0x66,0x00}, /* h */
    {0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00}, /* i */
    {0x06,0x00,0x0E,0x06,0x06,0x06,0x06,0x3C}, /* j */
    {0x60,0x60,0x66,0x6C,0x78,0x6C,0x66,0x00}, /* k */
    {0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00}, /* l */
    {0x00,0x00,0x6E,0x7F,0x6B,0x63,0x63,0x00}, /* m */
    {0x00,0x00,0x7C,0x66,0x66,0x66,0x66,0x00}, /* n */
    {0x00,0x00,0x3C,0x66,0x66,0x66,0x3C,0x00}, /* o */
    {0x00,0x00,0x7C,0x66,0x66,0x7C,0x60,0x60}, /* p */
    {0x00,0x00,0x3B,0x66,0x66,0x3E,0x06,0x06}, /* q */
    {0x00,0x00,0x7C,0x66,0x60,0x60,0x60,0x00}, /* r */
    {0x00,0x00,0x3E,0x60,0x3C,0x06,0x7C,0x00}, /* s */
    {0x30,0x30,0x78,0x30,0x30,0x30,0x1C,0x00}, /* t */
    {0x00,0x00,0x66,0x66,0x66,0x66,0x3B,0x00}, /* u */
    {0x00,0x00,0x66,0x66,0x66,0x3C,0x18,0x00}, /* v */
    {0x00,0x00,0x63,0x6B,0x7F,0x3E,0x36,0x00}, /* w */
    {0x00,0x00,0x66,0x3C,0x18,0x3C,0x66,0x00}, /* x */
    {0x00,0x00,0x66,0x66,0x66,0x3E,0x06,0x7C}, /* y */
    {0x00,0x00,0x7E,0x0C,0x18,0x30,0x7E,0x00}  /* z */
};

/* =========================================================
   GRAFİK VE ÇİZİM FONKSİYONLARI
   ========================================================= */
void put_pixel(i32 x, i32 y, u32 color){
    if(x < 0 || x >= (i32)SCR_W || y < 0 || y >= (i32)SCR_H) return;
    FB[y * SCR_PITCH + x] = color;
}

void fill_rect(i32 x, i32 y, i32 w, i32 h, u32 color){
    for(i32 i = 0; i < h; i++){
        for(i32 j = 0; j < w; j++){
            put_pixel(x + j, y + i, color);
        }
    }
}

void fill_rrect(i32 x, i32 y, i32 w, i32 h, i32 r, u32 color){
    for(i32 i = 0; i < h; i++){
        for(i32 j = 0; j < w; j++){
            i32 dx = 0, dy = 0;
            if(j < r) dx = r - j;
            if(j >= w - r) dx = j - (w - r - 1);
            if(i < r) dy = r - i;
            if(i >= h - r) dy = i - (h - r - 1);
            
            if(dx > 0 && dy > 0){
                if(dx * dx + dy * dy <= r * r){
                    put_pixel(x + j, y + i, color);
                }
            } else {
                put_pixel(x + j, y + i, color);
            }
        }
    }
}

void draw_char(i32 x, i32 y, char c, u32 color){
    if(c < 32 || c > 126) return;
    u32 idx = c - 32;
    for(i32 i = 0; i < 8; i++){
        u8 row = font8x8[idx][i];
        for(i32 j = 0; j < 8; j++){
            if(row & (0x80 >> j)){
                put_pixel(x + j * 2,     y + i * 2,     color);
                put_pixel(x + j * 2 + 1, y + i * 2,     color);
                put_pixel(x + j * 2,     y + i * 2 + 1, color);
                put_pixel(x + j * 2 + 1, y + i * 2 + 1, color);
            }
        }
    }
}

void draw_string(i32 x, i32 y, const char* str, u32 color){
    while(*str){
        draw_char(x, y, *str, color);
        x += 16;
        str++;
    }
}

/* =========================================================
   MOUSE VE KLAVYE SÜRÜCÜSÜ
   ========================================================= */
static i32 mouse_x = 512;
static i32 mouse_y = 384;
static u8  mouse_btn = 0;
static u8  prev_mouse_btn = 0;

void mouse_init(void) {
    outb(0x64, 0xA8);
    outb(0x64, 0x20);
    u8 status = inb(0x60) | 2;
    outb(0x64, 0x60);
    outb(0x60, status);
    
    outb(0x64, 0xD4);
    outb(0x60, 0xF4);
    inb(0x60);
}

void mouse_poll(void) {
    if((inb(0x64) & 1) == 0) return;
    if((inb(0x64) & 0x20) == 0) return; 
    
    u8 flags = inb(0x60);
    u8 dx = inb(0x60);
    u8 dy = inb(0x60);
    
    if(flags & 0xC0) return; 
    
    i32 x_offset = (i32)dx;
    i32 y_offset = (i32)dy;
    
    if(flags & 0x10) x_offset |= ~0xFF;
    if(flags & 0x20) y_offset |= ~0xFF;
    
    mouse_x += x_offset / 2;
    mouse_y -= y_offset / 2;
    
    if(mouse_x < 0) mouse_x = 0;
    if(mouse_x >= (i32)SCR_W) mouse_x = SCR_W - 1;
    if(mouse_y < 0) mouse_y = 0;
    if(mouse_y >= (i32)SCR_H) mouse_y = SCR_H - 1;
    
    prev_mouse_btn = mouse_btn;
    mouse_btn = flags & 7;
}

u8 kbd_poll(void) {
    if(inb(0x64) & 1) {
        if(!(inb(0x64) & 0x20)) {
            return inb(0x60);
        }
    }
    return 0;
}

/* =========================================================
   UI BUTON BİLEŞENİ
   ========================================================= */
i32 draw_button(i32 x, i32 y, i32 w, i32 h, const char* text, u8 is_blue, u8 click){
    i32 hover = (mouse_x >= x && mouse_x <= x + w && mouse_y >= y && mouse_y <= y + h);
    u32 bg = is_blue ? C_BLUE : C_GRAY;
    u32 fg = is_blue ? C_WHITE : C_BLACK;
    
    if(hover) bg = is_blue ? 0xFF0078D7u : C_WHITE;
    
    fill_rrect(x, y, w, h, 4, bg);
    
    i32 text_w = 0;
    const char* t = text;
    while(*t++) text_w += 16;
    draw_string(x + (w - text_w) / 2, y + (h - 16) / 2, text, fg);
    
    return (hover && click);
}

/* =========================================================
   OOBE ANA ŞABLON RENDERER
   ========================================================= */
void draw_setup_bg(const char* title, i32 step) {
    fill_rect(0, 0, (i32)SCR_W, (i32)SCR_H, 0xFFF0F0F0u);
    fill_rect(0, 0, 80, (i32)SCR_H, C_BLUE);
    draw_string(120, 60, title, C_BLACK);
    
    /* İlerleme Çubuğu (7 Adım Üzerinden Hesaplama) */
    fill_rrect(120, 100, 280, 6, 3, 0xFFDDDDDDu);
    fill_rrect(120, 100, (280 * step) / 7, 6, 3, C_BLUE);
}

/* =========================================================
   7 ADET KURULUM EKRANI FONKSİYONLARI
   ========================================================= */
static char username[32] = "Efe";
static i32  user_len = 3;

void screen_setup_1(u8 key, u8 click) {
    draw_setup_bg("Asama 1: Bilgisayariniza bir ad verin", 1);
    draw_string(120, 160, "Cihaziniza benzersiz bir isim verin:", C_DARK);
    
    fill_rrect(120, 200, 400, 40, 4, C_WHITE);
    if(key >= 32 && key <= 126 && user_len < 30){
        username[user_len++] = key;
        username[user_len] = '\0';
    } else if(key == 0x0E && user_len > 0){ /* Backspace */
        username[--user_len] = '\0';
    }
    draw_string(130, 212, username, C_BLACK);
    fill_rect(130 + user_len * 16, 210, 2, 20, C_BLUE);
    
    if(draw_button(420, 290, 100, 40, "Ileri", 1, click)) {
        state = STATE_SETUP_2_REGION;
    }
}

void screen_setup_2(u8 click) {
    draw_setup_bg("Asama 2: Bu dogru ulke/bolge mi?", 2);
    
    fill_rrect(120, 160, 400, 130, 4, C_WHITE);
    fill_rect(120, 160, 400, 40, C_BLUE);
    draw_string(140, 172, "Turkiye (Secili)", C_WHITE);
    draw_string(140, 215, "Azerbaycan", C_DARK);
    draw_string(140, 255, "United Kingdom", C_DARK);
    
    if(draw_button(420, 320, 100, 40, "Evet", 1, click)) {
        state = STATE_SETUP_3_KEYBOARD;
    }
}

void screen_setup_3(u8 click) {
    draw_setup_bg("Asama 3: Bu dogru klavye duzeni?", 3);
    
    fill_rrect(120, 160, 400, 130, 4, C_WHITE);
    fill_rect(120, 160, 400, 40, C_BLUE);
    draw_string(140, 172, "Turkce Q (Secili)", C_WHITE);
    draw_string(140, 215, "Turkce F", C_DARK);
    draw_string(140, 255, "English US", C_DARK);
    
    if(draw_button(420, 320, 100, 40, "Evet", 1, click)) {
        state = STATE_SETUP_4_NETWORK;
    }
}

void screen_setup_4(u8 click) {
    draw_setup_bg("Asama 4: Hadi sizi bir aga baglayalim", 4);
    draw_string(120, 160, "Baglantilar denetleniyor...", C_DARK);
    
    fill_rrect(120, 200, 400, 60, 4, C_WHITE);
    draw_string(140, 222, "[Bagli] Yerel Ethernet Agi", C_GREEN);
    
    if(draw_button(420, 290, 100, 40, "Ileri", 1, click)) {
        state = STATE_SETUP_5_PRIVACY;
    }
}

void screen_setup_5(u8 click) {
    draw_setup_bg("Asama 5: Gizlilik ayarlarini secin", 5);
    
    fill_rrect(120, 160, 500, 160, 4, C_WHITE);
    draw_string(140, 180, "[X] Konum Hizmetleri (Acik)", C_DARK);
    draw_string(140, 220, "[X] Diagnostik Verileri (Gonderiliyor)", C_DARK);
    draw_string(140, 260, "[X] Reklam Kimligi (Etkin)", C_DARK);
    
    if(draw_button(520, 350, 100, 40, "Kabul Et", 1, click)) {
        state = STATE_SETUP_6_CUSTOMIZE;
    }
}

void screen_setup_6(u8 click) {
    draw_setup_bg("Asama 6: Deneyiminizi ozellestirin", 6);
    draw_string(120, 150, "Cihazi ne amacla kullanacaksiniz?", C_DARK);
    
    fill_rrect(120, 190, 400, 130, 4, C_WHITE);
    fill_rect(120, 190, 400, 40, C_BLUE);
    draw_string(140, 202, "Yazilim ve Gelistirme (Secildi)", C_WHITE);
    draw_string(140, 245, "Oyun / Gaming", C_DARK);
    draw_string(140, 285, "Okul / Is", C_DARK);
    
    if(draw_button(420, 350, 100, 40, "Atla", 1, click)) {
        state = STATE_SETUP_7_WELCOME;
    }
}

void screen_setup_7(u8 click) {
    draw_setup_bg("Asama 7: Hos Geldiniz", 7);
    
    draw_string(120, 170, "Her sey harika sekilde ayarlandi!", C_GREEN);
    draw_string(120, 210, "Kullanici Profiliniz: ", C_DARK);
    draw_string(340, 210, username, C_BLUE);
    
    if(draw_button(120, 280, 220, 45, "Sisteme Giris Yap", 1, click)) {
        state = STATE_DESKTOP;
    }
}

/* =========================================================
   8. EKRAN: MASAÜSTÜ & BAŞLAT MENÜSÜ & DOSYA YÖNETİCİSİ
   ========================================================= */
void screen_desktop(u8 click) {
    /* Arkaplan Gradyanı çizimi */
    for(i32 y = 0; y < (i32)SCR_H; y++) {
        u32 g = 103 + (y * 50) / (i32)SCR_H;
        u32 b = 192 + (y * 50) / (i32)SCR_H;
        fill_rect(0, y, (i32)SCR_W, 1, 0xFF000000u | (g << 8) | b);
    }
    
    /* Görev Çubuğu (Taskbar) */
    i32 tb_h = 45;
    i32 tb_y = (i32)SCR_H - tb_h;
    fill_rect(0, tb_y, (i32)SCR_W, tb_h, 0xFF101010u);
    
    /* Başlat Butonu */
    i32 btn_hover = (mouse_x >= 0 && mouse_x <= 100 && mouse_y >= tb_y);
    u32 btn_color = btn_hover ? 0xFF0078D7u : C_BLUE;
    fill_rect(0, tb_y, 100, tb_h, btn_color);
    draw_string(15, tb_y + 14, "WindOS", C_WHITE);
    
    if(btn_hover && click) {
        start_menu_open = !start_menu_open;
    }
    
    /* Sağ Köşe Saat & Hava Durumu Göstergesi */
    draw_string((i32)SCR_W - 220, tb_y + 14, "24C Gunesli | 12:00", C_WHITE);
    
    /* BAŞLAT MENÜSÜ ÇEKMECE */
    if(start_menu_open) {
        i32 menu_w = 240;
        i32 menu_h = 220;
        i32 menu_x = 0;
        i32 menu_y = tb_y - menu_h;
        
        fill_rrect(menu_x, menu_y, menu_w, menu_h, 0, 0xFFEBEBEBu);
        fill_rect(menu_x, menu_y, 4, menu_h, C_BLUE);
        
        draw_string(15, menu_y + 15, username, C_BLACK);
        fill_rect(10, menu_y + 40, menu_w - 20, 1, 0xFFCCCCCCu);
        
        /* Dosya Yöneticisi Elemanı */
        i32 item_hover = (mouse_x >= 10 && mouse_x <= menu_w - 10 && mouse_y >= menu_y + 50 && mouse_y <= menu_y + 90);
        u32 item_bg = item_hover ? 0xFFCCCCCCu : 0xFFEBEBEBu;
        fill_rrect(10, menu_y + 50, menu_w - 20, 40, 4, item_bg);
        draw_string(20, menu_y + 62, "Dosya Yoneticisi", C_BLACK);
        
        if(item_hover && click) {
            file_manager_open = 1;
            start_menu_open = 0; 
        }
    }
    
    /* DOSYA YÖNETİCİSİ PENCERESİ */
    if(file_manager_open) {
        i32 wx = 150, wy = 100, ww = 600, wh = 400;
        fill_rrect(wx, wy, ww, wh, 6, C_WHITE);
        
        /* Pencere Başlığı */
        fill_rrect(wx, wy, ww, 35, 6, C_BLUE);
        fill_rect(wx, wy + 15, ww, 20, C_BLUE);
        draw_string(wx + 15, wy + 10, "Dosya Yoneticisi", C_WHITE);
        
        /* Kapatma Butonu (X) */
        i32 close_hover = (mouse_x >= wx + ww - 40 && mouse_x <= wx + ww - 5 && mouse_y >= wy + 5 && mouse_y <= wy + 30);
        u32 close_bg = close_hover ? 0xFFE81123u : C_BLUE;
        fill_rect(wx + ww - 40, wy + 5, 35, 25, close_bg);
        draw_string(wx + ww - 30, wy + 10, "X", C_WHITE);
        
        if(close_hover && click) {
            file_manager_open = 0;
        }
        
        /* Dizin İçeriği */
        draw_string(wx + 20, wy + 60, "Konum: C:\\WindOS_Sistem", C_DARK);
        fill_rect(wx + 15, wy + 85, ww - 30, 1, 0xFFEEEEEEu);
        
        draw_string(wx + 20, wy + 110, "[Klasor] Belgelerim", C_BLACK);
        draw_string(wx + 20, wy + 140, "[Klasor] Projeler", C_BLACK);
        draw_string(wx + 20, wy + 170, "[Dosya]  kernel.bin", C_BLACK);
        draw_string(wx + 20, wy + 200, "[Dosya]  system_config.sys", C_BLACK);
    }
}

/* =========================================================
   MOUSE GÖRSELİ ÇİZİCİ
   ========================================================= */
void draw_mouse(void){
    i32 mx = mouse_x;
    i32 my = mouse_y;
    for(i32 i=0; i<14; i++){
        for(i32 j=0; j<=i; j++){
            if(j < 8) {
                put_pixel(mx + j, my + i, C_BLACK);
            }
        }
    }
    put_pixel(mx, my, C_WHITE);
}

/* =========================================================
   KERNEL_MAIN (ANA GİRİŞ NOKTASI)
   ========================================================= */
void kernel_main(multiboot_info_t* mbi){
    /* GRUB'dan Framebuffer verilerini güvenli şekilde bağla */
    if(mbi && mbi->framebuffer_addr) {
        FB        = (u32*)(unsigned long)mbi->framebuffer_addr;
        SCR_W     = mbi->framebuffer_width;
        SCR_H     = mbi->framebuffer_height;
        SCR_PITCH = mbi->framebuffer_pitch / 4;
    } else {
        FB        = (u32*)0xFD000000u;
        SCR_W     = 1024;
        SCR_H     = 768;
        SCR_PITCH = 1024;
    }

    mouse_init();

    while(1){
        mouse_poll();
        u8 key = kbd_poll();
        u8 click = (mouse_btn & 1) && !(prev_mouse_btn & 1);

        /* kernel.h dosyasındaki 8 durumun tamamı eksiksiz yönetiliyor */
        switch(state) {
            case STATE_SETUP_1_NAME:
                screen_setup_1(key, click);
                break;
            case STATE_SETUP_2_REGION:
                screen_setup_2(click);
                break;
            case STATE_SETUP_3_KEYBOARD:
                screen_setup_3(click);
                break;
            case STATE_SETUP_4_NETWORK:
                screen_setup_4(click);
                break;
            case STATE_SETUP_5_PRIVACY:
                screen_setup_5(click);
                break;
            case STATE_SETUP_6_CUSTOMIZE:
                screen_setup_6(click);
                break;
            case STATE_SETUP_7_WELCOME:
                screen_setup_7(click);
                break;
            case STATE_DESKTOP:
                screen_desktop(click);
                break;
        }

        draw_mouse();

        /* CPU koruma gecikmesi */
        for(volatile int delay=0; delay<15000; delay++) {
            __asm__ volatile("nop");
        }
    }
}
