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
static u32  SCR_PITCH = 1024;  /* pitch in pixels (pitch_bytes/4) */

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
#define C_BLUE        0xFF0078D4u
#define C_DARK_BLUE   0xFF003E92u
#define C_LIGHT_BLUE  0xFFDEECF9u
#define C_GRAY        0xFF767676u
#define C_LIGHT_GRAY  0xFFE5E5E5u
#define C_MID_GRAY    0xFFB0B0B0u
#define C_DARK_GRAY   0xFF333333u
#define C_RED         0xFFD13438u
#define C_GREEN       0xFF107C10u
#define C_ORANGE      0xFFFF8C00u
#define C_BORDER      0xFFCFCFCFu
#define C_TASKBAR     0xFF1E1E1Eu
#define C_TASKBAR_HL  0xFF3A3A3Au
#define C_TOGGLE_ON   0xFF0078D4u
#define C_TOGGLE_OFF  0xFF767676u
#define C_INPUT_BG    0xFFFFFFFFu
#define C_CARD        0xFFFFFFFFu
#define C_SHADOW      0x55000000u
#define C_HEADER      0xFFEAEAF0u

/* =========================================================
   PORT G/Ç
   ========================================================= */
static inline u8  inb(u16 p)       { u8  v; __asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p)); return v; }
static inline void outb(u16 p,u8 v){ __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p)); }
static inline u32 inl(u16 p)       { u32 v; __asm__ volatile("inl %1,%0":"=a"(v):"Nd"(p)); return v; }
static inline void outl(u16 p,u32 v){ __asm__ volatile("outl %0,%1"::"a"(v),"Nd"(p)); }

/* =========================================================
   BELLEK / STRİNG YARDIMCILARI
   ========================================================= */
static void* memset_k(void* d, int c, u32 n){
    u8* p=(u8*)d; while(n--)*p++=(u8)c; return d;
}
static void* memcpy_k(void* d,const void* s,u32 n){
    u8* dp=(u8*)d; const u8* sp=(const u8*)s; while(n--)*dp++=*sp++; return d;
}
static u32 kstrlen(const char* s){ u32 n=0; while(s[n])n++; return n; }
static void kstrcpy(char* d,const char* s){ while(*s)*d++=*s++; *d=0; }
static int kstrcmp(const char* a,const char* b){
    while(*a&&*a==*b){a++;b++;} return (u8)*a-(u8)*b;
}

/* =========================================================
   TAMSAYI KÖK (sqrt olmadan)
   ========================================================= */
static u32 isqrt(u32 n){
    if(!n) return 0;
    u32 x=n, y=1;
    while(x>y){ x=(x+y)/2; y=n/x; }
    return x;
}

/* =========================================================
   8×8 BİTMAP FONT (IBM PC)
   ========================================================= */
static const u8 font8x8[128][8] = {
    [' '] ={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['!'] ={0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00},
    ['"'] ={0x36,0x36,0x00,0x00,0x00,0x00,0x00,0x00},
    ['#'] ={0x36,0x36,0x7F,0x36,0x7F,0x36,0x36,0x00},
    ['$'] ={0x0C,0x3E,0x03,0x1E,0x30,0x1F,0x0C,0x00},
    ['%'] ={0x00,0x63,0x33,0x18,0x0C,0x66,0x63,0x00},
    ['&'] ={0x1C,0x36,0x1C,0x6E,0x3B,0x33,0x6E,0x00},
    ['\'']={0x06,0x06,0x03,0x00,0x00,0x00,0x00,0x00},
    ['('] ={0x18,0x0C,0x06,0x06,0x06,0x0C,0x18,0x00},
    [')'] ={0x06,0x0C,0x18,0x18,0x18,0x0C,0x06,0x00},
    ['*'] ={0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00},
    ['+'] ={0x00,0x0C,0x0C,0x3F,0x0C,0x0C,0x00,0x00},
    [','] ={0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x06},
    ['-'] ={0x00,0x00,0x00,0x3F,0x00,0x00,0x00,0x00},
    ['.'] ={0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x00},
    ['/'] ={0x60,0x30,0x18,0x0C,0x06,0x03,0x01,0x00},
    ['0'] ={0x3E,0x63,0x73,0x7B,0x6F,0x67,0x3E,0x00},
    ['1'] ={0x0C,0x0E,0x0C,0x0C,0x0C,0x0C,0x3F,0x00},
    ['2'] ={0x1E,0x33,0x30,0x1C,0x06,0x33,0x3F,0x00},
    ['3'] ={0x1E,0x33,0x30,0x1C,0x30,0x33,0x1E,0x00},
    ['4'] ={0x38,0x3C,0x36,0x33,0x7F,0x30,0x78,0x00},
    ['5'] ={0x3F,0x03,0x1F,0x30,0x30,0x33,0x1E,0x00},
    ['6'] ={0x1C,0x06,0x03,0x1F,0x33,0x33,0x1E,0x00},
    ['7'] ={0x3F,0x33,0x30,0x18,0x0C,0x0C,0x0C,0x00},
    ['8'] ={0x1E,0x33,0x33,0x1E,0x33,0x33,0x1E,0x00},
    ['9'] ={0x1E,0x33,0x33,0x3E,0x30,0x18,0x0E,0x00},
    [':'] ={0x00,0x0C,0x0C,0x00,0x00,0x0C,0x0C,0x00},
    [';'] ={0x00,0x0C,0x0C,0x00,0x00,0x0C,0x0C,0x06},
    ['<'] ={0x18,0x0C,0x06,0x03,0x06,0x0C,0x18,0x00},
    ['='] ={0x00,0x00,0x3F,0x00,0x00,0x3F,0x00,0x00},
    ['>'] ={0x06,0x0C,0x18,0x30,0x18,0x0C,0x06,0x00},
    ['?'] ={0x1E,0x33,0x30,0x18,0x0C,0x00,0x0C,0x00},
    ['@'] ={0x3E,0x63,0x7B,0x7B,0x7B,0x03,0x1E,0x00},
    ['A'] ={0x0C,0x1E,0x33,0x33,0x3F,0x33,0x33,0x00},
    ['B'] ={0x3F,0x66,0x66,0x3E,0x66,0x66,0x3F,0x00},
    ['C'] ={0x3C,0x66,0x03,0x03,0x03,0x66,0x3C,0x00},
    ['D'] ={0x1F,0x36,0x66,0x66,0x66,0x36,0x1F,0x00},
    ['E'] ={0x7F,0x46,0x16,0x1E,0x16,0x46,0x7F,0x00},
    ['F'] ={0x7F,0x46,0x16,0x1E,0x16,0x06,0x0F,0x00},
    ['G'] ={0x3C,0x66,0x03,0x03,0x73,0x66,0x7C,0x00},
    ['H'] ={0x33,0x33,0x33,0x3F,0x33,0x33,0x33,0x00},
    ['I'] ={0x1E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00},
    ['J'] ={0x78,0x30,0x30,0x30,0x33,0x33,0x1E,0x00},
    ['K'] ={0x67,0x66,0x36,0x1E,0x36,0x66,0x67,0x00},
    ['L'] ={0x0F,0x06,0x06,0x06,0x46,0x66,0x7F,0x00},
    ['M'] ={0x63,0x77,0x7F,0x7F,0x6B,0x63,0x63,0x00},
    ['N'] ={0x63,0x67,0x6F,0x7B,0x73,0x63,0x63,0x00},
    ['O'] ={0x1C,0x36,0x63,0x63,0x63,0x36,0x1C,0x00},
    ['P'] ={0x3F,0x66,0x66,0x3E,0x06,0x06,0x0F,0x00},
    ['Q'] ={0x1E,0x33,0x33,0x33,0x3B,0x1E,0x38,0x00},
    ['R'] ={0x3F,0x66,0x66,0x3E,0x36,0x66,0x67,0x00},
    ['S'] ={0x1E,0x33,0x07,0x0E,0x38,0x33,0x1E,0x00},
    ['T'] ={0x3F,0x2D,0x0C,0x0C,0x0C,0x0C,0x1E,0x00},
    ['U'] ={0x33,0x33,0x33,0x33,0x33,0x33,0x3F,0x00},
    ['V'] ={0x33,0x33,0x33,0x33,0x33,0x1E,0x0C,0x00},
    ['W'] ={0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00},
    ['X'] ={0x63,0x63,0x36,0x1C,0x1C,0x36,0x63,0x00},
    ['Y'] ={0x33,0x33,0x33,0x1E,0x0C,0x0C,0x1E,0x00},
    ['Z'] ={0x7F,0x63,0x31,0x18,0x4C,0x66,0x7F,0x00},
    ['['] ={0x1E,0x06,0x06,0x06,0x06,0x06,0x1E,0x00},
    ['\\']={0x03,0x06,0x0C,0x18,0x30,0x60,0x40,0x00},
    [']'] ={0x1E,0x18,0x18,0x18,0x18,0x18,0x1E,0x00},
    ['^'] ={0x08,0x1C,0x36,0x63,0x00,0x00,0x00,0x00},
    ['_'] ={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF},
    ['`'] ={0x0C,0x0C,0x18,0x00,0x00,0x00,0x00,0x00},
    ['a'] ={0x00,0x00,0x1E,0x30,0x3E,0x33,0x6E,0x00},
    ['b'] ={0x07,0x06,0x06,0x3E,0x66,0x66,0x3B,0x00},
    ['c'] ={0x00,0x00,0x1E,0x33,0x03,0x33,0x1E,0x00},
    ['d'] ={0x38,0x30,0x30,0x3E,0x33,0x33,0x6E,0x00},
    ['e'] ={0x00,0x00,0x1E,0x33,0x3F,0x03,0x1E,0x00},
    ['f'] ={0x1C,0x36,0x06,0x0F,0x06,0x06,0x0F,0x00},
    ['g'] ={0x00,0x00,0x6E,0x33,0x33,0x3E,0x30,0x1F},
    ['h'] ={0x07,0x06,0x36,0x6E,0x66,0x66,0x67,0x00},
    ['i'] ={0x0C,0x00,0x0E,0x0C,0x0C,0x0C,0x1E,0x00},
    ['j'] ={0x30,0x00,0x30,0x30,0x30,0x33,0x33,0x1E},
    ['k'] ={0x07,0x06,0x66,0x36,0x1E,0x36,0x67,0x00},
    ['l'] ={0x0E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00},
    ['m'] ={0x00,0x00,0x33,0x7F,0x7F,0x6B,0x63,0x00},
    ['n'] ={0x00,0x00,0x1F,0x33,0x33,0x33,0x33,0x00},
    ['o'] ={0x00,0x00,0x1E,0x33,0x33,0x33,0x1E,0x00},
    ['p'] ={0x00,0x00,0x3B,0x66,0x66,0x3E,0x06,0x0F},
    ['q'] ={0x00,0x00,0x6E,0x33,0x33,0x3E,0x30,0x78},
    ['r'] ={0x00,0x00,0x3B,0x6E,0x66,0x06,0x0F,0x00},
    ['s'] ={0x00,0x00,0x3E,0x03,0x1E,0x30,0x1F,0x00},
    ['t'] ={0x08,0x0C,0x3E,0x0C,0x0C,0x2C,0x18,0x00},
    ['u'] ={0x00,0x00,0x33,0x33,0x33,0x33,0x6E,0x00},
    ['v'] ={0x00,0x00,0x33,0x33,0x33,0x1E,0x0C,0x00},
    ['w'] ={0x00,0x00,0x63,0x6B,0x7F,0x7F,0x36,0x00},
    ['x'] ={0x00,0x00,0x63,0x36,0x1C,0x36,0x63,0x00},
    ['y'] ={0x00,0x00,0x33,0x33,0x33,0x3E,0x30,0x1F},
    ['z'] ={0x00,0x00,0x3F,0x19,0x0C,0x26,0x3F,0x00},
    ['{'] ={0x38,0x0C,0x0C,0x07,0x0C,0x0C,0x38,0x00},
    ['|'] ={0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00},
    ['}'] ={0x07,0x0C,0x0C,0x38,0x0C,0x0C,0x07,0x00},
    ['~'] ={0x6E,0x3B,0x00,0x00,0x00,0x00,0x00,0x00},
};

/* =========================================================
   FRAMEBUFFER ÇİZİM FONKSİYONLARI
   ========================================================= */
static inline void put_pixel(i32 x, i32 y, u32 col){
    if((u32)x < SCR_W && (u32)y < SCR_H)
        FB[y * SCR_PITCH + x] = col;
}

static void fill_rect(i32 x, i32 y, i32 w, i32 h, u32 col){
    for(i32 j=y; j<y+h; j++)
        for(i32 i=x; i<x+w; i++)
            put_pixel(i,j,col);
}

static void draw_rect_border(i32 x,i32 y,i32 w,i32 h,u32 col,i32 t){
    fill_rect(x,     y,     w, t,   col);
    fill_rect(x,     y+h-t, w, t,   col);
    fill_rect(x,     y,     t, h,   col);
    fill_rect(x+w-t, y,     t, h,   col);
}

static void draw_circle(i32 cx, i32 cy, i32 r, u32 col){
    for(i32 dy=-r; dy<=r; dy++)
        for(i32 dx=-r; dx<=r; dx++)
            if(dx*dx+dy*dy <= r*r)
                put_pixel(cx+dx, cy+dy, col);
}

/* Dolu yuvarlak dikdörtgen (corner radius = r) */
static void fill_rrect(i32 x, i32 y, i32 w, i32 h, i32 r, u32 col){
    fill_rect(x+r,   y,     w-2*r, h,       col);
    fill_rect(x,     y+r,   r,     h-2*r,   col);
    fill_rect(x+w-r, y+r,   r,     h-2*r,   col);
    draw_circle(x+r,       y+r,     r, col);
    draw_circle(x+w-r-1,   y+r,     r, col);
    draw_circle(x+r,       y+h-r-1, r, col);
    draw_circle(x+w-r-1,   y+h-r-1, r, col);
}

static void draw_char(i32 x, i32 y, char c, u32 fg, u32 bg, i32 sc){
    if((u8)c>=128) c='?';
    const u8* g = font8x8[(u8)c];
    for(i32 row=0;row<8;row++)
        for(i32 col=0;col<8;col++){
            u32 color = (g[row]&(1<<(7-col))) ? fg : bg;
            fill_rect(x+col*sc, y+row*sc, sc, sc, color);
        }
}

static void draw_str(i32 x, i32 y, const char* s, u32 fg, u32 bg, i32 sc){
    i32 cx=x;
    while(*s){
        if(*s=='\n'){cx=x;y+=8*sc;}
        else{draw_char(cx,y,*s,fg,bg,sc);cx+=8*sc;}
        s++;
    }
}

static void draw_str_c(i32 x, i32 y, i32 w, const char* s, u32 fg, u32 bg, i32 sc){
    i32 tw = (i32)kstrlen(s)*8*sc;
    draw_str(x+(w-tw)/2, y, s, fg, bg, sc);
}

/* WiFi yay çizici (sqrt kullanarak) */
static void draw_wifi_arc(i32 cx, i32 cy, i32 r, i32 thick, u32 col){
    for(i32 dx2=-r; dx2<=r; dx2++){
        u32 dy_sq = (u32)(r*r - dx2*dx2);
        i32 dy2 = -(i32)isqrt(dy_sq);
        /* sadece üst yarım, en dıştaki thick piksel */
        for(i32 t=0;t<thick;t++){
            put_pixel(cx+dx2, cy+dy2-t, col);
        }
    }
}

/* =========================================================
   PS/2 KLAVYE SÜRÜCÜSÜ (Polling)
   ========================================================= */
#define KBD_DATA   0x60
#define KBD_STATUS 0x64

static u8 kbd_shift = 0;
static u8 kbd_caps  = 0;

static const char sc_map[128] = {
  0, 27,'1','2','3','4','5','6','7','8','9','0','-','=', 8,
  '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
  0,'a','s','d','f','g','h','j','k','l',';','\'','`',
  0,'\\','z','x','c','v','b','n','m',',','.','/',
  0,'*', 0,' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, '-', 0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* Bir tuşa basıldıysa ASCII döndür, yoksa 0 */
static u8 kbd_poll(void){
    if(!(inb(KBD_STATUS) & 0x01)) return 0;
    u8 sc = inb(KBD_DATA);
    /* sadece klavye (bit 5 = mouse veri bayrağı) */
    if(inb(KBD_STATUS) & 0x20) return 0; /* mouse veri, klavye değil */

    if(sc & 0x80){ /* tuş bırakıldı */
        u8 r = sc & 0x7F;
        if(r==0x2A||r==0x36) kbd_shift=0;
        return 0;
    }
    if(sc==0x2A||sc==0x36){ kbd_shift=1; return 0; }
    if(sc==0x3A){ kbd_caps=!kbd_caps; return 0; }
    if(sc>=128) return 0;
    char c = sc_map[sc];
    if(!c) return 0;
    if(c>='a'&&c<='z'){
        if(kbd_shift ^ kbd_caps) c -= 32;
    } else if(kbd_shift){
        switch(c){
          case '1':c='!';break; case '2':c='@';break; case '3':c='#';break;
          case '4':c='$';break; case '5':c='%';break; case '6':c='^';break;
          case '7':c='&';break; case '8':c='*';break; case '9':c='(';break;
          case '0':c=')';break; case '-':c='_';break; case '=':c='+';break;
          case '[':c='{';break; case ']':c='}';break; case ';':c=':';break;
          case '\'':c='"';break; case ',':c='<';break; case '.':c='>';break;
          case '/':c='?';break; case '`':c='~';break; case '\\':c='|';break;
        }
    }
    return (u8)c;
}

/* =========================================================
   PS/2 MOUSE SÜRÜCÜSÜ
   ========================================================= */
#define MOUSE_STATUS 0x64
#define MOUSE_CMD    0x64
#define MOUSE_DATA   0x60

static i32 mx=512, my=384;
static i32 mlb=0, mrb=0, prev_mlb=0;
static u8  mcycle=0;
static i8  mbuf[3]={0};

static void mouse_cmd_wait(void){
    u32 t=100000; while(t-- && (inb(MOUSE_STATUS)&0x02));
}
static void mouse_data_wait(void){
    u32 t=100000; while(t-- && !(inb(MOUSE_STATUS)&0x01));
}
static void mouse_write(u8 v){
    mouse_cmd_wait(); outb(MOUSE_CMD, 0xD4);
    mouse_cmd_wait(); outb(MOUSE_DATA, v);
}
static u8 mouse_read(void){
    mouse_data_wait(); return inb(MOUSE_DATA);
}

static void mouse_init(void){
    u8 cfg;
    mouse_cmd_wait(); outb(MOUSE_CMD, 0xA8); /* aux port aç */
    mouse_cmd_wait(); outb(MOUSE_CMD, 0x20);
    mouse_data_wait(); cfg = inb(MOUSE_DATA);
    cfg |= 0x02; cfg &= ~0x20;
    mouse_cmd_wait(); outb(MOUSE_CMD, 0x60);
    mouse_cmd_wait(); outb(MOUSE_DATA, cfg);
    mouse_write(0xFF); mouse_read(); mouse_read(); mouse_read(); /* reset */
    mouse_write(0xF6); mouse_read(); /* default */
    mouse_write(0xF4); mouse_read(); /* data reporting aç */
}

static void mouse_poll(void){
    while(1){
        u8 st = inb(MOUSE_STATUS);
        if(!(st & 0x01)) break;
        u8 data = inb(MOUSE_DATA);
        if(!(st & 0x20)){
            /* Klavye verisi – işleme */
            (void)data; continue;
        }
        switch(mcycle){
          case 0:
            if(!(data & 0x08)) break; /* senkronizasyon biti */
            mbuf[0]=(i8)data; mcycle=1; break;
          case 1: mbuf[1]=(i8)data; mcycle=2; break;
          case 2:
            mbuf[2]=(i8)data; mcycle=0;
            {
                i32 dx = mbuf[1]; if(mbuf[0]&0x10) dx|=0xFFFFFF00;
                i32 dy = mbuf[2]; if(mbuf[0]&0x20) dy|=0xFFFFFF00;
                mx += dx; my -= dy;
                if(mx<0) mx=0; if(my<0) my=0;
                if(mx>=(i32)SCR_W) mx=(i32)SCR_W-1;
                if(my>=(i32)SCR_H) my=(i32)SCR_H-1;
                prev_mlb = mlb;
                mlb = (mbuf[0]&0x01)?1:0;
                mrb = (mbuf[0]&0x02)?1:0;
            }
            break;
        }
    }
}

/* Tıklama tespiti: yeni sol tık + fare o bölgede mi? */
static int clicked(i32 x,i32 y,i32 w,i32 h){
    return (mlb && !prev_mlb && mx>=x && mx<x+w && my>=y && my<y+h);
}
/* Fare o bölgede mi (hover)? */
static int hovered(i32 x,i32 y,i32 w,i32 h){
    return (mx>=x && mx<x+w && my>=y && my<y+h);
}

/* =========================================================
   PCI TARAMA (USB/Depolama Algılama)
   ========================================================= */
#define PCI_ADDR 0xCF8
#define PCI_DATA 0xCFC

static u32 pci_read_cfg(u8 bus,u8 dev,u8 fn,u8 off){
    outl(PCI_ADDR, 0x80000000u|((u32)bus<<16)|((u32)dev<<11)|((u32)fn<<8)|(off&0xFC));
    return inl(PCI_DATA);
}

static int usb_detected      = 0;
static int storage_detected  = 0;
static char usb_dev_name[32] = "USB Surucu (8GB)";

static void pci_scan(void){
    for(int bus=0;bus<8;bus++){
        for(int dev=0;dev<32;dev++){
            u32 id = pci_read_cfg(bus,dev,0,0);
            if((id&0xFFFF)==0xFFFF) continue;
            u32 cls = pci_read_cfg(bus,dev,0,0x08);
            u8 cc = (u8)(cls>>24), sc = (u8)(cls>>16);
            if(cc==0x0C && sc==0x03) usb_detected=1;
            if(cc==0x01)             storage_detected=1;
        }
    }
}

/* =========================================================
   ORTAK UI BİLEŞENLERİ
   ========================================================= */

/* Düğme çiz + tıklama döndür */
static int draw_button(i32 x,i32 y,i32 w,i32 h,const char* lbl,u32 bg,u32 fg){
    u32 c = hovered(x,y,w,h) ? (bg==C_BLUE?0xFF1084DBu:0xFFD5D5D5u) : bg;
    fill_rrect(x,y,w,h,4,c);
    draw_str_c(x,y+(h-8)/2,w,lbl,fg,c,1);
    return clicked(x,y,w,h);
}

/* Toggle çiz + tıklama döndür */
static int draw_toggle(i32 x,i32 y,int on){
    u32 bg = on ? C_TOGGLE_ON : C_TOGGLE_OFF;
    fill_rrect(x,y,44,22,11,bg);
    i32 cx2 = on ? x+32 : x+11;
    draw_circle(cx2, y+11, 9, C_WHITE);
    return clicked(x,y,44,22);
}

/* Checkbox çiz + tıklama döndür */
static int draw_checkbox(i32 x,i32 y,int chk){
    fill_rect(x,y,18,18,C_WHITE);
    draw_rect_border(x,y,18,18, chk?C_BLUE:C_BORDER, 1);
    if(chk){
        /* Tik işareti */
        for(i32 i=0;i<4;i++){
            put_pixel(x+3+i, y+10+i, C_BLUE);
            put_pixel(x+4+i, y+10+i, C_BLUE);
        }
        for(i32 i=0;i<6;i++){
            put_pixel(x+7+i, y+13-i, C_BLUE);
            put_pixel(x+8+i, y+13-i, C_BLUE);
        }
    }
    return clicked(x,y,18,18);
}

/* Windows 4-renk logo */
static void draw_win_logo(i32 cx,i32 cy,i32 sz){
    i32 h=sz/2, g=2;
    fill_rect(cx-h,   cy-h,   h-g, h-g, 0xFFF35325u); /* kırmızı */
    fill_rect(cx+g,   cy-h,   h-g, h-g, 0xFF81BC06u); /* yeşil   */
    fill_rect(cx-h,   cy+g,   h-g, h-g, 0xFF05A6F0u); /* mavi    */
    fill_rect(cx+g,   cy+g,   h-g, h-g, 0xFFFFBA08u); /* sarı    */
}

/* İlerleme çubukları (7 nokta) */
static void draw_progress(int cur){
    i32 bar_y = (i32)SCR_H - 28;
    fill_rect(0, bar_y, (i32)SCR_W, 2, C_LIGHT_GRAY);
    i32 step = (i32)SCR_W / 9;
    for(int i=0;i<7;i++){
        i32 dx = step*(i+1);
        i32 dy = bar_y+12;
        if(i<cur)       draw_circle(dx,dy,5,C_BLUE);
        else if(i==cur){ draw_circle(dx,dy,7,C_BLUE); draw_circle(dx,dy,5,C_BLUE);}
        else {          draw_circle(dx,dy,5,C_LIGHT_GRAY); draw_circle(dx,dy,4,C_BG);}
    }
}

/* Alt logo şeridi */
static void draw_footer_bar(void){
    fill_rect(0,(i32)SCR_H-46,(i32)SCR_W,46,C_DARK_GRAY);
    draw_win_logo(28,(i32)SCR_H-23,18);
    draw_str(44,(i32)SCR_H-31,"Wind OS",C_WHITE,C_DARK_GRAY,1);
}

/* Ok imleci çiz */
static void draw_cursor(void){
    static const char arrow[16][12]={
        "X           ",
        "XX          ",
        "X.X         ",
        "X..X        ",
        "X...X       ",
        "X....X      ",
        "X.....X     ",
        "X......X    ",
        "X.......X   ",
        "X....XXXXX  ",
        "X..X..X     ",
        "X.X X..X    ",
        "XX  X..X    ",
        "X    X..X   ",
        "     X..X   ",
        "      XX    ",
    };
    for(int r=0;r<16;r++)
        for(int c=0;c<12;c++){
            i32 px=mx+c, py=my+r;
            if((u32)px>=SCR_W||(u32)py>=SCR_H) continue;
            if(arrow[r][c]=='X') put_pixel(px,py,C_BLACK);
            else if(arrow[r][c]=='.') put_pixel(px,py,C_WHITE);
        }
}

/* =========================================================
   UYGULAMA VERİLERİ
   ========================================================= */
typedef struct { char name[20]; int installed; u32 color; } App;
static App apps[8]={
    {"Mesajlar",  1, 0xFF0078D4u},
    {"Terminal",  1, 0xFF1A1A1Au},
    {"Kamera",    1, 0xFF107C10u},
    {"Harita",    1, 0xFFD13438u},
    {"Hesap M.",  1, 0xFF8B008Bu},
    {"Oyunlar",   0, 0xFFFF8C00u},
    {"Tarayici",  1, 0xFF005FB8u},
    {"Ayarlar",   1, 0xFF606060u},
};

typedef struct { char name[32]; int is_dir; } FSEntry;
static FSEntry local_fs[]={
    {"Masaustu",   1},{"Belgeler",  1},{"Indirmeler",1},
    {"Resimler",   1},{"Muzik",     1},{"wind_os.cfg",0},{"README.txt",0},
};
static FSEntry usb_fs[]={
    {"Kurulum.exe",0},{"Dosya1.exe",0},{"Notlar.txt",0},
};

/* =========================================================
   GLOBAL ARAYÜZ DURUMLARI
   ========================================================= */
/* Kurulum */
static char  pc_name[64]={0};
static int   pc_name_len=0;
static int   region_sel=0;
static int   kb_sel=0;
static int   net_sel=-1;
static char  wifi_pw[64]={0};
static int   wifi_pw_len=0;
static int   wifi_inputting=0;
static int   priv_feedback=1,priv_loc=1,priv_ads=1,priv_diag=0;
static int   cust_ent=1,cust_game=0,cust_work=0,cust_fam=0;

/* Masaüstü */
static int   drawer_open=0;
static int   fm_open=0;
static int   fm_in_usb=0;
static i32   fm_x=80, fm_y=60;
static int   fm_drag=0;
static i32   fm_drag_ox=0,fm_drag_oy=0;
static int   fm_sel=-1;
static int   usb_scan_tick=0;

/* =========================================================
   EKRAN 1: BİLGİSAYAR ADI
   ========================================================= */
static void screen1(u8 key){
    fill_rect(0,0,(i32)SCR_W,(i32)SCR_H,C_BG);

    /* Sol panel */
    fill_rect(0,0,380,(i32)SCR_H-46,C_HEADER);
    fill_rect(60,160,240,150,C_DARK_GRAY);
    fill_rect(64,164,232,142,0xFF1E2D45u);
    draw_circle(180,240,32,0xFF4FC3F7u);
    /* smiley */
    draw_circle(168,232,5,C_WHITE); draw_circle(192,232,5,C_WHITE);
    for(i32 i=-12;i<=12;i++) put_pixel(180+i, 252+(i*i)/20, C_WHITE);
    fill_rect(40,312,280,16,C_DARK_GRAY);

    /* Sağ panel */
    i32 rx=410, ry=70;
    draw_str(rx,ry,"Bilgisayariniza bir ad verin",C_DARK_GRAY,C_BG,2);
    ry+=40;
    draw_str(rx,ry,"Bu cihazi agda nasil tanimlayacaksiniz?",C_GRAY,C_BG,1);
    ry+=16; draw_str(rx,ry,"Ileride degistirebilirsiniz.",C_GRAY,C_BG,1);
    ry+=32;

    /* Input kutusu */
    fill_rect(rx,ry,520,38,C_INPUT_BG);
    draw_rect_border(rx,ry,520,38,C_BLUE,2);
    if(pc_name_len>0) draw_str(rx+10,ry+13,pc_name,C_DARK_GRAY,C_INPUT_BG,1);
    else draw_str(rx+10,ry+13,"Sunucu...",C_MID_GRAY,C_INPUT_BG,1);
    fill_rect(rx+10+pc_name_len*8, ry+9, 2,20,C_BLUE); /* imleç */

    ry+=52;
    draw_str(rx,ry,"19 karakterden az olmali.",C_GRAY,C_BG,1);
    ry+=56;

    if(draw_button(rx+320,ry,160,36,"Simdilik Atla",C_LIGHT_GRAY,C_DARK_GRAY))
        state=STATE_SETUP_2_REGION;
    if(draw_button(rx+490,ry,80,36,"Ileri",C_BLUE,C_WHITE))
        state=STATE_SETUP_2_REGION;

    draw_footer_bar(); draw_progress(0);

    /* Klavye girişi */
    if(key==8 && pc_name_len>0){ pc_name[--pc_name_len]=0; }
    else if(key>=32 && key<127 && pc_name_len<18){
        pc_name[pc_name_len++]=key; pc_name[pc_name_len]=0;
    }
}

/* =========================================================
   EKRAN 2: BÖLGE SEÇİMİ
   ========================================================= */
static void screen2(void){
    fill_rect(0,0,(i32)SCR_W,(i32)SCR_H,C_BG);

    /* Sol panel – dünya küre */
    fill_rect(0,0,380,(i32)SCR_H-46,C_HEADER);
    draw_circle(190,280,90,0xFF2196F3u);
    /* kıta yamalar */
    fill_rect(130,240,45,35,0xFF4CAF50u);
    fill_rect(200,250,55,45,0xFF4CAF50u);
    fill_rect(145,295,28,28,0xFF4CAF50u);
    fill_rect(215,290,38,38,0xFF4CAF50u);
    /* kürenin dışını temizle */
    for(i32 dy=-100;dy<=100;dy++)
        for(i32 dx=-100;dx<=100;dx++)
            if(dx*dx+dy*dy>90*90)
                put_pixel(190+dx,280+dy,C_HEADER);

    i32 rx=410, ry=80;
    draw_str(rx,ry,"Bu dogru ulke/bolge mi?",C_DARK_GRAY,C_BG,2);
    ry+=54;

    const char* regions[]={"Turkiye","Turkce Q","Turkce F"};
    for(int i=0;i<3;i++){
        u32 bg=(region_sel==i)?C_BLUE:C_WHITE;
        u32 fg=(region_sel==i)?C_WHITE:C_DARK_GRAY;
        fill_rect(rx,ry,520,42,bg);
        draw_rect_border(rx,ry,520,42,C_BORDER,1);
        draw_str(rx+16,ry+15,regions[i],fg,bg,1);
        if(clicked(rx,ry,520,42)) region_sel=i;
        ry+=48;
    }
    ry+=24;

    if(draw_button(rx+430,ry,80,36,"Evet",C_BLUE,C_WHITE))
        state=STATE_SETUP_3_KEYBOARD;

    draw_footer_bar(); draw_progress(1);
}

/* =========================================================
   EKRAN 3: KLAVYE DÜZENİ
   ========================================================= */
static void screen3(void){
    fill_rect(0,0,(i32)SCR_W,(i32)SCR_H,C_BG);
    fill_rect(0,0,380,(i32)SCR_H-46,C_HEADER);

    /* Klavye illüstrasyonu */
    const char* rows[]={"QWERTYUIOP","ASDFGHJKL","ZXCVBNM"};
    i32 off[]={0,12,24};
    for(int r=0;r<3;r++){
        i32 ry2=220+r*36;
        for(int k=0;rows[r][k];k++){
            i32 bx=40+off[r]+k*30, by=ry2;
            fill_rect(bx,by,26,28,C_WHITE);
            draw_rect_border(bx,by,26,28,C_MID_GRAY,1);
            char s[2]={(char)rows[r][k],0};
            draw_str(bx+9,by+9,s,C_DARK_GRAY,C_WHITE,1);
        }
    }

    i32 rx=410, ry=60;
    draw_str(rx,ry,"Bu, dogru klavye duzeni...",C_DARK_GRAY,C_BG,2);
    ry+=40;
    draw_str(rx,ry,"Dogru ise devam edin, degilse secin.",C_GRAY,C_BG,1);
    ry+=32;

    const char* layouts[]={"Layout","Layout A_BD","Layout 2-Dlame","Layout S-E","Layout S-G","Layout S-H"};
    for(int i=0;i<6;i++){
        u32 bg=(kb_sel==i)?C_LIGHT_BLUE:C_WHITE;
        fill_rect(rx,ry,520,34,bg);
        draw_rect_border(rx,ry,520,34,C_BORDER,1);
        draw_str(rx+12,ry+12,layouts[i],C_DARK_GRAY,bg,1);
        if(clicked(rx,ry,520,34)) kb_sel=i;
        ry+=38;
    }
    ry+=12;

    if(draw_button(rx+430,ry,80,36,"Evet",C_BLUE,C_WHITE))
        state=STATE_SETUP_4_NETWORK;

    draw_footer_bar(); draw_progress(2);
}

/* =========================================================
   EKRAN 4: AĞ / WiFi
   ========================================================= */
static void screen4(u8 key){
    fill_rect(0,0,(i32)SCR_W,(i32)SCR_H,C_BG);
    fill_rect(0,0,380,(i32)SCR_H-46,C_HEADER);

    /* WiFi yayları */
    i32 wcx=190, wcy=290;
    draw_circle(wcx,wcy+40,8,0xFF2196F3u);
    draw_wifi_arc(wcx,wcy+40,30,4,0xFF2196F3u);
    draw_wifi_arc(wcx,wcy+40,58,4,0xFF2196F3u);
    draw_wifi_arc(wcx,wcy+40,86,4,0xFF2196F3u);

    i32 rx=410, ry=60;
    draw_str(rx,ry,"Hadi sizi bir aga baglayalim",C_DARK_GRAY,C_BG,2);
    ry+=32; draw_str(rx,ry,"Baglanti olusturun veya mevcut aga baglanin.",C_GRAY,C_BG,1);
    ry+=44;

    const char* nets[]={"Sky.Net-Giga","Sky.Net-Giga (Connected)"};
    for(int i=0;i<2;i++){
        u32 bg=(net_sel==i)?C_LIGHT_BLUE:C_WHITE;
        fill_rect(rx,ry,520,50,bg);
        draw_rect_border(rx,ry,520,50,C_BORDER,1);
        draw_circle(rx+24,ry+25,5,0xFF2196F3u);
        draw_str(rx+42,ry+19,nets[i],C_DARK_GRAY,bg,1);
        if(clicked(rx,ry,520,50)){ net_sel=i; wifi_inputting=1; }
        ry+=56;
    }

    if(wifi_inputting && net_sel>=0){
        ry+=8;
        draw_str(rx,ry,"Parola:",C_DARK_GRAY,C_BG,1); ry+=20;
        fill_rect(rx,ry,520,36,C_WHITE);
        draw_rect_border(rx,ry,520,36,C_BLUE,2);
        char dots[64]={0};
        for(int i=0;i<wifi_pw_len&&i<48;i++) dots[i]='*';
        draw_str(rx+10,ry+12,dots,C_DARK_GRAY,C_WHITE,1);
        fill_rect(rx+10+wifi_pw_len*8,ry+8,2,20,C_BLUE);
        ry+=50;
        draw_button(rx+310,ry,100,36,"Iptal",C_LIGHT_GRAY,C_DARK_GRAY);
        if(draw_button(rx+420,ry,100,36,"Baglan",C_BLUE,C_WHITE)){
            state=STATE_SETUP_5_PRIVACY;
            wifi_inputting=0;
        }
        /* şifre girişi */
        if(key==8 && wifi_pw_len>0){ wifi_pw[--wifi_pw_len]=0; }
        else if(key>=32&&key<127&&wifi_pw_len<32){ wifi_pw[wifi_pw_len++]=key; wifi_pw[wifi_pw_len]=0; }
    } else {
        ry+=12;
        draw_button(rx+290,ry,110,36,"Aglar Goster",C_LIGHT_GRAY,C_DARK_GRAY);
        if(draw_button(rx+410,ry,110,36,"Ilerle",C_BLUE,C_WHITE))
            state=STATE_SETUP_5_PRIVACY;
    }

    draw_footer_bar(); draw_progress(3);
}

/* =========================================================
   EKRAN 5: GİZLİLİK AYARLARI
   ========================================================= */
static void screen5(void){
    fill_rect(0,0,(i32)SCR_W,(i32)SCR_H,C_BG);
    fill_rect(0,0,380,(i32)SCR_H-46,C_HEADER);

    /* Kalkan illüstrasyonu */
    i32 sx=190,sy=280;
    for(i32 dy=-80;dy<=80;dy++){
        i32 hw = (dy<0) ? 70 : 70-dy;
        if(hw<0) hw=0;
        fill_rect(sx-hw,sy+dy,hw*2,1,0xFF1565C0u);
    }
    draw_circle(sx,sy,20,0xFFFFD700u);
    fill_rect(sx-7,sy-10,14,20,0xFF1565C0u); /* kilit gövde boşluğu */
    fill_rect(sx-5,sy-22,10,16,0xFF1565C0u);
    draw_rect_border(sx-5,sy-22,10,18,0xFFFFD700u,3);

    i32 rx=410, ry=50;
    draw_str(rx,ry,"Cihaziniz icin gizlilik",C_DARK_GRAY,C_BG,2);
    ry+=22; draw_str(rx,ry,"ayarlarini secin",C_DARK_GRAY,C_BG,2);
    ry+=36;
    draw_str(rx,ry,"Asagidaki ozellikleri acip kapatabilirsiniz.",C_GRAY,C_BG,1);
    ry+=32;

    typedef struct{const char* nm;const char* desc;int* val;} PI;
    PI items[]={
        {"Geri bildirim",   "Sistem geri bildirimlerini etkin/pasif yapin",&priv_feedback},
        {"Konum",           "Konumunuzu uygulamalar ile paylasin",         &priv_loc},
        {"Reklam kimligi",  "Kisiye ozel reklamlar icin ID kullan",        &priv_ads},
        {"Teshis Sinyali",  "Teshis verisini Microsoft ile paylasin",      &priv_diag},
    };

    for(int i=0;i<4;i++){
        fill_rect(rx,ry,520,56,C_CARD);
        draw_rect_border(rx,ry,520,56,C_BORDER,1);
        draw_str(rx+12,ry+10,items[i].nm,  C_DARK_GRAY,C_CARD,1);
        draw_str(rx+12,ry+24,items[i].desc,C_GRAY,    C_CARD,1);
        draw_str(rx+420,ry+22,*items[i].val?"Evet":"Hayir",
                 *items[i].val?C_BLUE:C_GRAY, C_CARD, 1);
        if(draw_toggle(rx+464,ry+17,*items[i].val)) *items[i].val=!*items[i].val;
        ry+=60;
    }
    ry+=10;
    draw_str(rx,ry,"Daha fazla bilgi edinin",C_BLUE,C_BG,1);
    ry+=28;
    if(draw_button(rx+430,ry,90,36,"Kabul Et",C_BLUE,C_WHITE))
        state=STATE_SETUP_6_CUSTOMIZE;

    draw_footer_bar(); draw_progress(4);
}

/* =========================================================
   EKRAN 6: DENEYİM ÖZELLEŞTIRME
   ========================================================= */
static void screen6(void){
    fill_rect(0,0,(i32)SCR_W,(i32)SCR_H,C_BG);
    fill_rect(0,0,380,(i32)SCR_H-46,C_HEADER);

    /* Dekoratif ikonlar */
    u32 icols[]={0xFF2196F3u,0xFF4CAF50u,0xFFFF9800u,0xFF9C27B0u};
    const char* ilbls[]={"ENT","GAM","OKL","FAM"};
    i32 ipos[][2]={{80,220},{240,200},{80,320},{240,300}};
    for(int i=0;i<4;i++){
        draw_circle(ipos[i][0],ipos[i][1],32,icols[i]);
        draw_str_c(ipos[i][0]-24,ipos[i][1]-4,48,ilbls[i],C_WHITE,icols[i],1);
    }

    i32 rx=410, ry=50;
    draw_str(rx,ry,"Deneyiminizi ozellestirelim",C_DARK_GRAY,C_BG,2);
    ry+=28;
    draw_str(rx,ry,"Microsoft, sectiginiz etkinliklere gore ipuclari,",C_GRAY,C_BG,1);
    ry+=16; draw_str(rx,ry,"reklamlar ve oneriler sunabilir.",C_GRAY,C_BG,1);
    ry+=30;

    typedef struct{const char* nm;const char* desc;int* val;u32 col;} CI;
    CI citems[]={
        {"Eglence","Video, muzik ve diger medyayi kesfet",&cust_ent, 0xFF2196F3u},
        {"Oyun",   "Oyunlari ve yeni surumleri takip et",&cust_game,0xFF4CAF50u},
        {"Okul",   "Odev ve projelerde verimli calis",   &cust_work,0xFFFF9800u},
        {"Aile",   "Aile uyeleriyle guvenli kalin",      &cust_fam, 0xFF9C27B0u},
    };
    for(int i=0;i<4;i++){
        fill_rect(rx,ry,520,56,C_CARD);
        draw_rect_border(rx,ry,520,56,C_BORDER,1);
        draw_circle(rx+28,ry+28,18,citems[i].col);
        draw_str(rx+56,ry+12,citems[i].nm,  C_DARK_GRAY,C_CARD,1);
        draw_str(rx+56,ry+26,citems[i].desc,C_GRAY,    C_CARD,1);
        if(draw_checkbox(rx+490,ry+18,*citems[i].val)) *citems[i].val=!*citems[i].val;
        ry+=60;
    }
    ry+=10;

    draw_button(rx+290,ry,110,36,"Atla",C_LIGHT_GRAY,C_DARK_GRAY);
    if(draw_button(rx+410,ry,120,36,"Kabul Et",C_BLUE,C_WHITE))
        state=STATE_SETUP_7_WELCOME;

    draw_footer_bar(); draw_progress(5);
}

/* =========================================================
   EKRAN 7: HOŞ GELDİNİZ
   ========================================================= */
static void screen7(void){
    fill_rect(0,0,(i32)SCR_W,(i32)SCR_H,C_BG);
    fill_rect(0,0,380,(i32)SCR_H-46,C_HEADER);
    draw_footer_bar();

    /* Bildirim kartı */
    i32 cx=(i32)SCR_W/2-210, cy=80;
    fill_rect(cx+4,cy+4,420,210,C_SHADOW);
    fill_rect(cx,cy,420,210,C_DARK_GRAY);
    draw_rect_border(cx,cy,420,210,0xFF444466u,1);

    /* kapat X */
    fill_rect(cx+396,cy+4,20,20,C_RED);
    draw_str_c(cx+396,cy+8,20,"X",C_WHITE,C_RED,1);
    draw_circle(cx+16,cy+18,7,C_BLUE);
    draw_str(cx+28,cy+12,"HOS GELDLNLZ -",C_WHITE,C_DARK_GRAY,1);
    draw_str(cx+12,cy+46,"Sisteme Hos Geldiniz!",C_WHITE,C_DARK_GRAY,1);
    draw_str(cx+12,cy+64,"Wind OS hazir.",C_MID_GRAY,C_DARK_GRAY,1);

    /* Uygulama ikonları */
    u32 app_cols[]={C_BLUE,C_GREEN,0xFF9C27B0u,C_RED};
    const char* app_lbls[]={"MSG","TRM","HAR","KMR"};
    for(int i=0;i<4;i++){
        i32 ax=cx+12+i*102, ay=cy+120;
        fill_rrect(ax,ay,88,68,6,app_cols[i]);
        draw_str_c(ax,ay+30,88,app_lbls[i],C_WHITE,app_cols[i],1);
    }

    /* Ana başlık */
    draw_str_c(0,340,(i32)SCR_W,"Wind OS'e Hos Geldlnlz!",C_DARK_GRAY,C_BG,2);
    draw_str_c(0,380,(i32)SCR_W,
               pc_name[0]?pc_name:"Kullanici", C_GRAY,C_BG,1);

    if(draw_button((i32)SCR_W/2-110,440,220,44,"Masaustu Gir",C_BLUE,C_WHITE)){
        pci_scan();
        state=STATE_DESKTOP;
    }
    draw_progress(6);
}

/* =========================================================
   MASAÜSTÜ – UYGULAMA ÇEKMECESİ
   ========================================================= */
static void draw_drawer(void){
    if(!drawer_open) return;
    i32 dh=420, dx=80, dy=(i32)SCR_H-50-dh;
    i32 dw=(i32)SCR_W-160;

    fill_rrect(dx,dy,dw,dh,8,0xEE1E1E2Eu);
    draw_rect_border(dx,dy,dw,dh,0xFF3A3A5Au,1);

    draw_str_c(dx,dy+16,dw,"UYGULAMALAR",C_WHITE,0xFF1E1E2Eu,1);
    draw_str_c(dx,dy+32,dw,"Kurmak icin uygulamaya tiklayin",C_GRAY,0xFF1E1E2Eu,1);

    for(int i=0;i<8;i++){
        i32 col=i%4, row=i/4;
        i32 ix=dx+20+col*((dw-40)/4), iy=dy+60+row*130;
        u32 bg=apps[i].installed?apps[i].color:0xFF3A3A3Au;
        fill_rrect(ix,iy,72,72,10,bg);
        char ab[3]={apps[i].name[0],apps[i].name[1],0};
        draw_str_c(ix,iy+32,72,ab,C_WHITE,bg,1);
        draw_str_c(ix-8,iy+78,88,apps[i].name,C_WHITE,0xFF1E1E2Eu,1);
        if(!apps[i].installed){
            draw_str_c(ix,iy+92,72,"[Yukle]",C_BLUE,0xFF1E1E2Eu,1);
            if(clicked(ix,iy+88,72,14)) apps[i].installed=1;
        }
    }
}

/* =========================================================
   MASAÜSTÜ – DOSYA YÖNETİCİSİ
   ========================================================= */
static void draw_fm(void){
    if(!fm_open) return;
    i32 fw=620, fh=420;
    i32 fx=fm_x, fy=fm_y;

    /* Pencere */
    fill_rect(fx+5,fy+5,fw,fh,0xAAAAAAu);
    fill_rect(fx,fy,fw,fh,C_WHITE);
    draw_rect_border(fx,fy,fw,fh,C_BORDER,1);

    /* Başlık çubuğu */
    fill_rect(fx,fy,fw,34,C_DARK_GRAY);
    draw_circle(fx+14,fy+17,7,C_RED);
    draw_circle(fx+32,fy+17,7,0xFFFF9500u);
    draw_circle(fx+50,fy+17,7,C_GREEN);
    draw_str(fx+68,fy+12,"Dosya Yoneticisi",C_WHITE,C_DARK_GRAY,1);

    /* Kapat */
    if(clicked(fx+7,fy+10,14,14)){ fm_open=0; return; }

    /* Adres çubuğu */
    fill_rect(fx,fy+34,fw,28,C_LIGHT_GRAY);
    if(fm_in_usb){
        draw_str(fx+8,fy+42,"Bu Bilgisayar > USB Surucu",C_DARK_GRAY,C_LIGHT_GRAY,1);
        if(clicked(fx+8,fy+40,120,14)) fm_in_usb=0;
    } else {
        draw_str(fx+8,fy+42,"Bu Bilgisayar",C_DARK_GRAY,C_LIGHT_GRAY,1);
    }

    /* Sol kenar çubugu */
    i32 sb=120;
    fill_rect(fx,fy+62,sb,(i32)fh-62,C_HEADER);
    const char* sidebar[]={"Bu Bilgisayar","Masaustu","Belgeler","Resimler","Muzik"};
    for(int i=0;i<5;i++)
        draw_str(fx+8,fy+72+i*18,sidebar[i],C_DARK_GRAY,C_HEADER,1);

    if(usb_detected){
        draw_circle(fx+14,fy+170,6,0xFF2196F3u);
        draw_str(fx+24,fy+165,usb_dev_name,C_BLUE,C_HEADER,1);
        if(clicked(fx+8,fy+160,sb-8,20)) fm_in_usb=1;
    }

    /* İçerik alanı */
    i32 cx2=fx+sb+8, cy2=fy+68;
    FSEntry* entries = fm_in_usb ? usb_fs : local_fs;
    int count = fm_in_usb ? 3 : 7;

    for(int i=0;i<count;i++){
        i32 ex=cx2+(i%4)*118, ey=cy2+(i/4)*100;
        if(ex+100>fx+fw || ey+92>fy+fh) continue;

        u32 bg=(fm_sel==i)?C_LIGHT_BLUE:C_WHITE;
        fill_rect(ex,ey,100,88,bg);
        draw_rect_border(ex,ey,100,88,C_BORDER,1);

        if(entries[i].is_dir){
            fill_rect(ex+18,ey+8,56,16,0xFFFFD700u);
            fill_rect(ex+10,ey+20,70,40,0xFFFFE44Du);
        } else {
            fill_rect(ex+22,ey+8,44,48,C_WHITE);
            draw_rect_border(ex+22,ey+8,44,48,C_MID_GRAY,1);
            fill_rect(ex+54,ey+8,12,12,C_LIGHT_GRAY);
            /* .exe kırmızı etiket */
            const char* n=entries[i].name;
            int nl=(int)kstrlen(n);
            if(nl>4&&n[nl-4]=='.'&&n[nl-3]=='e'&&n[nl-2]=='x'&&n[nl-1]=='e'){
                fill_rect(ex+23,ey+34,42,12,C_RED);
                draw_str(ex+27,ey+36,".exe",C_WHITE,C_RED,1);
            }
        }

        /* Ad (kırp) */
        char sname[12]={0};
        int nl=(int)kstrlen(entries[i].name);
        if(nl>10){ memcpy_k(sname,entries[i].name,9); sname[9]='.'; sname[10]='.'; }
        else kstrcpy(sname,entries[i].name);
        draw_str(ex+4,ey+72,sname,C_DARK_GRAY,bg,1);

        if(clicked(ex,ey,100,88)){
            fm_sel=i;
            /* .exe tıklama = "Yukle" */
            const char* nm=entries[i].name;
            int nln=(int)kstrlen(nm);
            if(!entries[i].is_dir && nln>4 &&
               nm[nln-4]=='.'&&nm[nln-3]=='e'&&nm[nln-2]=='x'&&nm[nln-1]=='e'){
                for(int a=0;a<8;a++) if(!apps[a].installed){apps[a].installed=1;break;}
            }
        }
    }

    /* Durum çubuğu */
    fill_rect(fx,fy+fh-20,fw,20,C_LIGHT_GRAY);
    draw_str(fx+8,fy+fh-14,
             usb_detected?"USB takili":"USB takili degil",
             C_GRAY,C_LIGHT_GRAY,1);

    /* Pencere sürükleme */
    if(!fm_drag && mlb && !prev_mlb &&
       my>=fy && my<fy+34 && mx>=fx && mx<fx+fw){
        fm_drag=1; fm_drag_ox=mx-fx; fm_drag_oy=my-fy;
    }
    if(fm_drag){
        if(mlb){
            fm_x=mx-fm_drag_ox; fm_y=my-fm_drag_oy;
            if(fm_x<0) fm_x=0; if(fm_y<0) fm_y=0;
            if(fm_x>(i32)SCR_W-fw) fm_x=(i32)SCR_W-fw;
            if(fm_y>(i32)SCR_H-fh) fm_y=(i32)SCR_H-fh;
        } else fm_drag=0;
    }
}

/* =========================================================
   MASAÜSTÜ
   ========================================================= */
static void draw_desktop(void){
    /* Arkaplan gradyanı */
    for(i32 y=0;y<(i32)SCR_H-50;y++){
        u32 r=0x1A+(y*10)/SCR_H;
        u32 g=0x1A+(y*5)/SCR_H;
        u32 b=0x2E+(y*20)/SCR_H;
        fill_rect(0,y,(i32)SCR_W,1,0xFF000000u|(r<<16)|(g<<8)|b);
    }

    /* Filigran */
    draw_str_c(0,(i32)SCR_H/2-10,(i32)SCR_W,"Wind OS",0xFF2A2A50u,0x00000000u,2);

    /* Hava durumu widget */
    fill_rrect((i32)SCR_W-190,10,180,70,6,0xAA1E2040u);
    draw_str((i32)SCR_W-185,18,"Hava Durumu ve Saat",C_WHITE,0xFF1E2040u,1);
    draw_str((i32)SCR_W-185,32,"Istanbul:  22°C",C_WHITE,0xFF1E2040u,1);
    draw_str((i32)SCR_W-185,48,"Prtly Cloudy",C_MID_GRAY,0xFF1E2040u,1);
    draw_str((i32)SCR_W-185,62,"26:03  01.10.27",C_WHITE,0xFF1E2040u,1);

    /* Uygulama çekmecesi */
    draw_drawer();

    /* Dosya yöneticisi */
    draw_fm();

    /* ── GÖREV ÇUBUĞU ── */
    fill_rect(0,(i32)SCR_H-50,(i32)SCR_W,50,C_TASKBAR);

    /* Windows logo butonu */
    draw_win_logo(26,(i32)SCR_H-25,20);
    if(clicked(8,(i32)SCR_H-44,36,38)) drawer_open=!drawer_open;

    /* Görev çubuğu uygulama düğmeleri */
    const char* tb_lbls[]={"FM","TB","MS","TM"};
    for(int i=0;i<4;i++){
        i32 tx=64+i*52, ty=(i32)SCR_H-44;
        u32 bg=(i==0&&fm_open)?C_TASKBAR_HL:C_TASKBAR;
        fill_rrect(tx,ty,44,34,4,bg);
        draw_str_c(tx,ty+12,44,tb_lbls[i],C_WHITE,bg,1);
        if(clicked(tx,ty,44,34)){
            if(i==0){ fm_open=!fm_open; fm_sel=-1; }
        }
    }

    /* Saat / tarih */
    draw_str((i32)SCR_W-90,(i32)SCR_H-38,"26:03",C_WHITE,C_TASKBAR,1);
    draw_str((i32)SCR_W-96,(i32)SCR_H-22,"01.10.27",C_WHITE,C_TASKBAR,1);

    /* USB göstergesi */
    if(usb_detected){
        draw_circle((i32)SCR_W-110,(i32)SCR_H-25,5,0xFF2196F3u);
        draw_str((i32)SCR_W-103,(i32)SCR_H-29,"USB",C_WHITE,C_TASKBAR,1);
    }

    /* USB periyodik yeniden tarama */
    if(++usb_scan_tick>2000){ pci_scan(); usb_scan_tick=0; }
}

/* =========================================================
   GECIKTIRME
   ========================================================= */
static void delay_ms(int n){
    volatile int x=n*8000; while(x--) __asm__ volatile("nop");
}

/* =========================================================
   KERNEL_MAIN
   ========================================================= */
void kernel_main(multiboot_info_t* mbi){
    /* Framebuffer kur */
    FB        = (u32*)(unsigned long)mbi->framebuffer_addr;
    SCR_W     = mbi->framebuffer_width;
    SCR_H     = mbi->framebuffer_height;
    SCR_PITCH = mbi->framebuffer_pitch / 4;

    /* GRUB framebuffer atamadıysa yedek */
    if(!FB || SCR_W==0){
        FB       = (u32*)0xFD000000u;
        SCR_W    = 1024;
        SCR_H    = 768;
        SCR_PITCH= 1024;
    }

    /* PS/2 Mouse başlat */
    mouse_init();

    /* İlk PCI tarama */
    pci_scan();

    /* Ana döngü */
    while(1){
        /* Mouse güncelle */
        mouse_poll();

        /* Klavye oku */
        u8 key = kbd_poll();

        /* Durum makinesini çalıştır ve çiz */
        switch(state){
          case STATE_SETUP_1_NAME:      screen1(key);  break;
          case STATE_SETUP_2_REGION:    screen2();     break;
          case STATE_SETUP_3_KEYBOARD:  screen3();     break;
          case STATE_SETUP_4_NETWORK:   screen4(key);  break;
          case STATE_SETUP_5_PRIVACY:   screen5();     break;
          case STATE_SETUP_6_CUSTOMIZE: screen6();     break;
          case STATE_SETUP_7_WELCOME:   screen7();     break;
          case STATE_DESKTOP:           draw_desktop();break;
        }

        /* İmleç çiz (en üste) */
        draw_cursor();

        delay_ms(5);
    }
}
