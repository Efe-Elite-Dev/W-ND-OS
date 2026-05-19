/**
 * ==============================================================================
 * WIND OS / SKY CORE OS v1.5 - Vortex Kernel (Tam Sürüm)
 * ==============================================================================
 * Mimari   : x86 Intel/AMD IA-32 Korumalı Mod
 * Hedef    : 1024x768 x 32bpp VBE Linear Framebuffer
 * Geliştirici: Feyzula Efe Tuna
 * ==============================================================================
 *
 * LINKER KURALLARI:
 *  - GRAPHICS_FRAMEBUFFER : burada tanımlı → kerror.c / vga_force.c extern kullanır
 *  - is_graphics_mode     : burada tanımlı → vga_force.c extern kullanır
 *  - screen_init          : screen.c'de    → buraya YAZMA
 *  - setup_init           : setup.c'de     → buraya YAZMA
 *  - setup_handle_input   : setup.c'de     → buraya YAZMA
 *  - trigger_next_stage   : setup.c'de     → buraya YAZMA
 * ==============================================================================
 */
 
#include <stdint.h>
#include <stddef.h>
 
/* ============================================================
 * 1. GLOBAL DEĞİŞKENLER
 * ============================================================ */
uint32_t* GRAPHICS_FRAMEBUFFER = (uint32_t*)0xFD000000;
int       is_graphics_mode     = 1;
 
static int mouse_x = 512;
static int mouse_y = 384;
 
/* ============================================================
 * 2. DIŞ BAĞLANTI BİLDİRİMLERİ
 * ============================================================ */
extern void force_graphics_hardware(void);
extern void screen_init(void);
extern void setup_init(void);
extern void setup_handle_input(uint8_t sc);
 
/* ============================================================
 * 3. DONANIM I/O
 * ============================================================ */
static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
 
static inline void outb(uint16_t port, uint8_t val)
{
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}
 
void print_string(const char *str) { (void)str; }
 
#define SW 1024
#define SH 768
 
/* ============================================================
 * 4. GRAFİK ÇİZİM MOTORU
 * ============================================================ */
static inline void put_px(int x, int y, uint32_t c)
{
    if ((unsigned)x < SW && (unsigned)y < SH)
        GRAPHICS_FRAMEBUFFER[y * SW + x] = c;
}
 
static void draw_rect(int x, int y, int w, int h, uint32_t c)
{
    for (int r = y; r < y + h; r++)
        for (int col = x; col < x + w; col++)
            put_px(col, r, c);
}
 
static void draw_rect_outline(int x, int y, int w, int h, uint32_t c)
{
    for (int i = x; i < x + w; i++) { put_px(i, y, c); put_px(i, y + h - 1, c); }
    for (int i = y; i < y + h; i++) { put_px(x, i, c); put_px(x + w - 1, i, c); }
}
 
static void gradient_v(uint32_t top, uint32_t bot)
{
    for (int y = 0; y < SH; y++) {
        int t = SH - y, b = y;
        uint8_t r  = (uint8_t)(((top >> 16 & 0xFF) * t + (bot >> 16 & 0xFF) * b) / SH);
        uint8_t g  = (uint8_t)(((top >>  8 & 0xFF) * t + (bot >>  8 & 0xFF) * b) / SH);
        uint8_t bl = (uint8_t)(((top       & 0xFF) * t + (bot       & 0xFF) * b) / SH);
        uint32_t px = (0xFF << 24) | (r << 16) | (g << 8) | bl;
        for (int x = 0; x < SW; x++)
            GRAPHICS_FRAMEBUFFER[y * SW + x] = px;
    }
}
 
/* 5x7 Bitmap Font */
static const uint8_t FONT5[128][7] = {
    [' '] = {0,0,0,0,0,0,0},
    ['!'] = {0x04,0x04,0x04,0x04,0x00,0x04,0x00},
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
                        put_px(x + (4 - bit) * scale + sx, y + row * scale + sy, col);
}
 
static void draw_str(int x, int y, const char *s, uint32_t col, int scale)
{
    while (*s) { draw_char(x, y, *s, col, scale); x += 6 * scale; s++; }
}
 
/* Fare oku imleci */
static void draw_cursor(int x, int y)
{
    for (int i = 0; i < 14; i++)
        for (int j = 0; j <= i && j < 10; j++)
            put_px(x + j, y + i,
                   (j == 0 || j == i) ? 0xFF000000 : 0xFFFFFFFF);
}
 
/* ============================================================
 * 5. UYGULAMA YÖNETİCİSİ (EXE / DEB / APP STORE)
 * ============================================================ */
#define MAX_APPS 16
 
typedef enum { APP_BUILTIN = 0, APP_EXE, APP_DEB } AppType;
 
typedef struct {
    char    name[32];
    AppType type;
    int     installed;
    int     running;
} App;
 
static App app_registry[MAX_APPS] = {
    {"Terminal",  APP_BUILTIN, 1, 0},
    {"Dosyalar",  APP_BUILTIN, 1, 0},
    {"Kamera",    APP_BUILTIN, 1, 0},
    {"Mesajlar",  APP_BUILTIN, 1, 0},
    {"Haritalar", APP_BUILTIN, 1, 0},
    {"Ayarlar",   APP_BUILTIN, 1, 0},
    {"Galeri",    APP_BUILTIN, 1, 0},
    {"Muzik",     APP_BUILTIN, 1, 0},
};
static int app_count = 8;
 
typedef enum {
    WIN_NONE = 0,
    WIN_TERMINAL,
    WIN_EXE_RUNNING,
    WIN_DEB_INSTALL,
    WIN_APP_STORE,
    WIN_INSTALL_DONE
} ActiveWindow;
 
static ActiveWindow active_win      = WIN_NONE;
static int          active_app_idx  = -1;
static int          install_progress = 0;
static int          install_running  = 0;
 
static int str_eq(const char *a, const char *b)
{
    while (*a && *b) { if (*a != *b) return 0; a++; b++; }
    return (*a == '\0' && *b == '\0');
}
 
static void run_exe(const char *name)
{
    for (int i = 0; i < app_count; i++) {
        if (str_eq(app_registry[i].name, name) && app_registry[i].installed) {
            app_registry[i].running = 1;
            active_app_idx = i;
            active_win = (str_eq(name, "Terminal")) ? WIN_TERMINAL : WIN_EXE_RUNNING;
            return;
        }
    }
    active_win = WIN_TERMINAL;
}
 
static void install_deb(const char *pkgname)
{
    if (app_count >= MAX_APPS) return;
    for (int i = 0; i < app_count; i++)
        if (str_eq(app_registry[i].name, pkgname)) {
            active_win = WIN_DEB_INSTALL;
            install_progress = 100;
            return;
        }
    for (int k = 0; k < 31 && pkgname[k]; k++)
        app_registry[app_count].name[k] = pkgname[k];
    app_registry[app_count].name[31]  = '\0';
    app_registry[app_count].type      = APP_DEB;
    app_registry[app_count].installed = 0;
    app_registry[app_count].running   = 0;
    app_count++;
    install_progress = 0;
    install_running  = 1;
    active_win       = WIN_DEB_INSTALL;
}
 
static void update_install(void)
{
    if (!install_running || install_progress >= 100) return;
    install_progress += 2;
    if (install_progress >= 100) {
        install_progress = 100;
        install_running  = 0;
        if (app_count > 0)
            app_registry[app_count - 1].installed = 1;
        active_win = WIN_INSTALL_DONE;
    }
}
 
static const char *store_app_names[8] = {
    "VLC","Firefox","Calculator","TextEdit",
    "Paint","FileManager","Notes","Clock"
};
static void store_install(int idx)
{
    if (idx >= 0 && idx < 8)
        install_deb(store_app_names[idx]);
}
 
/* ============================================================
 * 6. PENCERE ÇİZİCİLERİ
 * ============================================================ */
static void draw_terminal_window(void)
{
    draw_rect(150, 150, 720, 440, 0xFF0C0C0C);
    draw_rect_outline(150, 150, 720, 440, 0xFF00E5FF);
    draw_rect(150, 150, 720, 28, 0xFF1A1A2A);
    draw_str(160, 158, "Terminal - Wind OS v1.5", 0xFF00E5FF, 1);
    draw_rect(844, 154, 22, 20, 0xFFCC3333);
    draw_str(849, 157, "X", 0xFFFFFFFF, 1);
 
    draw_str(160, 200, "root@windos:~#", 0xFF00FF00, 1);
    draw_str(264, 200, "Wind OS Terminal Aktif", 0xFFFFFFFF, 1);
    draw_str(160, 220, "root@windos:~#", 0xFF00FF00, 1);
    draw_str(264, 220, "exe_subsystem: HAZIR", 0xFFAAAAAA, 1);
    draw_str(160, 240, "root@windos:~#", 0xFF00FF00, 1);
    draw_str(264, 240, "deb_subsystem: HAZIR", 0xFFAAAAAA, 1);
    draw_str(160, 260, "root@windos:~#", 0xFF00FF00, 1);
    draw_str(264, 260, "Yüklü uygulamalar:", 0xFFAAAAAA, 1);
 
    for (int i = 0; i < app_count && i < 10; i++) {
        draw_str(170, 280 + i * 16, "-", 0xFF00FF00, 1);
        draw_str(182, 280 + i * 16, app_registry[i].name, 0xFFFFFFFF, 1);
        draw_str(380, 280 + i * 16,
                 app_registry[i].installed ? "[KURULU]" : "[BEKLIYOR]",
                 app_registry[i].installed ? 0xFF00FF00 : 0xFFFFAA00, 1);
    }
    draw_str(160, 450, "F1 Terminal  F2 Magaza  F3 DEB Kur  ESC Kapat", 0xFF666688, 1);
}
 
static void draw_exe_window(void)
{
    const char *aname = (active_app_idx >= 0)
                        ? app_registry[active_app_idx].name : "Uygulama";
    draw_rect(180, 160, 660, 400, 0xFF111120);
    draw_rect_outline(180, 160, 660, 400, 0xFF00E5FF);
    draw_rect(180, 160, 660, 28, 0xFF1E1E3A);
    draw_str(192, 168, aname, 0xFF00E5FF, 1);
    draw_str(310, 168, "- EXE Subsystem", 0xFF888888, 1);
    draw_rect(820, 162, 22, 20, 0xFFCC3333);
    draw_str(825, 165, "X", 0xFFFFFFFF, 1);
 
    draw_str(200, 210, "EXE_SUBSYSTEM: Calistiriliyor...", 0xFF00FF00, 1);
    draw_str(200, 232, "Protected Mode : AKTIF", 0xFFAAAAAA, 1);
    draw_str(200, 252, "ELF Stub       : Yuklendi", 0xFFAAAAAA, 1);
    draw_str(200, 272, "Calistirilan   :", 0xFFFFFFFF, 1);
    draw_str(340, 272, aname, 0xFF00E5FF, 1);
    draw_str(200, 300, "Status: SUCCESS", 0xFF00FF00, 1);
    draw_str(200, 325, "PID: 0x1337  MEM: 2MB  CPU: i386", 0xFF888888, 1);
    draw_str(200, 490, "ESC - Kapat", 0xFF444466, 1);
}
 
static void draw_deb_window(void)
{
    draw_rect(180, 180, 660, 380, 0xFF0A1A0A);
    draw_rect_outline(180, 180, 660, 380, 0xFF00CC44);
    draw_rect(180, 180, 660, 28, 0xFF0F2A0F);
    draw_str(192, 188, "DEB Paket Yukleyici - Wind OS", 0xFF00CC44, 1);
    draw_rect(822, 184, 22, 20, 0xFFCC3333);
    draw_str(827, 187, "X", 0xFFFFFFFF, 1);
 
    const char *pname = (app_count > 0) ? app_registry[app_count - 1].name : "paket";
    draw_str(200, 230, "Paket Adi:", 0xFFFFFFFF, 1);
    draw_str(310, 230, pname, 0xFF00CC44, 1);
    draw_str(200, 255, "Boyut: 2.4 MB", 0xFF888888, 1);
    draw_str(200, 275, "Bagimliliklar cozuluyor...", 0xFFAAAAAA, 1);
    draw_str(200, 295, "Dosyalar aciliyor...", 0xFFAAAAAA, 1);
 
    /* Progress bar */
    draw_rect(200, 330, 460, 24, 0xFF1A3A1A);
    draw_rect_outline(200, 330, 460, 24, 0xFF00CC44);
    int pw = (install_progress * 456) / 100;
    if (pw > 0) draw_rect(202, 332, pw, 20, 0xFF00CC44);
 
    char pct[5];
    int p = install_progress;
    pct[0] = '0' + (p / 100);
    pct[1] = '0' + ((p / 10) % 10);
    pct[2] = '0' + (p % 10);
    pct[3] = '%'; pct[4] = '\0';
    draw_str(672, 332, pct, 0xFF00CC44, 1);
 
    if (install_progress >= 100) {
        draw_str(200, 375, "Kurulum TAMAMLANDI!", 0xFF00FF00, 1);
        draw_str(200, 395, pname, 0xFF00CC44, 1);
        draw_str(298, 395, "basariyla yuklendi.", 0xFFFFFFFF, 1);
    } else {
        draw_str(200, 375, "Kuruluyor, lutfen bekleyin...", 0xFF888888, 1);
    }
    draw_str(200, 510, "ESC - Kapat", 0xFF444466, 1);
}
 
static void draw_store_window(void)
{
    draw_rect(120, 100, 780, 520, 0xFF0D1117);
    draw_rect_outline(120, 100, 780, 520, 0xFF0078D4);
    draw_rect(120, 100, 780, 32, 0xFF161B22);
    draw_str(135, 110, "Wind Store - Uygulama Magazasi", 0xFF0078D4, 1);
    draw_rect(878, 104, 22, 24, 0xFFCC3333);
    draw_str(883, 108, "X", 0xFFFFFFFF, 1);
 
    const char *descs[8] = {
        "Medya oynatici","Web tarayici","Hesap makinesi","Metin editoru",
        "Resim editoru","Dosya yoneticisi","Not defteri","Saat"
    };
    uint32_t ic[8] = {
        0xFFFF6600,0xFF4285F4,0xFF34A853,0xFF0078D4,
        0xFFEA4335,0xFF9C27B0,0xFFFF9800,0xFF00BCD4
    };
 
    for (int i = 0; i < 8; i++) {
        int cx = 135 + (i % 4) * 190;
        int cy = 160 + (i / 4) * 195;
        draw_rect(cx, cy, 170, 165, 0xFF161B22);
        draw_rect_outline(cx, cy, 170, 165, 0xFF30363D);
        draw_rect(cx + 55, cy + 18, 60, 60, ic[i]);
        draw_str(cx + 10, cy + 90, store_app_names[i], 0xFFFFFFFF, 1);
        draw_str(cx + 10, cy + 106, descs[i], 0xFF888888, 1);
        draw_rect(cx + 18, cy + 130, 134, 22, 0xFF0078D4);
        draw_str(cx + 22, cy + 135, "F ile yukle", 0xFFFFFFFF, 1);
    }
    draw_str(135, 610, "F5:VLC  F6:Firefox  F7:Calc  F8:TextEdit   ESC:Kapat", 0xFF444466, 1);
}
 
static void draw_done_window(void)
{
    draw_rect(280, 260, 464, 200, 0xFF0A1A0A);
    draw_rect_outline(280, 260, 464, 200, 0xFF00FF00);
    draw_str(340, 290, "Kurulum Tamamlandi!", 0xFF00FF00, 2);
    const char *pname = (app_count > 0) ? app_registry[app_count - 1].name : "paket";
    draw_str(300, 345, pname, 0xFF00CC44, 1);
    draw_str(406, 345, "basariyla yuklendi.", 0xFFFFFFFF, 1);
    draw_str(300, 368, "Uygulama masaustune eklendi.", 0xFF888888, 1);
    draw_str(330, 420, "Herhangi bir tusa basin...", 0xFF444466, 1);
}
 
/* ============================================================
 * 7. UI EKRANLARI (OOBE + MASAÜSTÜ)
 * ============================================================ */
static void draw_oobe_base(void)
{
    gradient_v(0xFFE8F4FD, 0xFFD0E8F8);
    draw_rect(190, 90, 644, 560, 0x22000000);
    draw_rect(186, 86, 644, 560, 0x11000000);
    draw_rect(192, 94, 640, 556, 0xFFFFFFFF);
    draw_rect_outline(192, 94, 640, 556, 0xFFDDE5EE);
}
 
static void draw_next_btn(int bx, int by)
{
    draw_rect(bx, by, 180, 46, 0xFF0078D4);
    draw_rect_outline(bx, by, 180, 46, 0xFF005A9E);
    draw_str(bx + 50, by + 15, "Evet", 0xFFFFFFFF, 2);
}
 
static void render_oobe_region(void)
{
    draw_oobe_base();
    draw_str(270, 130, "Bu dogru ulke", 0xFF1A1A2E, 2);
    draw_str(270, 155, "veya bolge mi?", 0xFF1A1A2E, 2);
    draw_rect(280, 230, 440, 50, 0xFFF0F6FF);
    draw_rect_outline(280, 230, 440, 50, 0xFF0078D4);
    draw_str(294, 248, "Turkiye", 0xFF1A1A2E, 2);
    draw_str(230, 460, "ENTER Ileri  BACKSPACE Geri", 0xFF666680, 1);
    draw_next_btn(590, 570);
}
 
static void render_oobe_keyboard(void)
{
    draw_oobe_base();
    draw_str(250, 130, "Bu dogru klavye", 0xFF1A1A2E, 2);
    draw_str(250, 155, "duzeni mi?", 0xFF1A1A2E, 2);
    int kx = 220, ky = 230;
    for (int row = 0; row < 4; row++) {
        int keys = (row == 0) ? 12 : (row == 1) ? 11 : (row == 2) ? 10 : 9;
        int kw = 42, kh = 38, gap = 4;
        for (int k = 0; k < keys; k++) {
            draw_rect(kx + k * (kw + gap) + row * 8, ky + row * (kh + gap), kw, kh, 0xFFEFF3F8);
            draw_rect_outline(kx + k * (kw + gap) + row * 8, ky + row * (kh + gap), kw, kh, 0xFFB0BEC5);
        }
    }
    draw_str(250, 480, "Secim: Turkce Q Klavyesi", 0xFF444466, 1);
    draw_str(230, 510, "ENTER Ileri  BACKSPACE Geri", 0xFF666680, 1);
    draw_next_btn(590, 570);
}
 
static void render_oobe_network(void)
{
    draw_oobe_base();
    draw_str(235, 130, "Bir aga baglanin", 0xFF1A1A2E, 2);
    int wx = 452, wy = 220;
    for (int a = 0; a < 4; a++) {
        int r = (a + 1) * 30;
        draw_rect_outline(wx - r, wy - r / 2 + a * 8, r * 2, r, 0xFF0078D4);
    }
    draw_rect(wx - 4, wy + 70, 8, 20, 0xFF0078D4);
    const char *nets[3] = {"Ev Wifi", "Ofis-5G", "Misafir"};
    for (int i = 0; i < 3; i++) {
        draw_rect(280, 340 + i * 60, 440, 46, i == 0 ? 0xFFE8F4FD : 0xFFF5F5F5);
        draw_rect_outline(280, 340 + i * 60, 440, 46, i == 0 ? 0xFF0078D4 : 0xFFDDDDDD);
        draw_str(300, 356 + i * 60, nets[i], 0xFF1A1A2E, 2);
    }
    draw_str(230, 560, "ENTER Ileri  BACKSPACE Geri", 0xFF666680, 1);
    draw_next_btn(590, 570);
}
 
static void render_oobe_name(void)
{
    draw_oobe_base();
    draw_str(220, 130, "Bilgisayariniza", 0xFF1A1A2E, 2);
    draw_str(220, 155, "bir ad verin", 0xFF1A1A2E, 2);
    draw_rect(390, 220, 220, 140, 0xFF37474F);
    draw_rect(398, 228, 204, 120, 0xFF263238);
    draw_rect(360, 360, 280, 16, 0xFF546E7A);
    draw_rect(350, 376, 300, 8, 0xFF455A64);
    draw_rect(230, 420, 560, 50, 0xFFFFFFFF);
    draw_rect_outline(230, 420, 560, 50, 0xFF0078D4);
    draw_str(244, 438, "Wind-PC", 0xFF1A1A2E, 2);
    draw_str(230, 510, "ENTER Ileri  BACKSPACE Geri", 0xFF666680, 1);
    draw_next_btn(590, 570);
}
 
static void render_oobe_privacy(void)
{
    draw_oobe_base();
    draw_str(230, 130, "Gizlilik ayarlari", 0xFF1A1A2E, 2);
    const char *items[5] = {
        "Konum servislerini ac",
        "Teshis verisi gonder",
        "Kisisellestirilmis deneyim",
        "Reklam kimligini kullan",
        "Ag baglantilarini optimize et"
    };
    for (int i = 0; i < 5; i++) {
        draw_rect(230, 220 + i * 52, 540, 44, 0xFFF9F9F9);
        draw_rect_outline(230, 220 + i * 52, 540, 44, 0xFFDDDDDD);
        draw_rect(700, 232 + i * 52, 50, 22, 0xFF0078D4);
        draw_rect(722, 234 + i * 52, 18, 18, 0xFFFFFFFF);
        draw_str(244, 234 + i * 52, items[i], 0xFF333333, 1);
    }
    draw_str(230, 510, "ENTER Ileri  BACKSPACE Geri", 0xFF666680, 1);
    draw_next_btn(590, 570);
}
 
static void render_oobe_customize(void)
{
    draw_oobe_base();
    draw_str(200, 130, "Deneyiminizi ozellestirin", 0xFF1A1A2E, 2);
    draw_str(230, 165, "Size en uygun kullanim amacini secin:", 0xFF444466, 1);
    const char *cats[4] = {"Oyun", "Isletme", "Okullar", "Kisisel"};
    uint32_t    cols[4] = {0xFF7C3AED, 0xFF0078D4, 0xFF059669, 0xFFEA580C};
    for (int i = 0; i < 4; i++) {
        int cx = 232 + i * 158, cy = 240;
        draw_rect(cx, cy, 140, 140, cols[i]);
        draw_rect_outline(cx, cy, 140, 140, 0xFF333333);
        draw_str(cx + 8, cy + 120, cats[i], 0xFFFFFFFF, 2);
    }
    draw_str(230, 510, "ENTER Ileri  BACKSPACE Geri", 0xFF666680, 1);
    draw_next_btn(590, 570);
}
 
static void render_desktop(void)
{
    gradient_v(0xFF1A0F2E, 0xFF0D0B18);
 
    /* Üst görev çubuğu */
    draw_rect(0, 0, SW, 38, 0xAA14102A);
    draw_str(10, 12, "Wind OS v1.5", 0xFF00E5FF, 1);
    draw_str(850, 12, "26:03", 0xFF00E5FF, 1);
    draw_str(180, 25, "F1:Term F2:Magaza F3:DEB F4:Run ESC:Kapat", 0xFF444466, 1);
 
    /* Hava & Saat Widget */
    draw_rect(30, 52, 480, 180, 0xAA211C38);
    draw_rect_outline(30, 52, 480, 180, 0xFF444466);
    draw_str(50, 72,  "SAAT: 26:03", 0xFF00E5FF, 2);
    draw_str(50, 112, "Hava Durumu - Esenyurt", 0xFFF5F5F5, 1);
    draw_str(50, 132, "21C  Bulutlu ve Firtina", 0xFFD0D0D0, 1);
 
    /* Ay ikonu */
    int mx = 432, my = 115, mr = 42;
    for (int y2 = -mr; y2 <= mr; y2++)
        for (int x2 = -mr; x2 <= mr; x2++)
            if (x2 * x2 + y2 * y2 <= mr * mr)
                put_px(mx + x2, my + y2, 0xFFFFD700);
    for (int y2 = -mr + 6; y2 <= mr; y2++)
        for (int x2 = -mr + 14; x2 <= mr + 14; x2++)
            if (x2 * x2 + y2 * y2 <= mr * mr)
                put_px(mx + x2, my + y2, 0xFF1A0F2E);
 
    /* Sol dock */
    int show = (app_count < 5) ? app_count : 5;
    for (int i = 0; i < show; i++) {
        int dy = 260 + i * 82;
        draw_rect(30, dy, 80, 60, 0xFF0D0B18);
        draw_rect_outline(30, dy, 80, 60,
                          app_registry[i].running ? 0xFF00E5FF : 0xFF444466);
        draw_str(34, dy + 22, app_registry[i].name, 0xFFF5F5F5, 1);
    }
 
    /* Sağ panel */
    int bx = SW - 230, bw = 210;
    draw_rect(bx, 44, bw, SH - 124, 0xAA211C38);
    draw_rect_outline(bx, 44, bw, SH - 124, 0xFF444466);
    draw_str(bx + 30, 62, "UYGULAMALAR", 0xFF00E5FF, 1);
    for (int i = 0; i < app_count && i < 8; i++) {
        draw_rect(bx + 14, 90 + i * 68, bw - 28, 52, 0xFF0D0B18);
        draw_rect_outline(bx + 14, 90 + i * 68, bw - 28, 52, 0xFF333355);
        draw_str(bx + 24, 108 + i * 68, app_registry[i].name, 0xFFF5F5F5, 1);
        draw_str(bx + 24, 122 + i * 68,
                 app_registry[i].installed ? "[Kurulu]" : "[Bekliyor]",
                 app_registry[i].installed ? 0xFF00CC44 : 0xFFFFAA00, 1);
    }
 
    /* Aktif pencere */
    switch (active_win) {
        case WIN_TERMINAL:     draw_terminal_window(); break;
        case WIN_EXE_RUNNING:  draw_exe_window();      break;
        case WIN_DEB_INSTALL:  draw_deb_window();      break;
        case WIN_APP_STORE:    draw_store_window();    break;
        case WIN_INSTALL_DONE: draw_done_window();     break;
        default: {
            int px = 240, py = 300;
            draw_rect(px, py, 380, 120, 0xFFFFFFFF);
            draw_rect_outline(px, py, 380, 120, 0xFF00E5FF);
            draw_str(px + 30, py + 20, "HOS GELDINIZ!", 0xFF1A1A2E, 2);
            draw_str(px + 30, py + 60, "Wind OS v1.5 aktif.", 0xFF0078D4, 1);
            draw_str(px + 30, py + 80, "F1 Terminal  F2 Magaza", 0xFF888888, 1);
        } break;
    }
 
    /* Alt dock */
    draw_rect(20, SH - 72, 700, 56, 0xAA211C38);
    draw_rect_outline(20, SH - 72, 700, 56, 0xFF00E5FF);
    draw_str(35, SH - 52, "TUSUMANA BASINCA CEKMECE ACILSIN", 0xFFF5F5F5, 1);
}
 
/* ============================================================
 * 8. DURUM MAKİNESİ
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
        case STATE_REGION:    render_oobe_region();    break;
        case STATE_KEYBOARD:  render_oobe_keyboard();  break;
        case STATE_NETWORK:   render_oobe_network();   break;
        case STATE_NAME:      render_oobe_name();      break;
        case STATE_PRIVACY:   render_oobe_privacy();   break;
        case STATE_CUSTOMIZE: render_oobe_customize(); break;
        case STATE_DESKTOP:   render_desktop();        break;
    }
    if (g_state == STATE_DESKTOP)
        draw_cursor(mouse_x, mouse_y);
}
 
static void go_next(void) { if (g_state < STATE_DESKTOP) { g_state++; refresh(); } }
static void go_prev(void) { if (g_state > STATE_REGION)  { g_state--; refresh(); } }
 
/* ============================================================
 * 9. FARE SÜRÜCÜsü (PS/2)
 * ============================================================ */
static void mouse_wait_in(void)
{
    uint32_t t = 100000; while (t--) { if ( inb(0x64) & 1)  return; }
}
static void mouse_wait_out(void)
{
    uint32_t t = 100000; while (t--) { if (!(inb(0x64) & 2)) return; }
}
static void mouse_send(uint8_t c)
{
    mouse_wait_out(); outb(0x64, 0xD4);
    mouse_wait_out(); outb(0x60, c);
    mouse_wait_in();  inb(0x60);
}
 
void mouse_init(void)
{
    mouse_wait_out(); outb(0x64, 0xA8);
    mouse_wait_out(); outb(0x64, 0x20);
    mouse_wait_in();
    uint8_t st = inb(0x60) | 2;
    mouse_wait_out(); outb(0x64, 0x60);
    mouse_wait_out(); outb(0x60, st);
    mouse_send(0xF6);
    mouse_send(0xF4);
}
 
static void handle_mouse(void)
{
    static uint8_t cyc = 0;
    static uint8_t mb[3];
 
    uint8_t st = inb(0x64);
    if (!(st & 1))    return;   /* veri yok */
    if (!(st & 0x20)) {         /* klavye verisi — yut */
        (void)inb(0x60); return;
    }
 
    mb[cyc++] = inb(0x60);
    if (cyc < 3) return;
    cyc = 0;
 
    if (!(mb[0] & 0x08)) return; /* geçersiz paket */
 
    mouse_x += (int8_t)mb[1] / 2;
    mouse_y -= (int8_t)mb[2] / 2;
    if (mouse_x < 0)   mouse_x = 0;
    if (mouse_y < 0)   mouse_y = 0;
    if (mouse_x >= SW) mouse_x = SW - 1;
    if (mouse_y >= SH) mouse_y = SH - 1;
 
    /* Sol tık */
    if ((mb[0] & 1) && g_state == STATE_DESKTOP) {
        int bx = SW - 230, bw = 210;
        for (int i = 0; i < app_count && i < 8; i++) {
            int ay = 90 + i * 68;
            if (mouse_x > bx + 14 && mouse_x < bx + bw - 14 &&
                mouse_y > ay      && mouse_y < ay + 52) {
                run_exe(app_registry[i].name);
            }
        }
        /* Kapat butonları */
        if (active_win != WIN_NONE) {
            if (mouse_x > 844 && mouse_x < 866 && mouse_y > 154 && mouse_y < 174)
                active_win = WIN_NONE;
            if (mouse_x > 822 && mouse_x < 844 && mouse_y > 184 && mouse_y < 204)
                active_win = WIN_NONE;
            if (mouse_x > 878 && mouse_x < 900 && mouse_y > 104 && mouse_y < 128)
                active_win = WIN_NONE;
        }
    }
 
    if (g_state == STATE_DESKTOP)
        refresh();
}
 
/* ============================================================
 * 10. PIC YENİDEN PROGRAMLAMA
 * ============================================================ */
void idt_init(void)
{
    outb(0x20, 0x11); outb(0xA0, 0x11);
    outb(0x21, 0x20); outb(0xA1, 0x28);
    outb(0x21, 0x04); outb(0xA1, 0x02);
    outb(0x21, 0x01); outb(0xA1, 0x01);
    outb(0x21, 0x00); outb(0xA1, 0x00);
}
 
/* ============================================================
 * 11. EXE / DEB DIŞ ARAYÜZÜ
 * ============================================================ */
void exe_subsystem_init(void)
{
    if (g_state == STATE_DESKTOP) {
        active_win = WIN_TERMINAL;
        refresh();
    }
}
 
void deb_subsystem_init(void)
{
    if (g_state == STATE_DESKTOP) {
        install_deb("WindPkg");
        refresh();
    }
}
 
/* ============================================================
 * 12. KERNEL GİRİŞ NOKTASI
 * ============================================================ */
void kernel_main(void *mboot_ptr, uint32_t magic)
{
    (void)magic;
 
    if (mboot_ptr) {
        uint32_t flags = *(uint32_t *)mboot_ptr;
        if (flags & (1u << 11)) {
            uint32_t fb_addr = ((uint32_t *)mboot_ptr)[22];
            if (fb_addr) GRAPHICS_FRAMEBUFFER = (uint32_t *)fb_addr;
        }
    }
 
    is_graphics_mode = 1;
    force_graphics_hardware();
    idt_init();
    mouse_init();
 
    g_state = STATE_REGION;
    refresh();
 
    while (1) {
        update_install();
        if (install_running && g_state == STATE_DESKTOP)
            refresh();
 
        uint8_t kst = inb(0x64);
        if ((kst & 1) && !(kst & 0x20)) {
            uint8_t sc = inb(0x60);
            if (!(sc & 0x80)) {
                if (g_state != STATE_DESKTOP) {
                    if (sc == 0x1C) go_next();
                    if (sc == 0x0E) go_prev();
                } else {
                    switch (sc) {
                        case 0x3B: active_win = WIN_TERMINAL;  refresh(); break;
                        case 0x3C: active_win = WIN_APP_STORE; refresh(); break;
                        case 0x3D: install_deb("WindPkg");      refresh(); break;
                        case 0x3E: run_exe("Terminal");         refresh(); break;
                        case 0x01: active_win = WIN_NONE;       refresh(); break;
                        case 0x3F: store_install(0); refresh(); break;
                        case 0x40: store_install(1); refresh(); break;
                        case 0x41: store_install(2); refresh(); break;
                        case 0x42: store_install(3); refresh(); break;
                        default: break;
                    }
                }
            }
        }
 
        handle_mouse();
        __asm__ volatile("pause");
    }
}
 
/* ============================================================
 * 13. STUB'LAR
 * ============================================================ */
void keyboard_init(void)       {}
void wind_subsystem_init(void) {}
void ai_subsystem_init(void)   {}
