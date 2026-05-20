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
   TAMSAYI KÖK (isqrt)
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

static void draw_wifi_arc(i32 cx, i32 cy, i32 r, i32 thick, u32 col){
    for(i32 dx2=-r; dx2<=r; dx2++){
        u32 dy_sq = (u32)(r*r - dx2*dx2);
        i32 dy2 = -(i32)isqrt(dy_sq);
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

static u8 kbd_poll(void){
    if(!(inb(KBD_STATUS) & 0x01)) return 0;
    u8 sc = inb(KBD_DATA);
    if(inb(KBD_STATUS) & 0x20) return 0; /* Mouse verisi ise atla */

    if(sc & 0x80){ /* Tuş bırakıldı */
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
    mouse_cmd_wait(); outb(MOUSE_CMD, 0xA8); /* Aux port aç */
    mouse_cmd_wait(); outb(MOUSE_CMD, 0x20);
    mouse_data_wait(); cfg = inb(MOUSE_DATA);
    cfg |= 0x02; cfg &= ~0x20;
    mouse_cmd_wait(); outb(MOUSE_CMD, 0x60);
    mouse_cmd_wait(); outb(MOUSE_DATA, cfg);
    mouse_write(0xFF); mouse_read(); mouse_read(); mouse_read(); /* Reset */
    mouse_write(0xF6); mouse_read(); /* Default */
    mouse_write(0xF4); mouse_read(); /* Data reporting aktif et */
}

static void mouse_poll(void){
    while(1){
        u8 st = inb(MOUSE_STATUS);
        if(!(st & 0x01)) break;
        u8 data = inb(MOUSE_DATA);
        if(!(st & 0x20)){
            continue;
        }
        switch(mcycle){
          case 0:
            if(!(data & 0x08)) break; /* Senkronizasyon kontrolü */
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

static int clicked(i32 x,i32 y,i32 w,i32 h){
    return (mlb && !prev_mlb && mx>=x && mx<x+w && my>=y && my<y+h);
}
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
static char usb_dev_name[32] = "USB Bellek (8GB)";

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
   ORTAK UI DEĞİŞKENLERİ VE BİLEŞENLERİ
   ========================================================= */
static char pc_name[64] = "";
static int pc_name_len = 0;
static char wifi_pw[64] = "";
static int wifi_pw_len = 0;
static int selected_region = 0;
static int selected_layout = 0;
static int privacy_1 = 1;
static int privacy_2 = 1;
static int privacy_3 = 0;
static int update_progress = 0;

static int drawer_open=0;
static int fm_open=0;
static int fm_in_usb=0;
static i32 fm_x=80, fm_y=60;
static int fm_sel=-1;
static int usb_scan_tick=0;

static int draw_button(i32 x,i32 y,i32 w,i32 h,const char* lbl,u32 bg,u32 fg){
    u32 c = hovered(x,y,w,h) ? (bg==C_BLUE?0xFF1084DBu:0xFFD5D5D5u) : bg;
    fill_rrect(x,y,w,h,4,c);
    draw_str_c(x,y+(h-8)/2,w,lbl,fg,c,1);
    return clicked(x,y,w,h);
}

static int draw_toggle(i32 x,i32 y,int on){
    u32 bg = on ? C_TOGGLE_ON : C_TOGGLE_OFF;
    fill_rrect(x,y,44,22,11,bg);
    i32 cx2 = on ? x+32 : x+11;
    draw_circle(cx2, y+11, 9, C_WHITE);
    return clicked(x,y,44,22);
}

static int draw_checkbox(i32 x,i32 y,int chk){
    fill_rect(x,y,18,18,C_WHITE);
    draw_rect_border(x,y,18,18, chk?C_BLUE:C_BORDER, 1);
    if(chk){
        for(i32 i=0;i<4;i++){ put_pixel(x+3+i, y+10+i, C_BLUE); put_pixel(x+4+i, y+10+i, C_BLUE); }
        for(i32 i=0;i<6;i++){ put_pixel(x+7+i, y+13-i, C_BLUE); put_pixel(x+8+i, y+13-i, C_BLUE); }
    }
    return clicked(x,y,18,18);
}

static void draw_win_logo(i32 cx,i32 cy,i32 sz){
    i32 h=sz/2, g=2;
    fill_rect(cx-h, cy-h, h-g, h-g, 0xFFF35325u); /* Kırmızı */
    fill_rect(cx+g, cy-h, h-g, h-g, 0xFF81BC06u); /* Yeşil */
    fill_rect(cx-h, cy+g, h-g, h-g, 0xFF05A6F0u); /* Mavi */
    fill_rect(cx+g, cy+g, h-g, h-g, 0xFFFFBA08u); /* Sarı */
}

static void draw_progress(i32 step){
    i32 start_x = 410, start_y = 700;
    for(i32 i=0; i<7; i++){
        u32 col = (i <= step) ? C_BLUE : C_LIGHT_GRAY;
        draw_circle(start_x + i*24, start_y, 6, col);
    }
}

static void draw_footer_bar(void){
    fill_rect(0, (i32)SCR_H-46, (i32)SCR_W, 46, C_HEADER);
    draw_rect_border(0, (i32)SCR_H-46, (i32)SCR_W, 46, C_BORDER, 1);
    draw_str(20, (i32)SCR_H-28, "Wind OS Kurulum Sihirbazi", C_DARK_GRAY, C_HEADER, 1);
}

/* =========================================================
   7 OOBE KURULUM EKRANLARI VE MASAÜSTÜ MODÜLLERİ
   ========================================================= */

/* EKRAN 1: BİLGİSAYAR ADI */
static void screen1(u8 key){
    fill_rect(0,0,(i32)SCR_W,(i32)SCR_H,C_BG);
    fill_rect(0,0,380,(i32)SCR_H-46,C_HEADER);
    fill_rect(60,160,240,150,C_DARK_GRAY);
    fill_rect(64,164,232,142,0xFF1E2D45u);
    draw_circle(180,240,32,0xFF4FC3F7u);
    draw_circle(168,232,5,C_WHITE);
    draw_circle(192,232,5,C_WHITE);
    for(i32 i=-12;i<=12;i++) put_pixel(180+i, 252+(i*i)/20, C_WHITE);
    fill_rect(40,312,280,16,C_DARK_GRAY);
    
    i32 rx=410, ry=70;
    draw_str(rx,ry,"Bilgisayariniza bir ad verin",C_DARK_GRAY,C_BG,2); ry+=40;
    draw_str(rx,ry,"Bu cihazi agda nasil tanimlayacaksiniz?",C_GRAY,C_BG,1); ry+=16;
    draw_str(rx,ry,"Ileride degistirebilirsiniz.",C_GRAY,C_BG,1); ry+=32;
    
    fill_rect(rx,ry,520,38,C_INPUT_BG);
    draw_rect_border(rx,ry,520,38,C_BLUE,2);
    if(pc_name_len>0) draw_str(rx+10,ry+13,pc_name,C_DARK_GRAY,C_INPUT_BG,1);
    else draw_str(rx+10,ry+13,"Cihaz Adi Girin...",C_MID_GRAY,C_INPUT_BG,1);
    fill_rect(rx+10+pc_name_len*8, ry+9, 2,20,C_BLUE);
    
    if(key==8 && pc_name_len>0){ pc_name[--pc_name_len]=0; }
    else if(key>=32 && key<127 && pc_name_len<31){ pc_name[pc_name_len++]=key; pc_name[pc_name_len]=0; }
    
    ry+=80;
    if(draw_button(rx+410,ry,110,36,"Ilerle",C_BLUE,C_WHITE)) state=STATE_SETUP_2_REGION;
    draw_footer_bar();
    draw_progress(0);
}

/* EKRAN 2: BÖLGE AYARI */
static void screen2(void){
    fill_rect(0,0,(i32)SCR_W,(i32)SCR_H,C_BG);
    fill_rect(0,0,380,(i32)SCR_H-46,C_HEADER);
    draw_str(60,220,"Bölge Ayari",C_DARK_GRAY,C_HEADER,2);
    
    i32 rx=410, ry=70;
    draw_str(rx,ry,"Bölgenizi Secin",C_DARK_GRAY,C_BG,2); ry+=40;
    
    const char* regions[] = {"Türkiye", "Amerika Birlesik Devletleri", "Almanya", "Ingiltere"};
    for(int i=0; i<4; i++){
        u32 bg = (selected_region == i) ? C_LIGHT_BLUE : C_WHITE;
        u32 border = (selected_region == i) ? C_BLUE : C_BORDER;
        fill_rect(rx, ry, 520, 40, bg);
        draw_rect_border(rx, ry, 520, 40, border, 1);
        draw_str(rx+15, ry+16, regions[i], C_DARK_GRAY, bg, 1);
        if(clicked(rx, ry, 520, 40)) selected_region = i;
        ry+=50;
    }
    ry+=20;
    if(draw_button(rx+410,ry,110,36,"Ilerle",C_BLUE,C_WHITE)) state=STATE_SETUP_3_KEYBOARD;
    draw_footer_bar();
    draw_progress(1);
}

/* EKRAN 3: KLAVYE DÜZENİ */
static void screen3(void){
    fill_rect(0,0,(i32)SCR_W,(i32)SCR_H,C_BG);
    fill_rect(0,0,380,(i32)SCR_H-46,C_HEADER);
    draw_str(60,220,"Klavye Secimi",C_DARK_GRAY,C_HEADER,2);
    
    i32 rx=410, ry=70;
    draw_str(rx,ry,"Klavye Düzeninizi Secin",C_DARK_GRAY,C_BG,2); ry+=40;
    
    const char* layouts[] = {"Türkçe Q Klavye", "Türkçe F Klavye", "English QWERTY"};
    for(int i=0; i<3; i++){
        u32 bg = (selected_layout == i) ? C_LIGHT_BLUE : C_WHITE;
        u32 border = (selected_layout == i) ? C_BLUE : C_BORDER;
        fill_rect(rx, ry, 520, 40, bg);
        draw_rect_border(rx, ry, 520, 40, border, 1);
        draw_str(rx+15, ry+16, layouts[i], C_DARK_GRAY, bg, 1);
        if(clicked(rx, ry, 520, 40)) selected_layout = i;
        ry+=50;
    }
    ry+=20;
    if(draw_button(rx+410,ry,110,36,"Ilerle",C_BLUE,C_WHITE)) state=STATE_SETUP_4_NETWORK;
    draw_footer_bar();
    draw_progress(2);
}

/* EKRAN 4: İNTERNET BAĞLANTISI */
static void screen4(u8 key){
    fill_rect(0,0,(i32)SCR_W,(i32)SCR_H,C_BG);
    fill_rect(0,0,380,(i32)SCR_H-46,C_HEADER);
    
    i32 cx=190, cy=220;
    draw_wifi_arc(cx, cy, 40, 4, C_BLUE);
    draw_wifi_arc(cx, cy, 25, 4, C_BLUE);
    draw_wifi_arc(cx, cy, 10, 4, C_BLUE);
    draw_circle(cx, cy+5, 4, C_BLUE);
    
    i32 rx=410, ry=70;
    draw_str(rx,ry,"Bir aga baglanin",C_DARK_GRAY,C_BG,2); ry+=40;
    draw_str(rx,ry,"Sisteminizi kurmak icin Wi-Fi baglantisi gerekir.",C_GRAY,C_BG,1); ry+=32;
    
    fill_rect(rx,ry,520,38,C_INPUT_BG);
    draw_rect_border(rx,ry,520,38,C_BLUE,2);
    if(wifi_pw_len>0) draw_str(rx+10,ry+13,wifi_pw,C_DARK_GRAY,C_INPUT_BG,1);
    else draw_str(rx+10,ry+13,"Wi-Fi Sifresi Girin...",C_MID_GRAY,C_INPUT_BG,1);
    fill_rect(rx+10+wifi_pw_len*8, ry+9, 2,20,C_BLUE);
    
    if(key==8 && wifi_pw_len>0){ wifi_pw[--wifi_pw_len]=0; }
    else if(key>=32 && key<127 && wifi_pw_len<31){ wifi_pw[wifi_pw_len++]=key; wifi_pw[wifi_pw_len]=0; }
    
    ry+=60;
    if(draw_button(rx+410,ry,110,36,"Ilerle",C_BLUE,C_WHITE)) state=STATE_SETUP_5_PRIVACY;
    draw_footer_bar();
    draw_progress(3);
}

/* EKRAN 5: GİZLİLİK AYARLARI */
static void screen5(void){
    fill_rect(0,0,(i32)SCR_W,(i32)SCR_H,C_BG);
    fill_rect(0,0,380,(i32)SCR_H-46,C_HEADER);
    
    i32 sx=190,sy=280;
    for(i32 dy=-80;dy<=80;dy++){
        i32 hw = (dy<0) ? 70 : 70-dy; if(hw<0) hw=0;
        fill_rect(sx-hw,sy+dy,hw*2,1,0xFF1565C0u);
    }
    draw_circle(sx,sy,20,0xFFFFD700u);
    fill_rect(sx-7,sy-10,14,20,0xFF1565C0u);
    fill_rect(sx-5,sy-22,10,16,0xFF1565C0u);
    draw_rect_border(sx-5,sy-22,10,18,0xFFFFD700u,3);
    
    i32 rx=410, ry=50;
    draw_str(rx,ry,"Cihaziniz icin gizlilik",C_DARK_GRAY,C_BG,2); ry+=22;
    draw_str(rx,ry,"ayarlarini secin",C_DARK_GRAY,C_BG,2); ry+=36;
    
    draw_str(rx,ry,"Konum hizmetleri aktif edilsin mi?",C_DARK_GRAY,C_BG,1);
    if(draw_toggle(rx+450, ry, privacy_1)) privacy_1 = !privacy_1;
    ry+=45;
    
    draw_str(rx,ry,"Hata ve tanilama verileri paylasilsin mi?",C_DARK_GRAY,C_BG,1);
    if(draw_toggle(rx+450, ry, privacy_2)) privacy_2 = !privacy_2;
    ry+=45;
    
    draw_str(rx,ry,"Kisisellestirilmis deneyimler",C_DARK_GRAY,C_BG,1);
    if(draw_toggle(rx+450, ry, privacy_3)) privacy_3 = !privacy_3;
    ry+=60;
    
    if(draw_button(rx+410,ry,110,36,"Kabul Et",C_BLUE,C_WHITE)) state=STATE_SETUP_6_UPDATE;
    draw_footer_bar();
    draw_progress(4);
}

/* EKRAN 6: GÜNCELLEME DENETLEME */
static void screen6(void){
    fill_rect(0,0,(i32)SCR_W,(i32)SCR_H,C_BG);
    fill_rect(0,0,380,(i32)SCR_H-46,C_HEADER);
    draw_str(60,220,"Güncellemeler",C_DARK_GRAY,C_HEADER,2);
    
    i32 rx=410, ry=200;
    draw_str(rx,ry,"Sistem güncellemeleri denetleniyor...",C_DARK_GRAY,C_BG,2); ry+=50;
    
    /* İlerleme çubuğu */
    fill_rect(rx, ry, 500, 10, C_LIGHT_GRAY);
    fill_rect(rx, ry, update_progress * 5, 10, C_BLUE);
    
    if(update_progress < 100) {
        update_progress++;
    } else {
        ry+=40;
        if(draw_button(rx+410,ry,110,36,"Ilerle",C_BLUE,C_WHITE)) state=STATE_SETUP_7_WELCOME;
    }
    draw_footer_bar();
    draw_progress(5);
}

/* EKRAN 7: KURULUM TAMAMLANDI */
static void screen7(void){
    fill_rect(0,0,(i32)SCR_W,(i32)SCR_H,C_BG);
    fill_rect(0,0,380,(i32)SCR_H-46,C_HEADER);
    
    i32 rx=410, ry=250;
    draw_str(rx,ry,"Her sey hazir!",C_DARK_GRAY,C_BG,3); ry+=60;
    draw_str(rx,ry,"Wind OS kullanima hazır.",C_GRAY,C_BG,1); ry+=50;
    
    if(draw_button(rx+150,ry,200,44,"Masaüstüne Gir",C_GREEN,C_WHITE)) state=STATE_DESKTOP;
    draw_footer_bar();
    draw_progress(6);
}

/* MASAÜSTÜ UYGULAMA YAPISI */
typedef struct {
    const char* name;
    u32 color;
    int installed;
} App;

static App apps[4] = {
    {"Dosya Yoneticisi", C_BLUE, 1},
    {"Tarayici", C_ORANGE, 0},
    {"Metin Editoru", C_GREEN, 1},
    {"Ayarlar", C_GRAY, 1}
};

/* MASAÜSTÜ DOSYA YÖNETİCİSİ */
static void draw_fm(void){
    if(!fm_open) return;
    i32 fw=620, fh=420;
    i32 fx=fm_x, fy=fm_y;
    
    fill_rect(fx+5,fy+5,fw,fh,C_SHADOW);
    fill_rect(fx,fy,fw,fh,C_WHITE);
    draw_rect_border(fx,fy,fw,fh,C_BORDER,1);
    
    fill_rect(fx,fy,fw,32,C_HEADER);
    draw_str(fx+12,fy+10,"Dosya Yoneticisi",C_DARK_GRAY,C_HEADER,1);
    
    if(draw_button(fx+fw-28,fy+6,22,20,"X",C_RED,C_WHITE)) fm_open=0;
    
    fill_rect(fx,fy+32,150,fh-32,C_BG);
    draw_str(fx+10,fy+50,"[Cihazlar]",C_DARK_GRAY,C_BG,1);
    
    u32 sys_bg = (fm_in_usb==0)?C_LIGHT_BLUE:C_BG;
    fill_rect(fx,fy+70,150,28,sys_bg);
    draw_str(fx+15,fy+78,"Sistem (C:)",C_DARK_GRAY,sys_bg,1);
    if(clicked(fx,fy+70,150,28)) fm_in_usb=0;
    
    if(usb_detected){
        u32 usb_bg = (fm_in_usb==1)?C_LIGHT_BLUE:C_BG;
        fill_rect(fx,fy+100,150,28,usb_bg);
        draw_str(fx+15,fy+108,usb_dev_name,C_DARK_GRAY,usb_bg,1);
        if(clicked(fx,fy+100,150,28)) fm_in_usb=1;
    }
    
    fill_rect(fx+150,fy+32,1,fh-32,C_BORDER);
    
    i32 rx=fx+165, ry=fy+50;
    if(fm_in_usb==0){
        draw_str(rx,ry,"WindOS_Kernel.bin  (256 KB)",C_DARK_GRAY,C_WHITE,1); ry+=24;
        draw_str(rx,ry,"System32/           <DIR>",C_DARK_GRAY,C_WHITE,1); ry+=24;
        draw_str(rx,ry,"Users/              <DIR>",C_DARK_GRAY,C_WHITE,1);
    } else {
        draw_str(rx,ry,"Fotograflar/        <DIR>",C_DARK_GRAY,C_WHITE,1); ry+=24;
        draw_str(rx,ry,"Belgelerim/         <DIR>",C_DARK_GRAY,C_WHITE,1); ry+=24;
        draw_str(rx,ry,"miku_wallpaper.png (1.2 MB)",C_DARK_GRAY,C_WHITE,1);
    }
}

/* ANA MASAÜSTÜ KABUĞU */
static void draw_desktop(void){
    fill_rect(0,0,(i32)SCR_W,(i32)SCR_H,0xFF1E1E2Eu); /* Arka plan */
    
    /* Wind OS Logosu */
    draw_win_logo(80, 80, 40);
    draw_str_c(40,110,80,"Wind OS",C_WHITE,0xFF1E1E2Eu,1);
    
    /* Uygulama Kısayollarını Çiz */
    for(int i=0; i<4; i++){
        i32 ix=20+i*110, iy=160;
        u32 bg=apps[i].installed?apps[i].color:0xFF3A3A3Au;
        fill_rrect(ix,iy,72,72,10,bg);
        char ab[3]={apps[i].name[0],apps[i].name[1],0};
        draw_str_c(ix,iy+32,72,ab,C_WHITE,bg,1);
        draw_str_c(ix-8,iy+78,88,apps[i].name,C_WHITE,0xFF1E1E2Eu,1);
        
        if(clicked(ix,iy,72,72) && i==0) fm_open=1; /* Dosya Yöneticisi tıklandı */
    }
    
    /* Dosya Yöneticisi Penceresi */
    draw_fm();
    
    /* Görev Çubuğu */
    fill_rect(0,(i32)SCR_H-40,(i32)SCR_W,40,C_TASKBAR);
    if(draw_button(0,(i32)SCR_H-40,50,40,"Baslat",C_BLUE,C_WHITE)) drawer_open = !drawer_open;
    
    /* Donanım Durum Bilgisi */
    if(usb_detected) draw_str((i32)SCR_W-120, (i32)SCR_H-24, "[USB AKTIF]", C_GREEN, C_TASKBAR, 1);
    else draw_str((i32)SCR_W-120, (i32)SCR_H-24, "[BAKILDI]", C_GRAY, C_TASKBAR, 1);
    
    /* Başlat Menüsü */
    if(drawer_open){
        fill_rect(0,(i32)SCR_H-240,200,200,C_TASKBAR_HL);
        draw_rect_border(0,(i32)SCR_H-240,200,200,C_BORDER,1);
        draw_str(15,(i32)SCR_H-220,"Wind OS Menu",C_WHITE,C_TASKBAR_HL,1);
        if(draw_button(10,(i32)SCR_H-80,180,30,"Sistem Yeniden Baslat",C_RED,C_WHITE)) state=STATE_SETUP_1_NAME;
    }
    
    /* PCI Arka Plan Taraması */
    if(++usb_scan_tick>2000){ pci_scan(); usb_scan_tick=0; }
}

/* GECİKTİRME FONKSİYONU */
static void delay_ms(int n){
    volatile int x=n*8000; while(x--);
}

/* =========================================================
   KERNEL_MAIN (Sistem Giriş Noktası)
   ========================================================= */
void kernel_main(multiboot_info_t* mbi){
    /* Framebuffer Yapılandırması */
    FB        = (u32*)(unsigned long)mbi->framebuffer_addr;
    SCR_W     = mbi->framebuffer_width;
    SCR_H     = mbi->framebuffer_height;
    SCR_PITCH = mbi->framebuffer_pitch / 4;

    /* GRUB Framebuffer Atamazsa Güvenli Yedek Modu */
    if(!FB || SCR_W==0){
        FB       = (u32*)0xFD000000u;
        SCR_W    = 1024;
        SCR_H    = 768;
        SCR_PITCH= 1024;
    }

    /* Donanım Sürücülerini Başlat */
    mouse_init();
    pci_scan();

    /* Çekirdek Sonsuz Döngüsü (Master Execution Loop) */
    while(1){
        /* Giriş Aygıtlarını Dinle (Polling) */
        mouse_poll();
        u8 key = kbd_poll();

        /* Aktif Ekran Durum Makinesini Çalıştır */
        switch(state){
          case STATE_SETUP_1_NAME:      screen1(key);  break;
          case STATE_SETUP_2_REGION:    screen2();     break;
          case STATE_SETUP_3_KEYBOARD:  screen3();     break;
          case STATE_SETUP_4_NETWORK:   screen4(key);  break;
          case STATE_SETUP_5_PRIVACY:   screen5();     break;
          case STATE_SETUP_6_UPDATE:    screen6();     break;
          case STATE_SETUP_7_WELCOME:   screen7();     break;
          case STATE_DESKTOP:           draw_desktop(); break;
        }

        /* Mouse İmlecini Render Et (Katman En Üstte Kalmalı) */
        draw_circle(mx, my, 4, C_RED);
        draw_circle(mx, my, 2, C_WHITE);

        /* Döngü Senkronizasyon Gecikmesi */
        delay_ms(8);
    }
}
