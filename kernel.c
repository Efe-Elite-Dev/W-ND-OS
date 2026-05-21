/*
 * Wind OS  -  kernel.c  v10 (ULTIMATE PROTOTYPE - NO LIMITS)
 * GPU olmayan bir ortamda CPU bazlı Alpha Blending, Soft Shadows,
 * Gradient ve Organik Blob çizim motoru entegre edildi.
 * Performans kısıtlamaları tamamen kaldırıldı, görsellik %100'e çekildi.
 *
 * gcc -m32 -ffreestanding -fno-builtin -fno-stack-protector -O3 -w -c kernel.c -o kernel.o
 */
#include "kernel.h"

typedef unsigned int   u32;
typedef unsigned short u16;
typedef unsigned char  u8;
typedef int            i32;

#define NULL ((void*)0)

/* ── FRAMEBUFFER ─────────────────────────────────── */
static volatile u32 *FB = (u32*)0;
static u32 SW = 1024, SH = 768, SP = 1024;
static u32 back_buffer[1024 * 768];

/* ── FÜTÜRİSTİK RENK PALETİ ─────────────────────── */
#define CW   0xFFFFFFFFu
#define CK   0xFF000000u
#define BG_TOP 0xFF1C1E23u 
#define BG_BOT 0xFF2A2D34u
#define PAN_BD  0xFF424549u
#define CTXT    0xFFDCDDDEu
#define CGY     0xFF99AAB5u

/* Canlı Neon İkon Renkleri */
#define C_CYAN  0xFF00E5FFu
#define C_PINK  0xFFE91E63u
#define C_ORNG  0xFFFF9800u
#define C_PURP  0xFF9C27B0u
#define C_LIME  0xFF10B981u
#define C_YEL   0xFFFFEB3Bu

/* ── PORT I/O & YARDIMCILAR ──────────────────────── */
static inline u8   inb (u16 p)       {u8  v;__asm__ volatile("inb  %1,%0":"=a"(v):"Nd"(p));return v;}
static inline void outb(u16 p, u8 v) {__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
static inline u32  inl (u16 p)       {u32 v;__asm__ volatile("inl  %1,%0":"=a"(v):"Nd"(p));return v;}
static inline void outl(u16 p, u32 v){__asm__ volatile("outl %0,%1"::"a"(v),"Nd"(p));}
static void *mcpy(void *d,const void *s,u32 n){ u8*dp=(u8*)d;const u8*sp=(const u8*)s;while(n--)*dp++=*sp++;return d; }
static u32 klen(const char *s){u32 n=0;while(s[n])n++;return n;}
static void kcpy(char *d,const char *s){while(*s)*d++=*s++;*d=0;}

/* ── 8x8 BITMAP FONT ─────────────────────────────── */
static const u8 F8[128][8]={
 [' ']={0,0,0,0,0,0,0,0},['!']={0x18,0x3C,0x3C,0x18,0x18,0,0x18,0},['"']={0x36,0x36,0,0,0,0,0,0},['#']={0x36,0x7F,0x36,0x36,0x7F,0x36,0x36,0},
 ['$']={0x0C,0x3E,0x03,0x1E,0x30,0x1F,0x0C,0},['%']={0x63,0x33,0x18,0x0C,0x66,0x63,0,0},['&']={0x1C,0x36,0x1C,0x6E,0x3B,0x33,0x6E,0},['\'']={0x06,0x0C,0,0,0,0,0,0},
 ['(']={0x18,0x0C,0x06,0x06,0x06,0x0C,0x18,0},[')']={0x06,0x0C,0x18,0x18,0x18,0x0C,0x06,0},['*']={0x66,0x3C,0xFF,0x3C,0x66,0,0,0},['+']={0,0x0C,0x0C,0x3F,0x0C,0x0C,0,0},
 [',']={0,0,0,0,0,0x18,0x18,0x0C},['-']={0,0,0,0x3F,0,0,0,0},['.']={0,0,0,0,0,0x18,0x18,0},['/']={0x60,0x30,0x18,0x0C,0x06,0x03,0x01,0},
 ['0']={0x3E,0x63,0x73,0x7B,0x6F,0x67,0x3E,0},['1']={0x0C,0x0E,0x0C,0x0C,0x0C,0x0C,0x3F,0},['2']={0x1E,0x33,0x30,0x1C,0x06,0x33,0x3F,0},['3']={0x1E,0x33,0x30,0x1C,0x30,0x33,0x1E,0},
 ['4']={0x38,0x3C,0x36,0x33,0x7F,0x30,0x78,0},['5']={0x3F,0x03,0x1F,0x30,0x30,0x33,0x1E,0},['6']={0x1C,0x06,0x03,0x1F,0x33,0x33,0x1E,0},['7']={0x3F,0x33,0x30,0x18,0x0C,0x0C,0x0C,0},
 ['8']={0x1E,0x33,0x33,0x1E,0x33,0x33,0x1E,0},['9']={0x1E,0x33,0x33,0x3E,0x30,0x18,0x0E,0},[':']={0,0x18,0x18,0,0x18,0x18,0,0},[';']={0,0x18,0x18,0,0x18,0x18,0x0C,0},
 ['<']={0x18,0x0C,0x06,0x03,0x06,0x0C,0x18,0},['=']={0,0x3F,0,0,0x3F,0,0,0},['>']={0x06,0x0C,0x18,0x30,0x18,0x0C,0x06,0},['?']={0x1E,0x33,0x30,0x18,0x0C,0,0x0C,0},
 ['@']={0x3E,0x63,0x7B,0x7B,0x7B,0x03,0x1E,0},['A']={0x0C,0x1E,0x33,0x3F,0x33,0x33,0x33,0},['B']={0x3F,0x66,0x66,0x3E,0x66,0x66,0x3F,0},['C']={0x3C,0x66,0x03,0x03,0x03,0x66,0x3C,0},
 ['D']={0x1F,0x36,0x66,0x66,0x66,0x36,0x1F,0},['E']={0x7F,0x46,0x16,0x1E,0x16,0x46,0x7F,0},['F']={0x7F,0x46,0x16,0x1E,0x16,0x06,0x0F,0},['G']={0x3C,0x66,0x03,0x73,0x63,0x66,0x7C,0},
 ['H']={0x33,0x33,0x33,0x3F,0x33,0x33,0x33,0},['I']={0x1E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0},['J']={0x78,0x30,0x30,0x30,0x33,0x33,0x1E,0},['K']={0x67,0x66,0x36,0x1E,0x36,0x66,0x67,0},
 ['L']={0x0F,0x06,0x06,0x06,0x46,0x66,0x7F,0},['M']={0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0},['N']={0x63,0x67,0x6F,0x7B,0x73,0x63,0x63,0},['O']={0x1C,0x36,0x63,0x63,0x63,0x36,0x1C,0},
 ['P']={0x3F,0x66,0x66,0x3E,0x06,0x06,0x0F,0},['Q']={0x1E,0x33,0x33,0x33,0x3B,0x1E,0x38,0},['R']={0x3F,0x66,0x66,0x3E,0x36,0x66,0x67,0},['S']={0x1E,0x33,0x07,0x0E,0x38,0x33,0x1E,0},
 ['T']={0x3F,0x2D,0x0C,0x0C,0x0C,0x0C,0x1E,0},['U']={0x33,0x33,0x33,0x33,0x33,0x33,0x3F,0},['V']={0x33,0x33,0x33,0x33,0x33,0x1E,0x0C,0},['W']={0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0},
 ['X']={0x63,0x63,0x36,0x1C,0x36,0x63,0x63,0},['Y']={0x33,0x33,0x33,0x1E,0x0C,0x0C,0x1E,0},['Z']={0x7F,0x63,0x31,0x18,0x4C,0x66,0x7F,0},['[']={0x1E,0x06,0x06,0x06,0x06,0x06,0x1E,0},
 ['\\']={0x03,0x06,0x0C,0x18,0x30,0x60,0x40,0},[']']={0x1E,0x18,0x18,0x18,0x18,0x18,0x1E,0},['^']={0x08,0x1C,0x36,0x63,0,0,0,0},['_']={0,0,0,0,0,0,0,0xFF},
 ['`']={0x06,0x0C,0x18,0,0,0,0,0},['a']={0,0x1E,0x30,0x3E,0x33,0x33,0x6E,0},['b']={0x07,0x06,0x3E,0x66,0x66,0x66,0x3B,0},['c']={0,0x1E,0x33,0x03,0x03,0x33,0x1E,0},
 ['d']={0x38,0x30,0x3E,0x33,0x33,0x33,0x6E,0},['e']={0,0x1E,0x33,0x3F,0x03,0x33,0x1E,0},['f']={0x1C,0x36,0x06,0x0F,0x06,0x06,0x0F,0},['g']={0,0x6E,0x33,0x33,0x3E,0x30,0x33,0x1E},
 ['h']={0x07,0x06,0x36,0x6E,0x66,0x66,0x67,0},['i']={0x0C,0,0x0E,0x0C,0x0C,0x0C,0x1E,0},['j']={0x18,0,0x18,0x18,0x18,0x1B,0x1B,0x0E},['k']={0x07,0x06,0x66,0x36,0x1E,0x36,0x67,0},
 ['l']={0x0E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0},['m']={0,0x33,0x7F,0x7F,0x6B,0x63,0x63,0},['n']={0,0x1F,0x33,0x33,0x33,0x33,0x33,0},['o']={0,0x1E,0x33,0x33,0x33,0x33,0x1E,0},
 ['p']={0x00,0x3B,0x66,0x66,0x3E,0x06,0x06,0x0F},['q']={0x00,0x6E,0x33,0x33,0x3E,0x30,0x30,0x78},['r']={0x00,0x3B,0x6E,0x66,0x06,0x06,0x0F,0},['s']={0x00,0x3E,0x03,0x1E,0x30,0x33,0x1E,0},
 ['t']={0x08,0x3E,0x0C,0x0C,0x0C,0x2C,0x18,0},['u']={0x00,0x33,0x33,0x33,0x33,0x33,0x6E,0},['v']={0x00,0x33,0x33,0x33,0x33,0x1E,0x0C,0},['w']={0x00,0x63,0x6B,0x7F,0x7F,0x36,0x36,0},
 ['x']={0x00,0x63,0x36,0x1C,0x1C,0x36,0x63,0},['y']={0x00,0x33,0x33,0x33,0x3E,0x30,0x33,0x1E},['z']={0x00,0x3F,0x19,0x0C,0x26,0x3F,0,0},['{']={0x38,0x0C,0x0C,0x07,0x0C,0x0C,0x38,0},
 ['|']={0x18,0x18,0x18,0,0x18,0x18,0x18,0},['}']={0x07,0x0C,0x0C,0x38,0x0C,0x0C,0x07,0},['~']={0x6E,0x3B,0,0,0,0,0,0},
};

/* ================================================================
   GELİŞMİŞ RENDER MOTORU (ALPHA BLENDING & GÖLGELER)
   ================================================================ */

/* Renkleri harmanlayan formül (Saydamlık için) */
static u32 blend(u32 fg, u32 bg, u8 alpha) {
    if (alpha == 255) return fg;
    if (alpha == 0) return bg;
    u32 rb = (((fg & 0xFF00FF) * alpha) + ((bg & 0xFF00FF) * (255 - alpha))) >> 8;
    u32 g  = (((fg & 0x00FF00) * alpha) + ((bg & 0x00FF00) * (255 - alpha))) >> 8;
    return (rb & 0xFF00FF) | (g & 0x00FF00);
}

/* Saydam Piksel Çizici */
static inline void alpha_pp(i32 x, i32 y, u32 c, u8 alpha){
    if((u32)x<SW && (u32)y<SH) {
        u32 bg = back_buffer[(u32)y*SP+(u32)x];
        back_buffer[(u32)y*SP+(u32)x] = blend(c, bg, alpha);
    }
}

/* Saydam Dikdörtgen */
static void alpha_fr(i32 x, i32 y, i32 w, i32 h, u32 c, u8 alpha){
    i32 x1=x<0?0:x, y1=y<0?0:y, x2=x+w>(i32)SW?(i32)SW:x+w, y2=y+h>(i32)SH?(i32)SH:y+h;
    for(i32 j=y1; j<y2; j++) 
        for(i32 i=x1; i<x2; i++) 
            alpha_pp(i, j, c, alpha);
}

/* Saydam Daire */
static void alpha_circ(i32 cx, i32 cy, i32 r, u32 c, u8 alpha){
    for(i32 dy=-r; dy<=r; dy++) 
        for(i32 dx=-r; dx<=r; dx++) 
            if(dx*dx+dy*dy <= r*r) 
                alpha_pp(cx+dx, cy+dy, c, alpha);
}

/* Gradient (Renk Geçişli) Arka Plan */
static void gradient_bg() {
    for(i32 y=0; y<(i32)SH; y++) {
        /* Üstten alta koyu lacivertten koyu griye geçiş */
        u8 ratio = (y * 255) / SH;
        u32 c = blend(BG_BOT, BG_TOP, ratio);
        for(i32 x=0; x<(i32)SW; x++) {
            back_buffer[y*SP+x] = c;
            /* Hafif Grid Efekti */
            if (x % 60 == 0 || y % 60 == 0) {
                back_buffer[y*SP+x] = blend(GRID_C, c, 100);
            }
        }
    }
}

/* Saydam ve Gölgeli Yuvarlatılmış Dikdörtgen (Glassmorphism) */
static void glass_rr(i32 x, i32 y, i32 w, i32 h, i32 r, u32 c, u8 alpha) {
    /* 1. Katman: Dışa yayılan yumuşak gölge (Soft Shadow) */
    for(int s=10; s>0; s-=2) {
        u8 shadow_alpha = 15 - s; 
        alpha_fr(x-s+r, y-s, w-2*r+2*s, h+2*s, 0x000000, shadow_alpha);
        alpha_fr(x-s, y-s+r, r, h-2*r+2*s, 0x000000, shadow_alpha);
        alpha_fr(x+w-r+s, y-s+r, r, h-2*r+2*s, 0x000000, shadow_alpha);
        alpha_circ(x+r, y+r, r+s, 0x000000, shadow_alpha);
        alpha_circ(x+w-r-1, y+r, r+s, 0x000000, shadow_alpha);
        alpha_circ(x+r, y+h-r-1, r+s, 0x000000, shadow_alpha);
        alpha_circ(x+w-r-1, y+h-r-1, r+s, 0x000000, shadow_alpha);
    }
    
    /* 2. Katman: Yarı saydam gövde */
    if(r>w/2) r=w/2; if(r>h/2) r=h/2;
    alpha_fr(x+r, y, w-2*r, h, c, alpha); 
    alpha_fr(x, y+r, r, h-2*r, c, alpha); 
    alpha_fr(x+w-r, y+r, r, h-2*r, c, alpha);
    alpha_circ(x+r, y+r, r, c, alpha); 
    alpha_circ(x+w-r-1, y+r, r, c, alpha); 
    alpha_circ(x+r, y+h-r-1, r, c, alpha); 
    alpha_circ(x+w-r-1, y+h-r-1, r, c, alpha);
    
    /* 3. Katman: Parlak Çerçeve Efekti (Glass border) */
    alpha_fr(x+r, y, w-2*r, 1, CW, 50);
}

/* Fütüristik "Organik" Blob İkon Çizici (Prototipteki Şekilsiz Kutular) */
static void blob_icon(i32 cx, i32 cy, u32 c) {
    /* 1. Yarı saydam koyu taban */
    alpha_circ(cx-8, cy-8, 18, 0x22252A, 200);
    alpha_circ(cx+10, cy-5, 20, 0x22252A, 200);
    alpha_circ(cx-2, cy+12, 22, 0x22252A, 200);
    alpha_fr(cx-15, cy-8, 30, 25, 0x22252A, 200);
    
    /* 2. Renkli Merkezi İkon (Parlayan) */
    alpha_circ(cx, cy, 10, c, 255);
    alpha_circ(cx, cy, 14, c, 80); /* Glow Efekti */
}

/* Kalın Diyagonal Çizgi */
static void thick_line(i32 x0, i32 y0, i32 x1, i32 y1, i32 thickness, u32 c, u8 alpha) {
    i32 dx = (x1>x0?x1-x0:x0-x1), dy = -(y1>y0?y1-y0:y0-y1);
    i32 sx = x0<x1?1:-1, sy = y0<y1?1:-1, err = dx+dy, e2;
    while(1) {
        alpha_circ(x0, y0, thickness/2, c, alpha);
        if(x0==x1 && y0==y1) break;
        e2=2*err; if(e2>=dy){err+=dy; x0+=sx;} if(e2<=dx){err+=dx; y0+=sy;}
    }
}

static void dc(i32 x,i32 y,char ch,u32 fg,i32 sc){
    if((u8)ch>=128) ch='?'; const u8 *g=F8[(u8)ch];
    for(i32 row=0;row<8;row++) for(i32 col=0;col<8;col++) 
        if(g[row]&(1<<(7-col))) alpha_fr(x+col*sc,y+row*sc,sc,sc,fg,255);
}
static void ds(i32 x,i32 y,const char*s,u32 fg,i32 sc){
    i32 cx=x; while(*s){ if(*s=='\n'){cx=x;y+=8*sc;} else{dc(cx,y,*s,fg,sc);cx+=8*sc;} s++; }
}
static void dsc(i32 x,i32 y,i32 w,const char*s,u32 fg,i32 sc){
    i32 tw=(i32)klen(s)*8*sc; if(tw<w) ds(x+(w-tw)/2,y,s,fg,sc); else ds(x,y,s,fg,sc);
}
static void swap_buffers(void) {
    u32 total = SW * SH; for(u32 i = 0; i < total; i++) FB[i] = back_buffer[i];
}

/* ================================================================
   SÜRÜCÜLER 
   ================================================================ */
static i32 MX=512,MY=384,MLB=0,MRB=0,PMLB=0;
static u8  MCY=0; static i8 MBF[3]={0}; static int MOUSE_READY=0;
static void m_cmd_wait(void){u32 t=100000;while(t--&&(inb(0x64)&0x02));}
static void m_dat_wait(void){u32 t=100000;while(t--&&!(inb(0x64)&0x01));}
static void m_write(u8 v){m_cmd_wait();outb(0x64,0xD4);m_cmd_wait();outb(0x60,v);}
static u8   m_read (void){m_dat_wait();return inb(0x60);}

static void mouse_init(void){
    m_cmd_wait(); outb(0x64,0xA8); m_cmd_wait(); outb(0x64,0x20); m_dat_wait(); u8 cfg=inb(0x60);
    cfg|=0x02; cfg&=~0x20; m_cmd_wait(); outb(0x64,0x60); m_cmd_wait(); outb(0x60,cfg);
    m_write(0xFF); u8 ack=m_read(); u8 ok=m_read(); m_read();
    if(ack==0xFA && ok==0xAA){ m_write(0xF6); m_read(); m_write(0xF4); m_read(); MOUSE_READY=1; }
}

static void mouse_poll(void){
    if(!MOUSE_READY) return;
    for(int iter=0;iter<16;iter++){
        u8 st=inb(0x64); if(!(st&0x01)) break; 
        if(!(st&0x20)){ inb(0x60); continue; }
        u8 dat=inb(0x60);
        switch(MCY){
          case 0: if(!(dat&0x08)){MCY=0;break;} MBF[0]=(i8)dat; MCY=1; break;
          case 1: MBF[1]=(i8)dat; MCY=2; break;
          case 2: MBF[2]=(i8)dat; MCY=0;{
            i32 dx=(i32)MBF[1]; i32 dy=(i32)MBF[2];
            if(MBF[0]&0x10) dx|=(i32)0xFFFFFF00;
            if(MBF[0]&0x20) dy|=(i32)0xFFFFFF00;
            if(MBF[0]&0x40) dx=0; if(MBF[0]&0x80) dy=0;
            MX+=dx; MY-=dy;
            if(MX<0) MX=0; if(MY<0) MY=0;
            if(MX>=(i32)SW) MX=(i32)SW-1; if(MY>=(i32)SH) MY=(i32)SH-1;
            PMLB=MLB; MLB=(MBF[0]&0x01)?1:0; MRB=(MBF[0]&0x02)?1:0;
          } break;
        }
    }
}

static void CUR(void){
    static const u8 cur[16][12]={ {1},{1,1},{1,2,1},{1,2,2,1},{1,2,2,2,1},{1,2,2,2,2,1},{1,2,2,2,2,2,1},{1,2,2,2,2,2,2,1},{1,2,2,2,2,2,2,2,1},{1,2,2,2,2,1,1,1,1,1},{1,2,2,1,2,2,1},{1,2,1,0,1,2,2,1},{1,1,0,0,1,2,2,1},{0,0,0,0,0,1,2,2,1},{0,0,0,0,0,1,2,2,1},{0,0,0,0,0,0,1,1} };
    /* İnce fare gölgesi */
    for(int r=0;r<16;r++) for(int c=0;c<12;c++){ if(cur[r][c]!=0) alpha_pp(MX+c+3, MY+r+3, 0x000000, 100); }
    /* Ana fare */
    for(int r=0;r<16;r++) for(int c=0;c<12;c++){ if(cur[r][c]==1) alpha_pp(MX+c, MY+r, CW, 255); else if(cur[r][c]==2) alpha_pp(MX+c, MY+r, CK, 255); }
}

/* ================================================================
   ANA ARAYÜZ (TAM KAPSAMLI FÜTÜRİSTİK ÇİZİM)
   ================================================================ */
static void DESKTOP(void){
    /* 1. Gradient (Geçişli) Arka Plan */
    gradient_bg();

    /* 2. Asimetrik Kalın Tasarım Çizgileri (Glow efekti ile) */
    thick_line(230, -50, 160, SH+50, 8, PAN_BD, 150);
    thick_line(SW-250, -50, SW-180, SH+50, 8, PAN_BD, 150);
    thick_line(130, SH-180, SW-150, SH-120, 6, PAN_BD, 150);

    /* 3. MERKEZİ HAVA DURUMU PENCERESİ (Saydam Glassmorphism) */
    i32 wx = 220, wy = 40, ww = 540, wh = 170;
    glass_rr(wx, wy, ww, wh, 25, 0x1A2533, 200); /* Yarı saydam mavi taban */
    
    /* Hava Durumu İç Detayları */
    alpha_circ(wx+80, wy+85, 45, 0xFFFFFF, 15); /* Saydam dış çember */
    alpha_circ(wx+80, wy+85, 38, WIDGET_BG, 200);
    thick_line(wx+80, wy+85, wx+80, wy+60, 4, CW, 255); /* Akrep */
    thick_line(wx+80, wy+85, wx+100, wy+100, 3, CW, 255); /* Yelkovan */
    
    ds(wx+150, wy+60, "26:03", CW, 4);
    ds(wx+155, wy+100, "Istanbul, Turkiye", CGY, 1);
    
    /* Sağ Taraf - Küçük Hava Tahminleri (Çizimdeki gibi) */
    for(int i=0; i<4; i++) {
        ds(wx+360+(i*40), wy+40, "Gun", CGY, 1);
        alpha_circ(wx+370+(i*40), wy+70, 8, C_ORNG, 200); /* Güneş simülasyonu */
        ds(wx+360+(i*40), wy+90, "22 C", CW, 1);
        ds(wx+360+(i*40), wy+105, "15 C", CGY, 1);
    }

    /* 4. SOL PANEL UYGULAMALARI (Tam Asimetrik) */
    dsc(0, 80, 160, "SISTEM ARACLARI", CTXT, 1);
    
    blob_icon(80, 150, C_CYAN); dsc(40, 180, 80, "Terminal", CTXT, 1);
    blob_icon(70, 270, C_PINK); dsc(30, 300, 80, "Kamera", CTXT, 1);
    blob_icon(60, 390, C_LIME); dsc(20, 420, 80, "Ayarlar", CTXT, 1);

    /* 5. SAĞ PANEL UYGULAMALARI */
    dsc(SW-180, 80, 180, "UYGULAMALAR", CTXT, 1);
    
    blob_icon(SW-90, 150, C_YEL);  dsc(SW-130, 180, 80, "Mesajlar", CTXT, 1);
    blob_icon(SW-80, 270, C_PURP); dsc(SW-120, 300, 80, "Dosyalar", CTXT, 1);
    blob_icon(SW-70, 390, C_ORNG); dsc(SW-110, 420, 80, "Tarayici", CTXT, 1);

    /* 6. ORTA ALT UYGULAMA GRİDİ (Prototipteki "Kutu içinde Blob" Görünümü) */
    i32 gx = 200, gy = 240;
    
    /* İç paneli Glassmorphism ile çiz */
    glass_rr(gx, gy, 560, 240, 15, 0x24272D, 150);

    /* Satır 1 */
    blob_icon(gx+60, gy+50, C_CYAN); dsc(gx+20, gy+80, 80, "Notlar", CTXT, 1);
    blob_icon(gx+180, gy+50, C_RED); dsc(gx+140, gy+80, 80, "Oyunlar", CTXT, 1);
    blob_icon(gx+300, gy+50, C_LIME); dsc(gx+260, gy+80, 80, "Harita", CTXT, 1);
    blob_icon(gx+420, gy+50, C_PINK); dsc(gx+380, gy+80, 80, "Muzik", CTXT, 1);

    /* Satır 2 */
    blob_icon(gx+90, gy+150, C_ORNG); dsc(gx+50, gy+180, 80, "Hesap", CTXT, 1);
    blob_icon(gx+210, gy+150, C_PURP); dsc(gx+170, gy+180, 80, "Takvim", CTXT, 1);
    blob_icon(gx+330, gy+150, C_YEL); dsc(gx+290, gy+180, 80, "Banka", CTXT, 1);
    blob_icon(gx+450, gy+150, C_CYAN); dsc(gx+410, gy+180, 80, "Hava", CTXT, 1);

    /* 7. ALT ASİMETRİK GÖREV ÇUBUĞU */
    glass_rr(250, SH-90, 500, 60, 20, 0x1A1C20, 180);
    ds(280, SH-65, "WIND OS // CORE v10 (NO LIMITS)", CGY, 1);
    alpha_circ(640, SH-60, 6, C_LIME, 255); 
    alpha_circ(640, SH-60, 12, C_LIME, 80); /* Glow */
    ds(660, SH-65, "SISTEM AKTIF", CTXT, 1);
}

/* ================================================================
   KERNEL_MAIN
   ================================================================ */
void kernel_main(multiboot_info_t *mbi){
    u8 bpp  = mbi->framebuffer_bpp; if(bpp==0) bpp=32; u32 Bpp = (u32)bpp / 8;
    FB  = (volatile u32*)(u32)mbi->framebuffer_addr; SW  = mbi->framebuffer_width; SH  = mbi->framebuffer_height; SP  = mbi->framebuffer_pitch / Bpp;
    if(!FB || SW==0){ FB=(volatile u32*)0xFD000000u; SW=1024; SH=768; SP=1024; }
    
    mouse_init(); 
    
    while(1){ 
        mouse_poll(); 
        inb(0x60); /* Kbd drain for stability */
        DESKTOP(); 
        CUR(); 
        swap_buffers(); 
        /* CPU sınırlarını kaldırdık, bekleme yok */
    }
}
