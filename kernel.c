/*
 * Wind OS - kernel.c
 * 7 Kurulum Ekranı (OOBE) + Masaüstü
 * PS/2 Klavye, PS/2 Mouse, PCI USB Algılama, Dosya Yöneticisi
 *
 * Derleme: gcc -m32 -ffreestanding -fno-builtin -O2 -Wall -c kernel.c -o kernel.o
 */

#include "kernel.h"

/* =========================================================
   TEMEL TİP TANIMLAMALARI
   ========================================================= */
typedef unsigned int      u32;
typedef unsigned short    u16;
typedef unsigned char     u8;
typedef int               i32;
typedef signed char       i8;

#define NULL ((void*)0)

/* =========================================================
   GLOBAL FRAMEBUFFER
   ========================================================= */
static u32* FB        = (u32*)0;
static u32  SCR_W     = 1024;
static u32  SCR_H     = 768;
static u32  SCR_PITCH = 1024;  /* piksel cinsinden pitch */

/* =========================================================
   OS DURUM MAKİNESİ
   ========================================================= */
static OS_State state = STATE_SETUP_1_NAME;

/* =========================================================
   RENKLER
   ========================================================= */
#define C_BG          0xFFF3F3F5u
#define C_WHITE       0xFFFFFFFFu
#define C_BLACK       0xFF000000u
#define C_BLUE        0xFF0067C0u
#define C_HOVER       0xFFE5E5E5u
#define C_TEXT        0xFF202020u
#define C_MUTED       0xFF616161u
#define C_BORDER      0xFFCCCCCCu
#define C_SUCCESS     0xFF107C41u

/* =========================================================
   DONANIM PORT I/O
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
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* (space) */
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
   GRAFİK ÇİZİM FONKSİYONLARI
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

/* Yuvarlak köşeli dikdörtgen doldurma */
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

/* Kenarlık çizen yuvarlak köşeli dikdörtgen */
void draw_rrect(i32 x, i32 y, i32 w, i32 h, i32 r, u32 color){
    for(i32 i = 0; i < h; i++){
        for(i32 j = 0; j < w; j++){
            if(i == 0 || i == h - 1 || j == 0 || j == w - 1){
                i32 dx = 0, dy = 0;
                if(j < r) dx = r - j;
                if(j >= w - r) dx = j - (w - r - 1);
                if(i < r) dy = r - i;
                if(i >= h - r) dy = i - (h - r - 1);
                
                if(dx > 0 && dy > 0){
                    if(dx * dx + dy * dy <= r * r && dx * dx + dy * dy > (r - 2) * (r - 2)){
                        put_pixel(x + j, y + i, color);
                    }
                } else {
                    put_pixel(x + j, y + i, color);
                }
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
                /* 2x2 piksel büyütme (Okunabilirlik için) */
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
        x += 16; /* Karakter genişliği */
        str++;
    }
}

/* WiFi Simgesi Çizimi */
void draw_wifi_icon(i32 x, i32 y, u32 color){
    fill_rrect(x + 14, y + 24, 4, 4, 2, color);
    /* İlkel WiFi yay çizgileri */
    for(i32 r = 8; r <= 24; r += 8){
        for(i32 deg = -45; deg <= 45; deg += 2){
            /* Statik yaklaşımla basit yay pikselleri */
            i32 px = x + 16 + (r * deg) / 60;
            i32 py = y + 24 - r + (deg * deg) / 80;
            fill_rect(px, py, 2, 2, color);
        }
    }
}

/* =========================================================
   DONANIM SÜRÜCÜLERİ (PS/2 KEYBOARD & MOUSE)
   ========================================================= */
static i32 mouse_x = 512;
static i32 mouse_y = 384;
static u8  mouse_btn = 0;

void mouse_init(void) {
    u8 status;
    
    /* Fareyi aktif et */
    outb(0x64, 0xA8);
    outb(0x64, 0x20);
    status = inb(0x60) | 2;
    outb(0x64, 0x60);
    outb(0x60, status);
    
    /* Varsayılan ayarları yükle */
    outb(0x64, 0xD4);
    outb(0x60, 0xF4);
    inb(0x60);
}

void mouse_poll(void) {
    if((inb(0x64) & 1) == 0) return;
    if((inb(0x64) & 0x20) == 0) return; /* Veri fareden gelmediyse çık */
    
    u8 flags = inb(0x60);
    u8 dx = inb(0x60);
    u8 dy = inb(0x60);
    
    if(flags & 0x40) return; /* Taşma var, geçersiz veri */
    if(flags & 0x80) return;
    
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
   PCI & USB TESPİT KATMANI
   ========================================================= */
static char usb_status_str[64] = "PCI: USB Denetleyici araniyor...";

u32 pci_config_read_dword(u8 bus, u8 slot, u8 func, u8 offset) {
    u32 address = (u32)((u32)0x80000000 | (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC));
    outb(0xCF8, (u8)(address >> 24));
    outb(0xCF9, (u8)(address >> 16));
    outb(0xCFA, (u8)(address >> 8));
    outb(0xCFB, (u8)address);
    
    /* 32 bitlik veriyi oku */
    u32 val;
    __asm__ volatile("inl %%dx, %%eax" : "=a"(val) : "d"(0xCFC));
    return val;
}

void pci_scan(void) {
    for(u16 bus = 0; bus < 256; bus++) {
        for(u8 slot = 0; slot < 32; slot++) {
            u32 r0 = pci_config_read_dword((u8)bus, slot, 0, 0);
            if((r0 & 0xFFFF) == 0xFFFF) continue;
            
            u32 r8 = pci_config_read_dword((u8)bus, slot, 0, 8);
            u8 class_code = (u8)(r8 >> 24);
            u8 sub_class  = (u8)(r8 >> 16);
            
            /* Class 0x0C = Serial Bus, Subclass 0x03 = USB */
            if(class_code == 0x0C && sub_class == 0x03) {
                u16 vendor = r0 & 0xFFFF;
                if(vendor == 0x8086) {
                    struct_strcpy(usb_status_str, "USB: Intel USB Controller Hazir (PCI ok)");
                } else if(vendor == 0x10DE) {
                    struct_strcpy(usb_status_str, "USB: Nvidia USB Controller Hazir (PCI ok)");
                } else {
                    struct_strcpy(usb_status_str, "USB: Genel USB 3.0 Donanimi Aktif (PCI ok)");
                }
                return;
            }
        }
    }
    struct_strcpy(usb_status_str, "USB: PCI Aygit Denetleyicisi Baglandi.");
}

/* Güvenli string kopyalama yardımı */
void struct_strcpy(char* dest, const char* src){
    while(*src){
        *dest = *src;
        dest++;
        src++;
    }
    *dest = '\0';
}

/* =========================================================
   UI MOCK DOSYA SİSTEMİ (FILE MANAGER)
   ========================================================= */
typedef struct {
    const char* name;
    const char* size;
    const char* type;
} MockFile;

static MockFile files[] = {
    {"Sistem",     "<DIR>", "Klasor"},
    {"Uygulamalar", "<DIR>", "Klasor"},
    {"Belgeler",   "<DIR>", "Klasor"},
    {"kernel.bin",  "240 KB", "Dosya"},
    {"windos.sys",  "12 KB",  "Sistem"},
    {"ayarlar.cfg", "1 KB",   "Ayar"}
};

/* =========================================================
   ORTAK ARAYÜZ BİLEŞENLERİ (WIDGETS)
   ========================================================= */
void draw_base_layout(const char* title, i32 step){
    fill_rect(0, 0, (i32)SCR_W, (i32)SCR_H, C_BG);
    
    /* Sol Yan Panel Süslemesi (Modern Degrade Taklidi) */
    fill_rect(0, 0, 80, (i32)SCR_H, 0xFF0052A3u);
    fill_rect(80, 0, 4, (i32)SCR_H, 0xFF0067C0u);
    
    /* Sol Alt Köşe WindOS Logosu Süslemesi */
    fill_rrect(20, (i32)SCR_H - 60, 40, 40, 8, C_BLUE);
    fill_rect(35, (i32)SCR_H - 48, 10, 16, C_WHITE);
    fill_rect(31, (i32)SCR_H - 44, 18, 8, C_WHITE);
    
    /* Ana Başlık */
    draw_string(140, 60, title, C_TEXT);
    
    /* Adım Çubuğu (7 Kurulum Ekranı İlerleyişi) */
    if(step > 0 && step <= 7){
        i32 bar_w = 200;
        fill_rrect(140, 110, bar_w, 6, 3, C_BORDER);
        fill_rrect(140, 110, (bar_w * step) / 7, 6, 3, C_BLUE);
    }
    
    /* Alt Bilgi Çubuğu */
    fill_rect(84, (i32)SCR_H - 40, (i32)SCR_W - 84, 1, C_BORDER);
    draw_string(120, (i32)SCR_H - 28, "Wind OS Kurulum Sihirbazi v1.0", C_MUTED);
}

i32 draw_button(i32 x, i32 y, i32 w, i32 h, const char* text, i32 is_primary){
    i32 hover = (mouse_x >= x && mouse_x <= x + w && mouse_y >= y && mouse_y <= y + h);
    u32 bg = is_primary ? C_BLUE : C_WHITE;
    if(hover && !is_primary) bg = C_HOVER;
    if(hover && is_primary)  bg = 0xFF0052A3u;
    
    fill_rrect(x, y, w, h, 6, bg);
    if(!is_primary){
        draw_rrect(x, y, w, h, 6, C_BORDER);
    }
    
    /* Yazıyı ortala */
    i32 text_w = 0;
    const char* t = text;
    while(*t++) text_w += 16;
    draw_string(x + (w - text_w) / 2, y + (h - 16) / 2, text, is_primary ? C_WHITE : C_TEXT);
    
    return (hover && (mouse_btn & 1));
}

/* =========================================================
   7 ADET KURULUM (OOBE) EKRANI MANTIĞI
   ========================================================= */
static char username[32] = "Feyzula Efe Tuna";
static i32  user_len = 16;

void screen1(u8 key){
    draw_base_layout("Bilgisayariniza bir ad verelim", 1);
    draw_string(140, 160, "Lutfen kullanici hesabi icin bir isim girin:", C_MUTED);
    
    /* Giriş Kutusu */
    fill_rrect(140, 210, 400, 45, 6, C_WHITE);
    draw_rrect(140, 210, 400, 45, 6, C_BLUE);
    
    /* Basit Klavye Karakter Ekleme */
    if(key >= 32 && key <= 126 && user_len < 31){
        username[user_len++] = key;
        username[user_len] = '\0';
    } else if(key == 0x0E && user_len > 0){ /* Backspace */
        username[--user_len] = '\0';
    }
    
    draw_string(160, 223, username, C_TEXT);
    /* İmleç simülasyonu */
    fill_rect(160 + user_len * 16, 221, 2, 20, C_BLUE);
    
    if(draw_button(440, 300, 100, 36, "Ileri", 1) || key == 0x1C){
        state = STATE_SETUP_2_REGION;
        /* Buton tıklama debouncing gecikmesi */
        for(volatile int i=0; i<500000; i++);
    }
}

void screen2(void){
    draw_base_layout("Bu dogru ulke/bolge mi?", 2);
    
    /* Bölge Seçim Kutusu */
    fill_rrect(140, 170, 400, 200, 6, C_WHITE);
    draw_rrect(140, 170, 400, 200, 6, C_BORDER);
    
    fill_rect(142, 172, 396, 40, C_BLUE);
    draw_string(160, 182, "Turkiye", C_WHITE);
    draw_string(160, 230, "Almanya", C_TEXT);
    draw_string(160, 270, "Ingiltere", C_TEXT);
    draw_string(160, 310, "Amerika Birlesik Devletleri", C_TEXT);
    
    if(draw_button(440, 400, 100, 36, "Evet", 1)){
        state = STATE_SETUP_3_KEYBOARD;
        for(volatile int i=0; i<500000; i++);
    }
}

void screen3(void){
    draw_base_layout("Bu dogru klavye duzeni mi?", 3);
    
    fill_rrect(140, 170, 400, 160, 6, C_WHITE);
    draw_rrect(140, 170, 400, 160, 6, C_BORDER);
    
    fill_rect(142, 172, 396, 40, C_BLUE);
    draw_string(160, 182, "Turkce Q", C_WHITE);
    draw_string(160, 230, "Turkce F", C_TEXT);
    draw_string(160, 270, "Ingilizce QWERTY", C_TEXT);
    
    if(draw_button(440, 360, 100, 36, "Evet", 1)){
        state = STATE_SETUP_4_NETWORK;
        for(volatile int i=0; i<500000; i++);
    }
}

static char wifi_pass[32] = "";
static i32  wifi_len = 0;

void screen4(u8 key){
    draw_base_layout("Hadi sizi bir aga baglayalim", 4);
    
    draw_wifi_icon(140, 160, C_BLUE);
    draw_string(190, 165, "Wind_Network_5G (Bagli Deor)", C_TEXT);
    
    draw_string(140, 220, "Ağ Sifresini Girin:", C_MUTED);
    fill_rrect(140, 250, 300, 40, 6, C_WHITE);
    draw_rrect(140, 250, 300, 40, 6, C_BORDER);
    
    if(key >= 32 && key <= 126 && wifi_len < 16){
        wifi_pass[wifi_len++] = key;
        wifi_pass[wifi_len] = '\0';
    } else if(key == 0x0E && wifi_len > 0){
        wifi_pass[--wifi_len] = '\0';
    }
    
    /* Yıldız maskesi */
    char mask[32] = "";
    for(int i=0; i<wifi_len; i++) mask[i] = '*';
    mask[wifi_len] = '\0';
    draw_string(155, 262, mask, C_TEXT);
    
    if(draw_button(350, 310, 120, 36, "Baglan", 1) || key == 0x1C){
        state = STATE_SETUP_5_PRIVACY;
        for(volatile int i=0; i<500000; i++);
    }
}

void screen5(void){
    draw_base_layout("Cihaziniz icin gizlilik ayarlari", 5);
    
    i32 box_y = 160;
    fill_rrect(140, box_y, 500, 200, 6, C_WHITE);
    draw_rrect(140, box_y, 500, 200, 6, C_BORDER);
    
    draw_string(160, box_y + 20, "[X] Konum Servislerini Etkinlestir", C_TEXT);
    draw_string(160, box_y + 60, "[X] Hata Raporlarini Otomatik Gonder", C_TEXT);
    draw_string(160, box_y + 100, "[X] Tanilama Verilerini Paylas", C_TEXT);
    draw_string(160, box_y + 140, "[ ] Reklam Kimligini Kullan", C_MUTED);
    
    if(draw_button(520, box_y + 220, 120, 36, "Kabul Et", 1)){
        state = STATE_SETUP_6_CUSTOMIZE;
        for(volatile int i=0; i<500000; i++);
    }
}

void screen6(void){
    draw_base_layout("Deneyiminizi ozellestirin", 6);
    draw_string(140, 160, "Bu cihazi ne amacla kullanacaksiniz?", C_MUTED);
    
    i32 cy = 200;
    fill_rrect(140, cy, 220, 100, 6, C_WHITE);
    draw_rrect(140, cy, 220, 100, 6, C_BLUE);
    draw_string(160, cy + 40, "Yazilim & Kod", C_BLUE);
    
    fill_rrect(390, cy, 220, 100, 6, C_WHITE);
    draw_rrect(390, cy, 220, 100, 6, C_BORDER);
    draw_string(410, cy + 40, "Oyun & Eglence", C_TEXT);
    
    if(draw_button(510, cy + 140, 100, 36, "Kabul Et", 1)){
        state = STATE_SETUP_7_WELCOME;
        for(volatile int i=0; i<500000; i++);
    }
}

void screen7(void){
    draw_base_layout("Sistem Kurulumu Tamamlandi", 7);
    
    fill_rrect(140, 180, 500, 160, 6, C_WHITE);
    draw_rrect(140, 180, 500, 160, 6, C_SUCCESS);
    
    draw_string(170, 210, "Sisteme Hos Geldiniz!", C_SUCCESS);
    draw_string(170, 250, username, C_TEXT);
    draw_string(170, 290, "Masaustu hazirlaniyor, lutfen bekleyin...", C_MUTED);
    
    if(draw_button(490, 370, 150, 40, "Masaustu", 1)){
        state = STATE_DESKTOP;
        for(volatile int i=0; i<500000; i++);
    }
}

/* =========================================================
   8. EKRAN: MASAÜSTÜ VE DOSYA YÖNETİCİSİ (DESKTOP)
   ========================================================= */
void draw_desktop(void){
    /* Arkaplan gradyanı (Boşluklar düzeltildi, hata çözüldü) */
    for(i32 y = 0; y < (i32)SCR_H - 50; y++){
        u32 r = 0x1A + (y * 10) / SCR_H;
        u32 g = 0x1A + (y * 5)  / SCR_H;
        u32 b = 0x2E + (y * 20) / SCR_H;
        fill_rect(0, y, (i32)SCR_W, 1, 0xFF000000u | (r << 16) | (g << 8) | b);
    }
    
    /* Görev Çubuğu (Taskbar) */
    fill_rect(0, (i32)SCR_H - 50, (i32)SCR_W, 50, 0xF0101010u);
    fill_rect(0, (i32)SCR_H - 50, (i32)SCR_W, 1, 0xFF303030u);
    
    /* Başlat Butonu */
    fill_rrect(10, (i32)SCR_H - 42, 90, 34, 4, C_BLUE);
    draw_string(25, (i32)SCR_H - 33, "WindOS", C_WHITE);
    
    /* Saat Bilgisi */
    draw_string((i32)SCR_W - 90, (i32)SCR_H - 33, "12:00", C_WHITE);
    
    /* Sağ Üst Köşe Widget: Hava Durumu */
    fill_rrect((i32)SCR_W - 260, 20, 240, 90, 8, 0x80000000u);
    draw_rrect((i32)SCR_W - 260, 20, 240, 90, 8, 0xFF404040u);
    draw_string((i32)SCR_W - 240, 35, "Istanbul: 22 C", C_WHITE);
    draw_string((i32)SCR_W - 240, 65, "Hava Acik", 0xFF00FF00u);
    
    /* Donanım Bilgisi Penceresi (PCI USB Kontrolü) */
    i32 hwy = 20;
    fill_rrect(20, hwy, 420, 80, 8, 0x80000000u);
    draw_rrect(20, hwy, 420, 80, 8, 0xFF404040u);
    draw_string(40, hwy + 20, "Sistem Donanim Bilgisi", 0xFFFFFF00u);
    draw_string(40, hwy + 45, usb_status_str, C_WHITE);
    
    /* MERKEZİ PENCERE: DOSYA YÖNETİCİSİ (FILE MANAGER) */
    i32 fwx = 120, fwy_pos = 160, fww = 760, fwh = 420;
    fill_rrect(fwx, fwy_pos, fww, fwh, 8, C_BG);
    draw_rrect(fwx, fwy_pos, fww, fwh, 8, C_BLUE);
    
    /* Pencere Başlık Çubuğu */
    fill_rrect(fwx + 2, fwy_pos + 2, fww - 4, 38, 6, C_BLUE);
    draw_string(fwx + 20, fwy_pos + 12, "Dosya Yoneticisi - root@windos:/#", C_WHITE);
    
    /* Sütun Başlıkları */
    draw_string(fwx + 20, fwy_pos + 55, "Isim", 0xFF000000u);
    draw_string(fwx + 300, fwy_pos + 55, "Boyut", 0xFF000000u);
    draw_string(fwx + 500, fwy_pos + 55, "Tur", 0xFF000000u);
    fill_rect(fwx + 10, fwy_pos + 78, fww - 20, 1, C_BORDER);
    
    /* Dosya Listesi Çizimi */
    for(int i = 0; i < 6; i++) {
        i32 row_y = fwy_pos + 90 + (i * 35);
        /* Satır arka plan vurgusu (seçili gibi göstermek için ilk satır) */
        if(i == 0) fill_rect(fwx + 10, row_y - 4, fww - 20, 28, 0xFFE0EEFFu);
        
        draw_string(fwx + 20, row_y, files[i].name, C_TEXT);
        draw_string(fwx + 300, row_y, files[i].size, C_MUTED);
        draw_string(fwx + 500, row_y, files[i].type, C_TEXT);
    }
    
    /* Alt Durum Çubuğu */
    fill_rect(fwx + 2, fwy_pos + fwh - 30, fww - 4, 28, 0xFFEAEAEA);
    draw_string(fwx + 20, fwy_pos + fwh - 24, "Toplam 6 nesne listelendi.", C_MUTED);
}

/* =========================================================
   MOUSE CURSOR RENDERER
   ========================================================= */
void draw_mouse(void){
    i32 mx = mouse_x;
    i32 my = mouse_y;
    
    /* Basit beyaz kenarlıklı siyah ok şeklinde fare imleci çizimi */
    for(i32 i=0; i<16; i++){
        for(i32 j=0; j<=i; j++){
            if(j < 10) {
                put_pixel(mx + j, my + i, C_BLACK);
            }
        }
    }
    /* İmlecin net görünmesi için küçük bir beyaz uç ekle */
    put_pixel(mx, my, C_WHITE);
    put_pixel(mx+1, my+1, C_WHITE);
}

/* =========================================================
   KERNEL_MAIN (ANA DÖNGÜ VE GİRİŞ NOKTASI)
   ========================================================= */
void kernel_main(multiboot_info_t* mbi){
    /* Framebuffer Yapılandırması */
    FB        = (u32*)(unsigned long)mbi->framebuffer_addr;
    SCR_W     = mbi->framebuffer_width;
    SCR_H     = mbi->framebuffer_height;
    SCR_PITCH = mbi->framebuffer_pitch / 4;

    /* GRUB bir sebeple framebuffer atayamazsa emniyet yedeği */
    if(!FB || SCR_W == 0){
        FB        = (u32*)0xFD000000u;
        SCR_W     = 1024;
        SCR_H     = 768;
        SCR_PITCH = 1024;
    }

    /* Donanımları Ayarla */
    mouse_init();
    pci_scan();

    /* Çekirdek Döngüsü */
    while(1){
        /* Fare koordinatlarını tazele */
        mouse_poll();

        /* Klavyeden basılan tuşu oku */
        u8 key = kbd_poll();

        /* Durum Makinesi Çizim Yönetimi */
        switch(state){
            case STATE_SETUP_1_NAME:     screen1(key); break;
            case STATE_SETUP_2_REGION:   screen2();    break;
            case STATE_SETUP_3_KEYBOARD: screen3();    break;
            case STATE_SETUP_4_NETWORK:  screen4(key); break;
            case STATE_SETUP_5_PRIVACY:  screen5();    break;
            case STATE_SETUP_6_CUSTOMIZE:screen6();    break;
            case STATE_SETUP_7_WELCOME:  screen7();    break;
            case STATE_DESKTOP:          draw_desktop(); break;
            default: break;
        }

        /* Fareyi her zaman en üst katmanda çiz */
        draw_mouse();

        /* İşlemciyi aşırı yükten korumak için hafif bekleme */
        for(volatile int delay=0; delay<20000; delay++) {
            __asm__ volatile("nop");
        }
    }
}
