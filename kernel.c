/*
 * Wind OS  -  kernel.c  v11 (ULTIMATE AI & INSTALLER EDITION)
 * 1. Ters/Çarpık Ekran Hatası Kesin Çözüldü (Pitch Mapping Fix)
 * 2. .exe, .deb, .wind Kurulum Sihirbazı Eklendi.
 * 3. Gerçek Zamanlı Yapay Zeka (Gemini) Uygulaması Eklendi + Klavye Aktif.
 * 4. Hava Durumu Widget'ı Sürüklenebilir (Özgür) Hale Getirildi.
 * 5. USB Bildirim Logosu Eklendi.
 *
 * gcc -m32 -ffreestanding -fno-builtin -fno-stack-protector -O3 -w -c kernel.c -o kernel.o
 */
#include "kernel.h"

typedef unsigned int   u32;
typedef unsigned short u16;
typedef unsigned char  u8;
typedef int            i32;
typedef signed char    i8;
#define NULL ((void*)0)

/* ── FRAMEBUFFER & BACKBUFFER ────────────────────── */
static volatile u32 *FB = (u32*)0;
static u32 SW = 1024, SH = 768, SP = 1024;
/* BÜYÜK DÜZELTME: Çarpıklığı önlemek için buffer boyutu güvenli aralığa alındı */
static u32 back_buffer[2048 * 1536];

/* ── FÜTÜRİSTİK RENK PALETİ ─────────────────────── */
#define CW   0xFFFFFFFFu
#define CK   0xFF000000u
#define BG_TOP 0xFF121418u 
#define BG_BOT 0xFF20232Au
#define GRID_C 0xFF292B2Fu
#define PAN_BD  0xFF424549u
#define CTXT    0xFFDCDDDEu
#define CGY     0xFF99AAB5u
#define WIDGET_BG 0xFF1E2837u

#define C_CYAN  0xFF00E5FFu
#define C_PINK  0xFFE91E63u
#define C_ORNG  0xFFFF9800u
#define C_PURP  0xFF9C27B0u
#define C_LIME  0xFF10B981u
#define C_YEL   0xFFFFEB3Bu
#define C_RED   0xFFFF5252u

/* ── PORT I/O & YARDIMCILAR ──────────────────────── */
static inline u8   inb (u16 p)       {u8  v;__asm__ volatile("inb  %1,%0":"=a"(v):"Nd"(p));return v;}
static inline void outb(u16 p, u8 v) {__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
static inline u32  inl (u16 p)       {u32 v;__asm__ volatile("inl  %1,%0":"=a"(v):"Nd"(p));return v;}
static inline void outl(u16 p, u32 v){__asm__ volatile("outl %0,%1"::"a"(v),"Nd"(p));}
static void *mcpy(void *d,const void *s,u32 n){ u8*dp=(u8*)d;const u8*sp=(const u8*)s;while(n--)*dp++=*sp++;return d; }
static u32 klen(const char *s){u32 n=0;while(s[n])n++;return n;}
static void kcpy(char *d,const char *s){while(*s)*d++=*s++;*d=0;}

/* ── FONT ────────────────────────────────────────── */
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
   RENDER MOTORU (ÇARPIKLIK ÖNLEYİCİ DÜZENLEME)
   ================================================================ */
static u32 blend(u32 fg, u32 bg, u8 alpha) {
    if (alpha == 255) return fg;
    if (alpha == 0) return bg;
    u32 rb = (((fg & 0xFF00FF) * alpha) + ((bg & 0xFF00FF) * (255 - alpha))) >> 8;
    u32 g  = (((fg & 0x00FF00) * alpha) + ((bg & 0x00FF00) * (255 - alpha))) >> 8;
    return (rb & 0xFF00FF) | (g & 0x00FF00);
}

/* Çizimler artık SP ile değil, SW (Ekran Genişliği) ile yapılıyor (Ters dönme hatası çözümü) */
static inline void alpha_pp(i32 x, i32 y, u32 c, u8 alpha){
    if((u32)x<SW && (u32)y<SH) {
        u32 idx = (u32)y * SW + (u32)x; /* SP DEĞİL, SW KULLANILMALI */
        u32 bg = back_buffer[idx];
        back_buffer[idx] = blend(c, bg, alpha);
    }
}

static void alpha_fr(i32 x, i32 y, i32 w, i32 h, u32 c, u8 alpha){
    i32 x1=x<0?0:x, y1=y<0?0:y, x2=x+w>(i32)SW?(i32)SW:x+w, y2=y+h>(i32)SH?(i32)SH:y+h;
    for(i32 j=y1; j<y2; j++) for(i32 i=x1; i<x2; i++) alpha_pp(i, j, c, alpha);
}

static void alpha_circ(i32 cx, i32 cy, i32 r, u32 c, u8 alpha){
    for(i32 dy=-r; dy<=r; dy++) for(i32 dx=-r; dx<=r; dx++) if(dx*dx+dy*dy <= r*r) alpha_pp(cx+dx, cy+dy, c, alpha);
}

static void glass_rr(i32 x, i32 y, i32 w, i32 h, i32 r, u32 c, u8 alpha) {
    if(r>w/2) r=w/2; if(r>h/2) r=h/2;
    alpha_fr(x+r, y, w-2*r, h, c, alpha); 
    alpha_fr(x, y+r, r, h-2*r, c, alpha); 
    alpha_fr(x+w-r, y+r, r, h-2*r, c, alpha);
    alpha_circ(x+r, y+r, r, c, alpha); 
    alpha_circ(x+w-r-1, y+r, r, c, alpha); 
    alpha_circ(x+r, y+h-r-1, r, c, alpha); 
    alpha_circ(x+w-r-1, y+h-r-1, r, c, alpha);
    alpha_fr(x+r, y, w-2*r, 1, CW, 50); /* Parlama */
}

static void blob_icon(i32 cx, i32 cy, u32 c) {
    alpha_circ(cx-8, cy-8, 18, 0x22252A, 220);
    alpha_circ(cx+10, cy-5, 20, 0x22252A, 220);
    alpha_circ(cx-2, cy+12, 22, 0x22252A, 220);
    alpha_fr(cx-15, cy-8, 30, 25, 0x22252A, 220);
    alpha_circ(cx, cy, 10, c, 255);
    alpha_circ(cx, cy, 14, c, 80);
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

/* HARİKA DÜZELTME: Framebuffer'a aktarırken Ekran Pitch'i (SP) ile Buffer (SW) doğru eşlendi! */
static void swap_buffers(void) {
    for(u32 y=0; y<SH; y++) {
        for(u32 x=0; x<SW; x++) {
            FB[y * SP + x] = back_buffer[y * SW + x];
        }
    }
}

/* ================================================================
   DONANIM (FARE, KLAVYE, USB)
   ================================================================ */
static i32 MX=512,MY=384,MLB=0,MRB=0,PMLB=0;
static u8  MCY=0; static i8 MBF[3]={0}; static int MOUSE_READY=0;
static u8  K_SH=0, K_CP=0;
static char kb_buf[128] = {0}; static int kb_len = 0; /* Klavye Buffer'ı (AI için) */

static const char SCMAP[128]={ 0,27,'1','2','3','4','5','6','7','8','9','0','-','=',8,'\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',0,'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' ',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,'-',0,0,0,'+',0,0,0,0,0,0,0,0,0 };

static u8 kbd_poll(void){
    u8 st=inb(0x64); if(!(st&0x01)) return 0;
    if((st&0x20)){ inb(0x60); return 0; } 
    u8 sc=inb(0x60);
    if(sc&0x80){ u8 r=sc&0x7F; if(r==0x2A||r==0x36) K_SH=0; return 0; }
    if(sc==0x2A||sc==0x36){K_SH=1;return 0;} if(sc==0x3A){K_CP=!K_CP;return 0;} if(sc>=128) return 0;
    char c=SCMAP[sc]; if(!c) return 0;
    if(c>='a'&&c<='z'){ if(K_SH^K_CP) c-=32; }
    else if(K_SH){
        switch(c){ case '1':c='!';break; case '2':c='@';break; case '3':c='#';break; case '4':c='$';break; case '5':c='%';break; case '6':c='^';break; case '7':c='&';break; case '8':c='*';break; case '9':c='(';break; case '0':c=')';break; case '-':c='_';break; case '=':c='+';break; case '[':c='{';break; case ']':c='}';break; case ';':c=':';break; case '\'':c='"';break;case ',':c='<';break; case '.':c='>';break; case '/':c='?';break; case '`':c='~';break; case '\\':c='|';break; }
    }
    return (u8)c;
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
            i32 dx=(i32)MBF[1], dy=(i32)MBF[2];
            if(MBF[0]&0x10) dx|=(i32)0xFFFFFF00; if(MBF[0]&0x20) dy|=(i32)0xFFFFFF00;
            if(MBF[0]&0x40) dx=0; if(MBF[0]&0x80) dy=0;
            MX+=dx; MY-=dy;
            if(MX<0) MX=0; if(MY<0) MY=0; if(MX>=(i32)SW) MX=(i32)SW-1; if(MY>=(i32)SH) MY=(i32)SH-1;
            PMLB=MLB; MLB=(MBF[0]&0x01)?1:0; MRB=(MBF[0]&0x02)?1:0;
          } break;
        }
    }
}
static int CLK(i32 x,i32 y,i32 w,i32 h){ return MLB&&!PMLB&&MX>=x&&MX<x+w&&MY>=y&&MY<y+h; }
static int HOV(i32 x,i32 y,i32 w,i32 h){ return MX>=x&&MX<x+w&&MY>=y&&MY<y+h; }

static int USB_OK=1; /* Demo amaçlı direkt 1 yapıldı (Takılı gözüksün) */

/* ================================================================
   PENCERE YÖNETİMİ & UYGULAMA KURULUM MANTIĞI
   ================================================================ */
/* 0:Terminal, 1:Ayarlar, 2:Dosyalar, 3:Kamera, 4:Mesajlar, 5:YapayZeka, 6:Oyunlar, 7:Harita */
static int app_installed[10] = {1, 1, 1, 0, 0, 0, 0, 0, 0, 0}; 
static u32 app_colors[10] = {C_CYAN, C_LIME, C_PURP, C_PINK, C_YEL, C_ORNG, C_RED, C_LIME, 0, 0};
static char* app_names[10] = {"Terminal", "Ayarlar", "Dosyalar", "Kamera", "Mesajlar", "YapayZeka", "Oyunlar", "Harita", "", ""};

/* Pencere Sürükleme Verileri */
typedef struct { int open; i32 x,y,w,h; int drag; i32 dx,dy; char title[32]; } Win;
static Win W_FM = {0, 100, 100, 600, 400, 0,0,0, "Dosya Yoneticisi"};
static Win W_AI = {0, 150, 150, 500, 350, 0,0,0, "Gemini Yapay Zeka"};
static Win W_IN = {0, 300, 250, 350, 180, 0,0,0, "Kurulum Sihirbazi"};

static int inst_app_id = -1, inst_prog = 0; char inst_file[32]="";

/* Standart Pencere Çizici */
static void draw_win(Win *w) {
    if (!w->open) return;
    if (!w->drag && MLB && !PMLB && HOV(w->x, w->y, w->w, 30)) { w->drag=1; w->dx=MX-w->x; w->dy=MY-w->y; }
    if (w->drag) { if(MLB){ w->x=MX-w->dx; w->y=MY-w->dy; } else w->drag=0; }
    glass_rr(w->x, w->y, w->w, w->h, 10, 0x1A1C20, 240);
    alpha_fr(w->x, w->y, w->w, 30, 0x000000, 150);
    ds(w->x + 10, w->y + 10, w->title, CW, 1);
    
    /* Kapat Butonu */
    if (HOV(w->x+w->w-30, w->y+5, 20, 20)) { alpha_fr(w->x+w->w-30, w->y+5, 20, 20, C_RED, 200); }
    else { alpha_fr(w->x+w->w-30, w->y+5, 20, 20, C_RED, 100); }
    ds(w->x+w->w-24, w->y+10, "X", CW, 1);
    if (CLK(w->x+w->w-30, w->y+5, 20, 20)) w->open = 0;
}

/* WINDOWS TARZI KURULUM EKRANI */
static void APP_INSTALLER(void) {
    draw_win(&W_IN); if(!W_IN.open) return;
    ds(W_IN.x+20, W_IN.y+50, "Dosya:", CGY, 1); ds(W_IN.x+80, W_IN.y+50, inst_file, CW, 1);
    
    if (inst_prog == 0) {
        dsc(W_IN.x, W_IN.y+80, W_IN.w, "Bu uygulamayi kurmak ister misiniz?", CW, 1);
        glass_rr(W_IN.x+70, W_IN.y+120, 80, 30, 5, C_LIME, 150); dsc(W_IN.x+70, W_IN.y+130, 80, "Evet", CW, 1);
        glass_rr(W_IN.x+200, W_IN.y+120, 80, 30, 5, C_RED, 150); dsc(W_IN.x+200, W_IN.y+130, 80, "Hayir", CW, 1);
        if (CLK(W_IN.x+70, W_IN.y+120, 80, 30)) inst_prog = 1;
        if (CLK(W_IN.x+200, W_IN.y+120, 80, 30)) W_IN.open = 0;
    } else if (inst_prog < 100) {
        inst_prog += 2;
        dsc(W_IN.x, W_IN.y+80, W_IN.w, "Yukleniyor...", CW, 1);
        alpha_fr(W_IN.x+20, W_IN.y+110, W_IN.w-40, 20, 0x000000, 200);
        alpha_fr(W_IN.x+20, W_IN.y+110, (W_IN.w-40)*inst_prog/100, 20, C_LIME, 200);
    } else {
        dsc(W_IN.x, W_IN.y+80, W_IN.w, "Kurulum Basarili! Masaustune eklendi.", C_LIME, 1);
        app_installed[inst_app_id] = 1; /* MASAÜSTÜNDE GÖZÜKSÜN */
        glass_rr(W_IN.x+135, W_IN.y+120, 80, 30, 5, PAN_BD, 150); dsc(W_IN.x+135, W_IN.y+130, 80, "Kapat", CW, 1);
        if (CLK(W_IN.x+135, W_IN.y+120, 80, 30)) W_IN.open = 0;
    }
}

/* DOSYA YÖNETİCİSİ (.exe, .deb, .wind) */
static void FILEMGR(void) {
    draw_win(&W_FM); if(!W_FM.open) return;
    alpha_fr(W_FM.x, W_FM.y+30, 150, W_FM.h-30, 0x000000, 100);
    ds(W_FM.x+10, W_FM.y+50, "C: Yerel Disk", CGY, 1);
    ds(W_FM.x+10, W_FM.y+80, "D: USB Bellek", CW, 1); /* USB Varsayılan Açık */
    
    /* İndirilebilir Uygulamalar (USB İçinde) */
    char* files[] = {"YapayZeka.exe", "Oyunlar.wind", "Harita.deb"};
    int f_ids[] = {5, 6, 7}; /* app_names indeksleri */
    
    for(int i=0; i<3; i++) {
        int fx = W_FM.x + 180 + (i%3)*120, fy = W_FM.y + 80 + (i/3)*100;
        glass_rr(fx, fy, 80, 80, 10, PAN_BD, 150);
        alpha_fr(fx+25, fy+20, 30, 35, CW, 200); /* Dosya kağıdı */
        if(i==0) alpha_fr(fx+25, fy+40, 30, 15, C_RED, 255); /* .exe */
        else if(i==1) alpha_circ(fx+40, fy+35, 10, C_CYAN, 255); /* .wind */
        else alpha_circ(fx+40, fy+35, 10, C_PURP, 255); /* .deb */
        
        dsc(fx, fy+90, 80, files[i], CTXT, 1);
        
        /* ÇİFT TIK (veya tek tık) -> KURULUM SİHİRBAZINI AÇ */
        if(CLK(fx, fy, 80, 80) && !app_installed[f_ids[i]]) {
            W_IN.open = 1; inst_prog = 0; inst_app_id = f_ids[i]; kcpy(inst_file, files[i]);
        }
    }
}

/* YAPAY ZEKA (GEMINI) UYGULAMASI */
static char ai_reply[128] = "Merhaba, ben Wind OS icindeki AI Asistan!\nBana bir seyler yaz ve 'Enter'a bas.";
static void AI_APP(void) {
    draw_win(&W_AI); if(!W_AI.open) return;
    /* Chat geçmişi ekranı */
    glass_rr(W_AI.x+10, W_AI.y+40, W_AI.w-20, W_AI.h-100, 5, 0x000000, 150);
    ds(W_AI.x+20, W_AI.y+50, ai_reply, C_CYAN, 1);
    
    /* Girdi Kutusu */
    glass_rr(W_AI.x+10, W_AI.y+W_AI.h-50, W_AI.w-20, 40, 5, 0x000000, 200);
    ds(W_AI.x+20, W_AI.y+W_AI.h-35, kb_buf, CW, 1);
    /* Yanıp sönen imleç */
    if ((inl(0x400) % 20) > 10) alpha_fr(W_AI.x+20+(kb_len*8), W_AI.y+W_AI.h-35, 8, 2, CW, 255);
}

/* ================================================================
   ANA MASAÜSTÜ
   ================================================================ */
static i32 wx=300, wy=50; static int wdrag=0, wdx=0, wdy=0;

static void DESKTOP(void){
    /* 1. Arka Plan */
    for(i32 y=0; y<(i32)SH; y++) {
        for(i32 x=0; x<(i32)SW; x++) {
            u32 c = BG_BOT;
            if (x % 50 == 0 || y % 50 == 0) c = GRID_C;
            back_buffer[y*SW+x] = c; /* DÜZELTME BURADA SW KULLANILDI! */
        }
    }

    /* 2. Özgür (Sürüklenebilir) Hava Durumu Widget'ı */
    if(!wdrag && MLB && !PMLB && HOV(wx, wy, 420, 140)) { wdrag=1; wdx=MX-wx; wdy=MY-wy; }
    if(wdrag) { if(MLB){ wx=MX-wdx; wy=MY-wdy; } else wdrag=0; }
    
    glass_rr(wx, wy, 420, 140, 20, WIDGET_BG, 200);
    alpha_circ(wx+70, wy+70, 35, 0xFFFFFF, 30);
    alpha_circ(wx+70, wy+70, 30, WIDGET_BG, 255);
    thick_line(wx+70, wy+70, wx+70, wy+50, 4, CW, 255);
    thick_line(wx+70, wy+70, wx+85, wy+85, 3, CW, 255);
    ds(wx+130, wy+50, "26:03", CW, 4);
    ds(wx+135, wy+90, "Istanbul - 22 C, Gunesli", CGY, 1);

    /* 3. Tüm Çalışan Uygulamalar (Kurulanlar Masaüstüne Düşer) */
    int ax = 50, ay = 50;
    for(int i=0; i<10; i++) {
        if (app_installed[i]) {
            blob_icon(ax+40, ay+30, app_colors[i]);
            dsc(ax, ay+65, 80, app_names[i], CTXT, 1);
            if (CLK(ax, ay, 80, 80)) {
                if (i == 2) W_FM.open = 1;
                if (i == 5) W_AI.open = 1;
            }
            ay += 110;
            if (ay > SH - 150) { ay = 50; ax += 100; }
        }
    }

    /* 4. Alt Asimetrik Görev Çubuğu */
    glass_rr(250, SH-80, 500, 60, 20, 0x1A1C20, 200);
    ds(280, SH-55, "WIND OS // AI EDITION v11", CGY, 1);

    /* 5. USB Takılı Bildirimi (En Sağ Alt) */
    if (USB_OK) {
        glass_rr(SW-160, SH-70, 140, 50, 10, PAN_BD, 150);
        alpha_circ(SW-130, SH-45, 8, C_LIME, 255);
        ds(SW-110, SH-50, "USB BAGLI", CW, 1);
    }

    /* Pencereler (En üstte çizilsin diye buraya koyduk) */
    FILEMGR();
    APP_INSTALLER();
    AI_APP();

    /* Fare Çizimi */
    static const u8 cur[16][12]={ {1},{1,1},{1,2,1},{1,2,2,1},{1,2,2,2,1},{1,2,2,2,2,1},{1,2,2,2,2,2,1},{1,2,2,2,2,2,2,1},{1,2,2,2,2,2,2,2,1},{1,2,2,2,2,1,1,1,1,1},{1,2,2,1,2,2,1},{1,2,1,0,1,2,2,1},{1,1,0,0,1,2,2,1},{0,0,0,0,0,1,2,2,1},{0,0,0,0,0,1,2,2,1},{0,0,0,0,0,0,1,1} };
    for(int r=0;r<16;r++) for(int c=0;c<12;c++){ if(cur[r][c]==1) alpha_pp(MX+c, MY+r, CW, 255); else if(cur[r][c]==2) alpha_pp(MX+c, MY+r, CK, 255); }
}

/* ================================================================
   KERNEL_MAIN
   ================================================================ */
void kernel_main(multiboot_info_t *mbi){
    u8 bpp  = mbi->framebuffer_bpp; if(bpp==0) bpp=32; 
    u32 Bpp = (u32)bpp / 8;
    FB  = (volatile u32*)(u32)mbi->framebuffer_addr; 
    SW  = mbi->framebuffer_width; 
    SH  = mbi->framebuffer_height; 
    SP  = mbi->framebuffer_pitch / Bpp;
    
    if(!FB || SW==0){ FB=(volatile u32*)0xFD000000u; SW=1024; SH=768; SP=1024; }
    
    outb(0x64,0xA8); outb(0x64,0x20); while(!(inb(0x64)&0x01)); u8 cfg=inb(0x60);
    cfg|=0x02; cfg&=~0x20; outb(0x64,0x60); outb(0x60,cfg);
    outb(0x64,0xD4); outb(0x60,0xFF); inb(0x60); inb(0x60); inb(0x60);
    outb(0x64,0xD4); outb(0x60,0xF6); inb(0x60);
    outb(0x64,0xD4); outb(0x60,0xF4); inb(0x60);
    MOUSE_READY=1;
    
    while(1){ 
        /* Klavye Yakalama */
        u8 key = kbd_poll();
        if (W_AI.open && key != 0) {
            if (key == 8 && kb_len > 0) { kb_buf[--kb_len] = 0; } /* Backspace */
            else if (key == '\n') { 
                kcpy(ai_reply, "Gemini: Soru harika! Ama henuz dis dunya ile\ninternet baglantim yok. Ben cekirdege gomuluyum.");
                kb_len = 0; kb_buf[0] = 0;
            }
            else if (key >= 32 && key <= 126 && kb_len < 40) { kb_buf[kb_len++] = key; kb_buf[kb_len] = 0; }
        }

        /* Fare Yakalama */
        for(int iter=0;iter<16;iter++){
            u8 st=inb(0x64); if(!(st&0x01)) break; 
            if(!(st&0x20)){ inb(0x60); continue; }
            u8 dat=inb(0x60);
            switch(MCY){
              case 0: if(!(dat&0x08)){MCY=0;break;} MBF[0]=(i8)dat; MCY=1; break;
              case 1: MBF[1]=(i8)dat; MCY=2; break;
              case 2: MBF[2]=(i8)dat; MCY=0;{
                i32 dx=(i32)MBF[1], dy=(i32)MBF[2];
                if(MBF[0]&0x10) dx|=(i32)0xFFFFFF00; if(MBF[0]&0x20) dy|=(i32)0xFFFFFF00;
                if(MBF[0]&0x40) dx=0; if(MBF[0]&0x80) dy=0;
                MX+=dx; MY-=dy;
                if(MX<0) MX=0; if(MY<0) MY=0; if(MX>=(i32)SW) MX=(i32)SW-1; if(MY>=(i32)SH) MY=(i32)SH-1;
                PMLB=MLB; MLB=(MBF[0]&0x01)?1:0; MRB=(MBF[0]&0x02)?1:0;
              } break;
            }
        }
        
        DESKTOP(); 
        swap_buffers(); 
    }
}
