/*
 * Wind OS / Sky-Core-OS  -  kernel.c  (TAM SURUM)
 * 7 OOBE + Masaustu + Cekmece + Dosya Yoneticisi
 * PS/2 Klavye, PS/2 Mouse, PCI/USB
 *
 * gcc -m32 -ffreestanding -fno-builtin -fno-stack-protector -O2 -Wall -c kernel.c -o kernel.o
 */
#include "kernel.h"

typedef unsigned int   u32;
typedef unsigned short u16;
typedef unsigned char  u8;
typedef int            i32;
typedef signed char    i8;
#define NULL ((void*)0)

/* ── FRAMEBUFFER ─────────────────────────────────────────── */
static u32 *FB     = (u32*)0;
static u32 SCR_W   = 1024;
static u32 SCR_H   = 768;
static u32 SCR_P   = 1024; /* pitch in pixels */

/* ── DURUM ────────────────────────────────────────────────── */
static OS_State g_state = STATE_SETUP_1_NAME;

/* ── RENKLER ─────────────────────────────────────────────── */
#define CB   0xFFF3F3F5u
#define CW   0xFFFFFFFFu
#define CK   0xFF000000u
#define CBL  0xFF0078D4u
#define CLL  0xFFDEECF9u
#define CGY  0xFF767676u
#define CLG  0xFFE5E5E5u
#define CMG  0xFFB0B0B0u
#define CDG  0xFF333333u
#define CRD  0xFFD13438u
#define CGN  0xFF107C10u
#define COR  0xFFFF8C00u
#define CBR  0xFFCFCFCFu
#define CTB  0xFF1E1E1Eu
#define CTH  0xFF3A3A3Au
#define CHD  0xFFEAEAF0u
#define CND  0xFF2D2D2Du

/* ── PORT I/O ────────────────────────────────────────────── */
static inline u8   inb (u16 p)       {u8  v;__asm__ volatile("inb  %1,%0":"=a"(v):"Nd"(p));return v;}
static inline void outb(u16 p,u8  v) {__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
static inline u32  inl (u16 p)       {u32 v;__asm__ volatile("inl  %1,%0":"=a"(v):"Nd"(p));return v;}
static inline void outl(u16 p,u32 v) {__asm__ volatile("outl %0,%1"::"a"(v),"Nd"(p));}

/* ── YARDIMCILAR ─────────────────────────────────────────── */
static void *memcpy_k(void *d,const void *s,u32 n){
    u8*dp=(u8*)d;const u8*sp=(const u8*)s;while(n--)*dp++=*sp++;return d;
}
static u32 klen(const char *s){u32 n=0;while(s[n])n++;return n;}
static void kcpy(char *d,const char *s){while(*s)*d++=*s++;*d=0;}

static u32 isqrt(u32 n){
    if(!n)return 0;u32 x=n,y=1;
    while(x>y){x=(x+y)/2;y=n/x;}return x;
}

/* ── 8x8 BITMAP FONT ─────────────────────────────────────── */
static const u8 F8[128][8]={
[' ']={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
['!']={0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00},
['"']={0x36,0x36,0x00,0x00,0x00,0x00,0x00,0x00},
['#']={0x36,0x7F,0x36,0x36,0x7F,0x36,0x36,0x00},
['$']={0x0C,0x3E,0x03,0x1E,0x30,0x1F,0x0C,0x00},
['%']={0x63,0x33,0x18,0x0C,0x66,0x63,0x00,0x00},
['&']={0x1C,0x36,0x1C,0x6E,0x3B,0x33,0x6E,0x00},
['\'']={0x06,0x0C,0x00,0x00,0x00,0x00,0x00,0x00},
['(']={0x18,0x0C,0x06,0x06,0x06,0x0C,0x18,0x00},
[')']={0x06,0x0C,0x18,0x18,0x18,0x0C,0x06,0x00},
['*']={0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00,0x00},
['+']={0x00,0x0C,0x0C,0x3F,0x0C,0x0C,0x00,0x00},
[',']={0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x06},
['-']={0x00,0x00,0x00,0x3F,0x00,0x00,0x00,0x00},
['.']={0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00},
['/']={0x60,0x30,0x18,0x0C,0x06,0x03,0x01,0x00},
['0']={0x3E,0x63,0x73,0x7B,0x6F,0x67,0x3E,0x00},
['1']={0x0C,0x0E,0x0C,0x0C,0x0C,0x0C,0x3F,0x00},
['2']={0x1E,0x33,0x30,0x1C,0x06,0x33,0x3F,0x00},
['3']={0x1E,0x33,0x30,0x1C,0x30,0x33,0x1E,0x00},
['4']={0x38,0x3C,0x36,0x33,0x7F,0x30,0x78,0x00},
['5']={0x3F,0x03,0x1F,0x30,0x30,0x33,0x1E,0x00},
['6']={0x1C,0x06,0x03,0x1F,0x33,0x33,0x1E,0x00},
['7']={0x3F,0x33,0x30,0x18,0x0C,0x0C,0x0C,0x00},
['8']={0x1E,0x33,0x33,0x1E,0x33,0x33,0x1E,0x00},
['9']={0x1E,0x33,0x33,0x3E,0x30,0x18,0x0E,0x00},
[':']={0x00,0x18,0x18,0x00,0x18,0x18,0x00,0x00},
[';']={0x00,0x18,0x18,0x00,0x18,0x18,0x0C,0x00},
['<']={0x18,0x0C,0x06,0x03,0x06,0x0C,0x18,0x00},
['=']={0x00,0x3F,0x00,0x00,0x3F,0x00,0x00,0x00},
['>']={0x06,0x0C,0x18,0x30,0x18,0x0C,0x06,0x00},
['?']={0x1E,0x33,0x30,0x18,0x0C,0x00,0x0C,0x00},
['@']={0x3E,0x63,0x7B,0x7B,0x7B,0x03,0x1E,0x00},
['A']={0x0C,0x1E,0x33,0x3F,0x33,0x33,0x33,0x00},
['B']={0x3F,0x66,0x66,0x3E,0x66,0x66,0x3F,0x00},
['C']={0x3C,0x66,0x03,0x03,0x03,0x66,0x3C,0x00},
['D']={0x1F,0x36,0x66,0x66,0x66,0x36,0x1F,0x00},
['E']={0x7F,0x46,0x16,0x1E,0x16,0x46,0x7F,0x00},
['F']={0x7F,0x46,0x16,0x1E,0x16,0x06,0x0F,0x00},
['G']={0x3C,0x66,0x03,0x03,0x73,0x66,0x7C,0x00},
['H']={0x33,0x33,0x33,0x3F,0x33,0x33,0x33,0x00},
['I']={0x1E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00},
['J']={0x78,0x30,0x30,0x30,0x33,0x33,0x1E,0x00},
['K']={0x67,0x66,0x36,0x1E,0x36,0x66,0x67,0x00},
['L']={0x0F,0x06,0x06,0x06,0x46,0x66,0x7F,0x00},
['M']={0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x00},
['N']={0x63,0x67,0x6F,0x7B,0x73,0x63,0x63,0x00},
['O']={0x1C,0x36,0x63,0x63,0x63,0x36,0x1C,0x00},
['P']={0x3F,0x66,0x66,0x3E,0x06,0x06,0x0F,0x00},
['Q']={0x1E,0x33,0x33,0x33,0x3B,0x1E,0x38,0x00},
['R']={0x3F,0x66,0x66,0x3E,0x36,0x66,0x67,0x00},
['S']={0x1E,0x33,0x07,0x0E,0x38,0x33,0x1E,0x00},
['T']={0x3F,0x2D,0x0C,0x0C,0x0C,0x0C,0x1E,0x00},
['U']={0x33,0x33,0x33,0x33,0x33,0x33,0x3F,0x00},
['V']={0x33,0x33,0x33,0x33,0x33,0x1E,0x0C,0x00},
['W']={0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00},
['X']={0x63,0x63,0x36,0x1C,0x36,0x63,0x63,0x00},
['Y']={0x33,0x33,0x33,0x1E,0x0C,0x0C,0x1E,0x00},
['Z']={0x7F,0x63,0x31,0x18,0x4C,0x66,0x7F,0x00},
['[']={0x1E,0x06,0x06,0x06,0x06,0x06,0x1E,0x00},
['\\']={0x03,0x06,0x0C,0x18,0x30,0x60,0x40,0x00},
[']']={0x1E,0x18,0x18,0x18,0x18,0x18,0x1E,0x00},
['^']={0x08,0x1C,0x36,0x63,0x00,0x00,0x00,0x00},
['_']={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF},
['`']={0x06,0x0C,0x18,0x00,0x00,0x00,0x00,0x00},
['a']={0x00,0x1E,0x30,0x3E,0x33,0x33,0x6E,0x00},
['b']={0x07,0x06,0x3E,0x66,0x66,0x66,0x3B,0x00},
['c']={0x00,0x1E,0x33,0x03,0x03,0x33,0x1E,0x00},
['d']={0x38,0x30,0x3E,0x33,0x33,0x33,0x6E,0x00},
['e']={0x00,0x1E,0x33,0x3F,0x03,0x33,0x1E,0x00},
['f']={0x1C,0x36,0x06,0x0F,0x06,0x06,0x0F,0x00},
['g']={0x00,0x6E,0x33,0x33,0x3E,0x30,0x33,0x1E},
['h']={0x07,0x06,0x36,0x6E,0x66,0x66,0x67,0x00},
['i']={0x0C,0x00,0x0E,0x0C,0x0C,0x0C,0x1E,0x00},
['j']={0x18,0x00,0x18,0x18,0x18,0x1B,0x1B,0x0E},
['k']={0x07,0x06,0x66,0x36,0x1E,0x36,0x67,0x00},
['l']={0x0E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00},
['m']={0x00,0x33,0x7F,0x7F,0x6B,0x63,0x63,0x00},
['n']={0x00,0x1F,0x33,0x33,0x33,0x33,0x33,0x00},
['o']={0x00,0x1E,0x33,0x33,0x33,0x33,0x1E,0x00},
['p']={0x00,0x3B,0x66,0x66,0x3E,0x06,0x06,0x0F},
['q']={0x00,0x6E,0x33,0x33,0x3E,0x30,0x30,0x78},
['r']={0x00,0x3B,0x6E,0x66,0x06,0x06,0x0F,0x00},
['s']={0x00,0x3E,0x03,0x1E,0x30,0x33,0x1E,0x00},
['t']={0x08,0x3E,0x0C,0x0C,0x0C,0x2C,0x18,0x00},
['u']={0x00,0x33,0x33,0x33,0x33,0x33,0x6E,0x00},
['v']={0x00,0x33,0x33,0x33,0x33,0x1E,0x0C,0x00},
['w']={0x00,0x63,0x6B,0x7F,0x7F,0x36,0x36,0x00},
['x']={0x00,0x63,0x36,0x1C,0x1C,0x36,0x63,0x00},
['y']={0x00,0x33,0x33,0x33,0x3E,0x30,0x33,0x1E},
['z']={0x00,0x3F,0x19,0x0C,0x26,0x3F,0x00,0x00},
['{']={0x38,0x0C,0x0C,0x07,0x0C,0x0C,0x38,0x00},
['|']={0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00},
['}']={0x07,0x0C,0x0C,0x38,0x0C,0x0C,0x07,0x00},
['~']={0x6E,0x3B,0x00,0x00,0x00,0x00,0x00,0x00},
};

/* ================================================================
   ÇİZİM FONKSİYONLARI
   ================================================================ */
static inline void pp(i32 x,i32 y,u32 c){
    if((u32)x<SCR_W&&(u32)y<SCR_H) FB[y*SCR_P+x]=c;
}
static void fr(i32 x,i32 y,i32 w,i32 h,u32 c){
    for(i32 j=y;j<y+h;j++) for(i32 i=x;i<x+w;i++) pp(i,j,c);
}
static void rb(i32 x,i32 y,i32 w,i32 h,u32 c,i32 t){
    fr(x,y,w,t,c);fr(x,y+h-t,w,t,c);
    fr(x,y,t,h,c);fr(x+w-t,y,t,h,c);
}
static void circ(i32 cx,i32 cy,i32 r,u32 c){
    for(i32 dy=-r;dy<=r;dy++)
        for(i32 dx=-r;dx<=r;dx++)
            if(dx*dx+dy*dy<=r*r) pp(cx+dx,cy+dy,c);
}
static void rr(i32 x,i32 y,i32 w,i32 h,i32 r,u32 c){
    fr(x+r,y,w-2*r,h,c);
    fr(x,y+r,r,h-2*r,c); fr(x+w-r,y+r,r,h-2*r,c);
    circ(x+r,y+r,r,c);   circ(x+w-r-1,y+r,r,c);
    circ(x+r,y+h-r-1,r,c); circ(x+w-r-1,y+h-r-1,r,c);
}
static void dc(i32 x,i32 y,char ch,u32 fg,u32 bg,i32 sc){
    if((u8)ch>=128)ch='?';
    const u8*g=F8[(u8)ch];
    for(i32 row=0;row<8;row++)
        for(i32 col=0;col<8;col++){
            u32 cc=(g[row]&(1<<(7-col)))?fg:bg;
            fr(x+col*sc,y+row*sc,sc,sc,cc);
        }
}
static void ds(i32 x,i32 y,const char*s,u32 fg,u32 bg,i32 sc){
    i32 cx=x;
    while(*s){
        if(*s=='\n'){cx=x;y+=8*sc;}
        else{dc(cx,y,*s,fg,bg,sc);cx+=8*sc;}
        s++;
    }
}
static void dsc(i32 x,i32 y,i32 w,const char*s,u32 fg,u32 bg,i32 sc){
    i32 tw=(i32)klen(s)*8*sc;
    ds(x+(w-tw)/2,y,s,fg,bg,sc);
}
/* WiFi yayı (üst yarı daire) */
static void warc(i32 cx,i32 cy,i32 r,i32 t,u32 c){
    for(i32 dx=-r;dx<=r;dx++){
        i32 dy=-(i32)isqrt((u32)(r*r-dx*dx));
        for(i32 k=0;k<t;k++) pp(cx+dx,cy+dy-k,c);
    }
}
/* Gradyan arkaplan */
static void grad(u32 top,u32 bot){
    for(u32 y=0;y<SCR_H-50;y++){
        u32 tr=(top>>16)&0xFF, tg=(top>>8)&0xFF, tb=top&0xFF;
        u32 br=(bot>>16)&0xFF, bg2=(bot>>8)&0xFF, bb=bot&0xFF;
        u32 r=tr+(br-tr)*y/(SCR_H-50);
        u32 g2=tg+(bg2-tg)*y/(SCR_H-50);
        u32 b2=tb+(bb-tb)*y/(SCR_H-50);
        fr(0,y,SCR_W,1,0xFF000000u|(r<<16)|(g2<<8)|b2);
    }
}

/* ================================================================
   PS/2 KLAVYE (POLLING)
   ================================================================ */
static u8 k_shift=0,k_caps=0;
static const char sc_normal[128]={
  0,27,'1','2','3','4','5','6','7','8','9','0','-','=',8,
  '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
  0,'a','s','d','f','g','h','j','k','l',';','\'','`',
  0,'\\','z','x','c','v','b','n','m',',','.','/',
  0,'*',0,' ',0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,'-',0,0,0,'+',0,0,0,0,0,0,0,0,0
};
static u8 kbd_poll(void){
    if(!(inb(0x64)&0x01)) return 0;
    u8 sc=inb(0x60);
    if(inb(0x64)&0x20) return 0;
    if(sc&0x80){
        u8 r=sc&0x7F;
        if(r==0x2A||r==0x36) k_shift=0;
        return 0;
    }
    if(sc==0x2A||sc==0x36){k_shift=1;return 0;}
    if(sc==0x3A){k_caps=!k_caps;return 0;}
    if(sc>=128) return 0;
    char c=sc_normal[sc]; if(!c) return 0;
    if(c>='a'&&c<='z'){ if(k_shift^k_caps) c-=32; }
    else if(k_shift){
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

/* ================================================================
   PS/2 MOUSE (POLLING)
   ================================================================ */
static i32 MX=512,MY=384,MLB=0,MRB=0,PMLB=0;
static u8  mcyc=0; static i8 mbuf[3]={0};

static void mwait_cmd(void){u32 t=100000;while(t--&&(inb(0x64)&0x02));}
static void mwait_dat(void){u32 t=100000;while(t--&&!(inb(0x64)&0x01));}
static void mwrite(u8 v){mwait_cmd();outb(0x64,0xD4);mwait_cmd();outb(0x60,v);}
static u8 mread(void){mwait_dat();return inb(0x60);}

static void mouse_init(void){
    u8 cfg;
    mwait_cmd(); outb(0x64,0xA8);
    mwait_cmd(); outb(0x64,0x20);
    mwait_dat(); cfg=inb(0x60);
    cfg|=0x02; cfg&=~0x20;
    mwait_cmd(); outb(0x64,0x60);
    mwait_cmd(); outb(0x60,cfg);
    mwrite(0xFF); mread(); mread(); mread();
    mwrite(0xF6); mread();
    mwrite(0xF4); mread();
}
static void mouse_poll(void){
    while(1){
        u8 st=inb(0x64);
        if(!(st&0x01)) break;
        u8 dat=inb(0x60);
        if(!(st&0x20)){(void)dat;continue;}
        switch(mcyc){
          case 0: if(!(dat&0x08))break; mbuf[0]=(i8)dat;mcyc=1;break;
          case 1: mbuf[1]=(i8)dat;mcyc=2;break;
          case 2: mbuf[2]=(i8)dat;mcyc=0;{
            i32 dx=mbuf[1]; if(mbuf[0]&0x10)dx|=~0xFF;
            i32 dy=mbuf[2]; if(mbuf[0]&0x20)dy|=~0xFF;
            MX+=dx; MY-=dy;
            if(MX<0)MX=0; if(MY<0)MY=0;
            if(MX>=(i32)SCR_W)MX=(i32)SCR_W-1;
            if(MY>=(i32)SCR_H)MY=(i32)SCR_H-1;
            PMLB=MLB;
            MLB=(mbuf[0]&0x01)?1:0;
            MRB=(mbuf[0]&0x02)?1:0;
          } break;
        }
    }
}
static int CLK(i32 x,i32 y,i32 w,i32 h){
    return MLB&&!PMLB&&MX>=x&&MX<x+w&&MY>=y&&MY<y+h;
}
static int HOV(i32 x,i32 y,i32 w,i32 h){
    return MX>=x&&MX<x+w&&MY>=y&&MY<y+h;
}

/* ================================================================
   PCI / USB ALGILAMA
   ================================================================ */
static u32 pci_rd(u8 bus,u8 dev,u8 fn,u8 off){
    outl(0xCF8,0x80000000u|((u32)bus<<16)|((u32)dev<<11)|((u32)fn<<8)|(off&0xFC));
    return inl(0xCFC);
}
static int USB_OK=0, STORAGE_OK=0;
static char USB_NAME[32]="USB Surucu (8GB)";
static void pci_scan(void){
    for(int b=0;b<8;b++) for(int d=0;d<32;d++){
        u32 id=pci_rd(b,d,0,0);
        if((id&0xFFFF)==0xFFFF) continue;
        u32 cls=pci_rd(b,d,0,8);
        u8 cc=(u8)(cls>>24),sc2=(u8)(cls>>16);
        if(cc==0x0C&&sc2==0x03) USB_OK=1;
        if(cc==0x01)            STORAGE_OK=1;
    }
}

/* ================================================================
   ORTAK UI BİLEŞENLERİ
   ================================================================ */
static int BTN(i32 x,i32 y,i32 w,i32 h,const char*lbl,u32 bg,u32 fg){
    u32 c=HOV(x,y,w,h)?(bg==CBL?0xFF1084DBu:0xFFD0D0D0u):bg;
    rr(x,y,w,h,4,c);
    dsc(x,y+(h-8)/2,w,lbl,fg,c,1);
    return CLK(x,y,w,h);
}
static int TGL(i32 x,i32 y,int on){
    rr(x,y,44,22,11,on?CBL:CGY);
    circ(on?x+32:x+11,y+11,9,CW);
    return CLK(x,y,44,22);
}
static int CHK(i32 x,i32 y,int v){
    fr(x,y,18,18,CW); rb(x,y,18,18,v?CBL:CBR,1);
    if(v){
        for(i32 i=0;i<4;i++){pp(x+3+i,y+10+i,CBL);pp(x+4+i,y+10+i,CBL);}
        for(i32 i=0;i<6;i++){pp(x+7+i,y+13-i,CBL);pp(x+8+i,y+13-i,CBL);}
    }
    return CLK(x,y,18,18);
}
static void WIN_LOGO(i32 cx,i32 cy,i32 sz){
    i32 h=sz/2,g=2;
    fr(cx-h,cy-h,h-g,h-g,0xFFF35325u);
    fr(cx+g, cy-h,h-g,h-g,0xFF81BC06u);
    fr(cx-h,cy+g, h-g,h-g,0xFF05A6F0u);
    fr(cx+g, cy+g, h-g,h-g,0xFFFFBA08u);
}
static void PROGRESS(int cur){
    i32 by=(i32)SCR_H-28;
    fr(0,by,(i32)SCR_W,2,CLG);
    i32 step=(i32)SCR_W/9;
    for(int i=0;i<7;i++){
        i32 px=step*(i+1),py=by+12;
        if(i<cur)        circ(px,py,5,CBL);
        else if(i==cur) {circ(px,py,7,CBL);circ(px,py,5,CBL);}
        else            {circ(px,py,5,CLG);circ(px,py,4,CB);}
    }
}
static void FOOTER(void){
    fr(0,(i32)SCR_H-46,(i32)SCR_W,46,CDG);
    WIN_LOGO(28,(i32)SCR_H-23,18);
    ds(44,(i32)SCR_H-31,"Wind OS",CW,CDG,1);
}
static void CURSOR(void){
    static const char AR[16][12]={
        "X           ","XX          ","X.X         ","X..X        ",
        "X...X       ","X....X      ","X.....X     ","X......X    ",
        "X.......X   ","X....XXXXX  ","X..X..X     ","X.X X..X    ",
        "XX  X..X    ","X    X..X   ","     X..X   ","      XX    "
    };
    for(int r=0;r<16;r++) for(int c=0;c<12;c++){
        i32 px=MX+c,py=MY+r;
        if((u32)px>=SCR_W||(u32)py>=SCR_H) continue;
        if(AR[r][c]=='X') pp(px,py,CK);
        else if(AR[r][c]=='.') pp(px,py,CW);
    }
}

/* ================================================================
   GLOBAL KURULUM DEGISKENLERI
   ================================================================ */
static char PC_NAME[64]={0}; static int PC_NL=0;
static int REG_SEL=0, KB_SEL=0, NET_SEL=-1;
static char WF_PW[64]={0}; static int WF_LEN=0; static int WF_INPUT=0;
static int PV_FB=1,PV_LOC=1,PV_ADS=1,PV_DG=0;
static int CU_ENT=1,CU_GAM=0,CU_SCH=0,CU_FAM=0;

/* ================================================================
   UYGULAMA + DOSYA SİSTEMİ VERİSİ
   ================================================================ */
typedef struct{char name[20];int inst;u32 col;} App;
static App APPS[8]={
    {"Mesajlar", 1,0xFF0078D4u},{"Terminal",1,0xFF1A1A1Au},
    {"Kamera",   1,0xFF107C10u},{"Harita",  1,0xFFD13438u},
    {"Hesap M.", 1,0xFF8B008Bu},{"Oyunlar", 0,0xFFFF8C00u},
    {"Tarayici", 1,0xFF005FB8u},{"Ayarlar", 1,0xFF606060u},
};
typedef struct{char name[32];int is_dir;} FSE;
static FSE LOCAL_FS[]={
    {"Masaustu",1},{"Belgeler",1},{"Indirmeler",1},
    {"Resimler",1},{"Muzik",   1},{"wind_os.cfg",0},{"README.txt",0},
};
static FSE USB_FS[]={
    {"Kurulum.exe",0},{"Oyun.exe",0},{"Notlar.txt",0},
};

/* ================================================================
   MASAUSTU DURUM
   ================================================================ */
static int DRW_OPEN=0;
static int FM_OPEN=0, FM_USB=0;
static i32 FM_X=80,FM_Y=60;
static int FM_DRAG=0; static i32 FM_DOX=0,FM_DOY=0;
static int FM_SEL=-1;
static int USB_TICK=0;

/* ================================================================
   EKRAN 1 — BİLGİSAYAR ADI
   ================================================================ */
static void S1(u8 key){
    fr(0,0,(i32)SCR_W,(i32)SCR_H,CB);

    /* Sol panel */
    fr(0,0,390,(i32)SCR_H-46,CHD);
    /* Laptop simgesi */
    fr(90,150,200,140,CDG);
    fr(94,154,192,132,0xFF1A2A40u);
    circ(190,218,28,0xFF4FC3F7u);
    /* smiley */
    circ(179,210,4,CW); circ(201,210,4,CW);
    for(i32 i=-10;i<=10;i++) pp(190+i,228+(i*i/14),CW);
    fr(60,295,280,14,CDG);

    /* Sağ panel */
    i32 rx=420,ry=68;
    ds(rx,ry,"Bilgisayariniza bir ad verin",CDG,CB,2); ry+=42;
    ds(rx,ry,"Bu cihazi agda nasil tanimlayacaksiniz?",CGY,CB,1); ry+=16;
    ds(rx,ry,"Ileride degistirebilirsiniz.",CGY,CB,1); ry+=36;

    /* Input kutusu */
    fr(rx,ry,510,38,CW); rb(rx,ry,510,38,CBL,2);
    if(PC_NL) ds(rx+10,ry+13,PC_NAME,CDG,CW,1);
    else ds(rx+10,ry+13,"Sunucu",CMG,CW,1);
    fr(rx+10+PC_NL*8,ry+9,2,20,CBL);

    ry+=52;
    ds(rx,ry,"19 karakterden az olmali.",CGY,CB,1); ry+=60;

    if(BTN(rx+310,ry,150,36,"Simdilik Atla",CLG,CDG)) g_state=STATE_SETUP_2_REGION;
    if(BTN(rx+470,ry,80,36,"Ileri",CBL,CW))           g_state=STATE_SETUP_2_REGION;

    FOOTER(); PROGRESS(0);

    /* klavye */
    if(key==8&&PC_NL>0){PC_NAME[--PC_NL]=0;}
    else if(key>=32&&key<127&&PC_NL<18){PC_NAME[PC_NL++]=(char)key;PC_NAME[PC_NL]=0;}
}

/* ================================================================
   EKRAN 2 — BÖLGE
   ================================================================ */
static void S2(void){
    fr(0,0,(i32)SCR_W,(i32)SCR_H,CB);
    fr(0,0,390,(i32)SCR_H-46,CHD);

    /* Dünya */
    circ(195,280,88,0xFF2196F3u);
    fr(132,248,50,36,0xFF4CAF50u); fr(200,258,52,44,0xFF4CAF50u);
    fr(148,290,32,28,0xFF4CAF50u); fr(212,286,38,36,0xFF4CAF50u);
    for(i32 dy=-96;dy<=96;dy++) for(i32 dx=-96;dx<=96;dx++)
        if((u32)(dx*dx+dy*dy)>(u32)(88*88)) pp(195+dx,280+dy,CHD);

    i32 rx=420,ry=70;
    ds(rx,ry,"Bu dogru ulke/bolge mi?",CDG,CB,2); ry+=56;

    const char*regs[]={"Turkiye","Turkce Q","Turkce F"};
    for(int i=0;i<3;i++){
        u32 bg=(REG_SEL==i)?CBL:CW; u32 fg=(REG_SEL==i)?CW:CDG;
        fr(rx,ry,510,42,bg); rb(rx,ry,510,42,CBR,1);
        ds(rx+16,ry+15,regs[i],fg,bg,1);
        if(CLK(rx,ry,510,42)) REG_SEL=i;
        ry+=48;
    }
    ry+=28;
    if(BTN(rx+430,ry,80,36,"Evet",CBL,CW)) g_state=STATE_SETUP_3_KEYBOARD;
    FOOTER(); PROGRESS(1);
}

/* ================================================================
   EKRAN 3 — KLAVYE
   ================================================================ */
static void S3(void){
    fr(0,0,(i32)SCR_W,(i32)SCR_H,CB);
    fr(0,0,390,(i32)SCR_H-46,CHD);

    /* Klavye illüstrasyonu */
    const char*rows[]={"QWERTYUIOP","ASDFGHJKL","ZXCVBNM"};
    i32 off[]={0,14,26};
    for(int r=0;r<3;r++){
        for(int k=0;rows[r][k];k++){
            i32 bx=44+off[r]+k*30, by=218+r*36;
            fr(bx,by,26,28,CW); rb(bx,by,26,28,CMG,1);
            char s[2]={(char)rows[r][k],0}; ds(bx+9,by+9,s,CDG,CW,1);
        }
    }
    /* Space */
    fr(80,326,228,28,CW); rb(80,326,228,28,CMG,1);

    i32 rx=420,ry=60;
    ds(rx,ry,"Bu, dogru klavye duzeni...",CDG,CB,2); ry+=40;
    ds(rx,ry,"Dogru ise devam edin, degilse secin.",CGY,CB,1); ry+=36;

    const char*lays[]={"Layout","Layout A_BD","Layout 2-Dlame","Layout S-E","Layout S-G","Layout S-H"};
    for(int i=0;i<6;i++){
        u32 bg=(KB_SEL==i)?CLL:CW;
        fr(rx,ry,510,34,bg); rb(rx,ry,510,34,CBR,1);
        ds(rx+12,ry+12,lays[i],CDG,bg,1);
        if(CLK(rx,ry,510,34)) KB_SEL=i;
        ry+=38;
    }
    ry+=14;
    if(BTN(rx+430,ry,80,36,"Evet",CBL,CW)) g_state=STATE_SETUP_4_NETWORK;
    FOOTER(); PROGRESS(2);
}

/* ================================================================
   EKRAN 4 — AĞ
   ================================================================ */
static void S4(u8 key){
    fr(0,0,(i32)SCR_W,(i32)SCR_H,CB);
    fr(0,0,390,(i32)SCR_H-46,CHD);

    /* WiFi yayları */
    i32 wcx=195,wcy=310;
    circ(wcx,wcy+36,9,0xFF2196F3u);
    warc(wcx,wcy+36,32,5,0xFF2196F3u);
    warc(wcx,wcy+36,60,5,0xFF2196F3u);
    warc(wcx,wcy+36,88,5,0xFF2196F3u);

    i32 rx=420,ry=58;
    ds(rx,ry,"Hadi sizi bir aga baglayalim",CDG,CB,2); ry+=32;
    ds(rx,ry,"Baglanti olusturun veya mevcut aga baglanin.",CGY,CB,1); ry+=48;

    const char*nets[]={"Sky.Net-Giga","Sky.Net-Giga (Baglandı)"};
    for(int i=0;i<2;i++){
        u32 bg=(NET_SEL==i)?CLL:CW;
        fr(rx,ry,510,50,bg); rb(rx,ry,510,50,CBR,1);
        circ(rx+22,ry+25,6,0xFF2196F3u);
        ds(rx+40,ry+19,nets[i],CDG,bg,1);
        if(CLK(rx,ry,510,50)){NET_SEL=i;WF_INPUT=1;}
        ry+=56;
    }

    if(WF_INPUT&&NET_SEL>=0){
        ry+=10; ds(rx,ry,"Parola:",CDG,CB,1); ry+=20;
        fr(rx,ry,510,36,CW); rb(rx,ry,510,36,CBL,2);
        char dots[65]={0};
        for(int i=0;i<WF_LEN&&i<48;i++) dots[i]='*';
        ds(rx+10,ry+12,dots,CDG,CW,1);
        fr(rx+10+WF_LEN*8,ry+8,2,20,CBL);
        ry+=50;
        BTN(rx+300,ry,100,36,"Iptal",CLG,CDG);
        if(BTN(rx+410,ry,100,36,"Baglan",CBL,CW)){g_state=STATE_SETUP_5_PRIVACY;WF_INPUT=0;}
        if(key==8&&WF_LEN>0){WF_PW[--WF_LEN]=0;}
        else if(key>=32&&key<127&&WF_LEN<32){WF_PW[WF_LEN++]=(char)key;WF_PW[WF_LEN]=0;}
    } else {
        ry+=14;
        BTN(rx+280,ry,120,36,"Aglar Goster",CLG,CDG);
        if(BTN(rx+410,ry,100,36,"Ilerle",CBL,CW)) g_state=STATE_SETUP_5_PRIVACY;
    }
    FOOTER(); PROGRESS(3);
}

/* ================================================================
   EKRAN 5 — GİZLİLİK
   ================================================================ */
static void S5(void){
    fr(0,0,(i32)SCR_W,(i32)SCR_H,CB);
    fr(0,0,390,(i32)SCR_H-46,CHD);

    /* Kalkan */
    for(i32 dy=-78;dy<=78;dy++){
        i32 hw=dy<0?68:68-dy; if(hw<0)hw=0;
        fr(195-hw,280+dy,hw*2,1,0xFF1565C0u);
    }
    circ(195,280,18,0xFFFFD700u);
    fr(189,265,12,18,0xFF1565C0u);
    rb(188,262,14,20,0xFFFFD700u,3);

    i32 rx=420,ry=48;
    ds(rx,ry,"Cihaziniz icin gizlilik",CDG,CB,2); ry+=22;
    ds(rx,ry,"ayarlarini secin",CDG,CB,2); ry+=38;
    ds(rx,ry,"Asagidaki ozellikleri acip kapatabilirsiniz.",CGY,CB,1); ry+=32;

    typedef struct{const char*nm;const char*desc;int*val;}PI;
    PI items[]={
        {"Geri bildirim","Sistem geri bildirimlerini yonet",&PV_FB},
        {"Konum",        "Konumunuzu uygulamalar ile paylasin",&PV_LOC},
        {"Reklam ID",    "Kisisel reklamlar icin kimlik kullan",&PV_ADS},
        {"Teshis",       "Teshis verisini paylasın",&PV_DG},
    };
    for(int i=0;i<4;i++){
        fr(rx,ry,510,56,CW); rb(rx,ry,510,56,CBR,1);
        ds(rx+12,ry+10,items[i].nm,CDG,CW,1);
        ds(rx+12,ry+24,items[i].desc,CGY,CW,1);
        ds(rx+418,ry+22,*items[i].val?"Evet":"Hayir",*items[i].val?CBL:CGY,CW,1);
        if(TGL(rx+460,ry+17,*items[i].val)) *items[i].val=!*items[i].val;
        ry+=60;
    }
    ry+=10;
    ds(rx,ry,"Daha fazla bilgi edinin",CBL,CB,1); ry+=28;
    if(BTN(rx+420,ry,90,36,"Kabul Et",CBL,CW)) g_state=STATE_SETUP_6_CUSTOMIZE;
    FOOTER(); PROGRESS(4);
}

/* ================================================================
   EKRAN 6 — ÖZELLEŞTIRME
   ================================================================ */
static void S6(void){
    fr(0,0,(i32)SCR_W,(i32)SCR_H,CB);
    fr(0,0,390,(i32)SCR_H-46,CHD);

    /* Dekoratif ikonlar */
    u32 ic[]={0xFF2196F3u,0xFF4CAF50u,0xFFFF9800u,0xFF9C27B0u};
    i32 ip[][2]={{85,210},{245,195},{85,320},{245,305}};
    const char*il[]={"ENT","GAM","SCH","FAM"};
    for(int i=0;i<4;i++){
        circ(ip[i][0],ip[i][1],30,ic[i]);
        dsc(ip[i][0]-22,ip[i][1]-4,44,il[i],CW,ic[i],1);
    }

    i32 rx=420,ry=48;
    ds(rx,ry,"Deneyiminizi ozellestirelim",CDG,CB,2); ry+=30;
    ds(rx,ry,"Sectiginiz ilgi alanlarina gore oneriler",CGY,CB,1); ry+=16;
    ds(rx,ry,"ve ipuclari sunulabilir.",CGY,CB,1); ry+=30;

    typedef struct{const char*nm;const char*desc;int*val;u32 col;}CI;
    CI ci[]={
        {"Eglence","Video, muzik ve medyayi kesfet",&CU_ENT,0xFF2196F3u},
        {"Oyun",   "Oyunlari ve yeni surumleri takip et",&CU_GAM,0xFF4CAF50u},
        {"Okul",   "Odev ve projelerde verimli calis",&CU_SCH,0xFFFF9800u},
        {"Aile",   "Aile uyeleriyle guvenli kalin",&CU_FAM,0xFF9C27B0u},
    };
    for(int i=0;i<4;i++){
        fr(rx,ry,510,56,CW); rb(rx,ry,510,56,CBR,1);
        circ(rx+26,ry+28,18,ci[i].col);
        ds(rx+52,ry+12,ci[i].nm,CDG,CW,1);
        ds(rx+52,ry+26,ci[i].desc,CGY,CW,1);
        if(CHK(rx+482,ry+19,*ci[i].val)) *ci[i].val=!*ci[i].val;
        ry+=60;
    }
    ry+=12;
    BTN(rx+288,ry,112,36,"Atla",CLG,CDG);
    if(BTN(rx+410,ry,100,36,"Kabul Et",CBL,CW)) g_state=STATE_SETUP_7_WELCOME;
    FOOTER(); PROGRESS(5);
}

/* ================================================================
   EKRAN 7 — HOŞ GELDİNİZ
   ================================================================ */
static void S7(void){
    fr(0,0,(i32)SCR_W,(i32)SCR_H,CB);
    fr(0,0,390,(i32)SCR_H-46,CHD);
    FOOTER();

    /* Bildirim kartı */
    i32 cx=(i32)SCR_W/2-205, cy=72;
    fr(cx+5,cy+5,410,200,0x55000000u);
    fr(cx,cy,410,200,CDG);
    rb(cx,cy,410,200,0xFF3A3A5Au,1);

    /* Kapat X */
    fr(cx+388,cy+6,18,18,CRD); dsc(cx+388,cy+10,18,"X",CW,CRD,1);
    if(CLK(cx+388,cy+6,18,18)) g_state=STATE_DESKTOP;

    circ(cx+14,cy+18,7,CBL);
    ds(cx+26,cy+12,"HOS GELDINIZ",CW,CDG,1);
    ds(cx+12,cy+46,"Sisteme Hos Geldiniz!",CW,CDG,1);
    ds(cx+12,cy+64,"Wind OS hazir.",CMG,CDG,1);

    u32 ac[]={CBL,CGN,0xFF9C27B0u,CRD};
    const char*al[]={"MSG","TRM","HAR","KMR"};
    for(int i=0;i<4;i++){
        i32 ax=cx+12+i*98,ay=cy+112;
        rr(ax,ay,86,66,6,ac[i]);
        dsc(ax,ay+28,86,al[i],CW,ac[i],1);
    }

    dsc(0,340,(i32)SCR_W,"Wind OS'e Hos Geldiniz!",CDG,CB,2);
    dsc(0,380,(i32)SCR_W,PC_NL?PC_NAME:"Kullanici",CGY,CB,1);

    if(BTN((i32)SCR_W/2-110,438,220,44,"Masaustu Gir",CBL,CW)){
        pci_scan(); g_state=STATE_DESKTOP;
    }
    PROGRESS(6);
}

/* ================================================================
   MASAÜSTÜ — ÇEKMECE (Start Menu)
   ================================================================ */
static void DRAWER(void){
    if(!DRW_OPEN) return;
    i32 dh=440,dx=60,dw=(i32)SCR_W-120;
    i32 dy=(i32)SCR_H-50-dh;

    rr(dx,dy,dw,dh,10,0xEE1C1C28u);
    rb(dx,dy,dw,dh,0xFF3A3A5Au,1);

    dsc(dx,dy+14,dw,"UYGULAMALAR",CW,0xFF1C1C28u,1);
    dsc(dx,dy+28,dw,"Tiklayin: yukle veya calistir",CMG,0xFF1C1C28u,1);

    for(int i=0;i<8;i++){
        i32 col=i%4, row=i/4;
        i32 ix=dx+24+col*((dw-48)/4);
        i32 iy=dy+56+row*136;

        u32 bg=APPS[i].inst?APPS[i].col:CTH;
        rr(ix,iy,76,76,10,bg);
        char ab[3]={APPS[i].name[0],APPS[i].name[1],0};
        dsc(ix,iy+34,76,ab,CW,bg,1);
        dsc(ix-6,iy+80,88,APPS[i].name,CW,0xFF1C1C28u,1);

        if(!APPS[i].inst){
            rr(ix+10,iy+90,56,20,4,CBL);
            dsc(ix+10,iy+96,56,"Yukle",CW,CBL,1);
            if(CLK(ix+10,iy+90,56,20)) APPS[i].inst=1;
        }
    }
    /* Çekmeceye tık dışı kapatma */
    if(MLB&&!PMLB){
        if(!(MX>=(i32)dx&&MX<dx+dw&&MY>=(i32)dy&&MY<dy+dh)&&
           !(MX>=8&&MX<44&&MY>=(i32)SCR_H-44&&MY<(i32)SCR_H-6))
            DRW_OPEN=0;
    }
}

/* ================================================================
   MASAÜSTÜ — DOSYA YÖNETİCİSİ
   ================================================================ */
static void FILEMGR(void){
    if(!FM_OPEN) return;
    i32 fw=620,fh=420,fx=FM_X,fy=FM_Y;

    /* Gölge + pencere */
    fr(fx+4,fy+4,fw,fh,0xAA000000u);
    fr(fx,fy,fw,fh,CW);
    rb(fx,fy,fw,fh,CBR,1);

    /* Başlık */
    fr(fx,fy,fw,34,CDG);
    circ(fx+14,fy+17,7,CRD);
    circ(fx+32,fy+17,7,COR);
    circ(fx+50,fy+17,7,CGN);
    if(CLK(fx+7,fy+10,14,14)){FM_OPEN=0;return;}
    ds(fx+66,fy+12,"Dosya Yoneticisi",CW,CDG,1);

    /* Adres çubuğu */
    fr(fx,fy+34,fw,28,CLG);
    if(FM_USB){
        ds(fx+8,fy+42,"Bu Bilgisayar > USB Surucu",CDG,CLG,1);
        if(CLK(fx+8,fy+40,130,16)) FM_USB=0;
    } else {
        ds(fx+8,fy+42,"Bu Bilgisayar",CDG,CLG,1);
    }

    /* Sol kenar çubuğu */
    i32 sb=128;
    fr(fx,fy+62,sb,fh-62,0xFFF0F0F4u);
    const char*side[]={"Bu Bilgisayar","Masaustu","Belgeler","Resimler","Muzik"};
    for(int i=0;i<5;i++) ds(fx+8,fy+72+i*18,side[i],CDG,0xFFF0F0F4u,1);

    if(USB_OK){
        circ(fx+12,fy+170,6,CBL);
        ds(fx+22,fy+165,USB_NAME,CBL,0xFFF0F0F4u,1);
        if(CLK(fx+6,fy+160,sb-6,20)) FM_USB=1;
    }

    /* İçerik */
    i32 cx2=fx+sb+8, cy2=fy+68;
    FSE *entries=FM_USB?USB_FS:LOCAL_FS;
    int cnt=FM_USB?3:7;

    for(int i=0;i<cnt;i++){
        i32 ex=cx2+(i%4)*118, ey=cy2+(i/4)*100;
        if(ex+100>fx+fw||ey+90>fy+fh) continue;
        u32 bg=(FM_SEL==i)?CLL:CW;
        fr(ex,ey,100,88,bg); rb(ex,ey,100,88,CBR,1);

        if(entries[i].is_dir){
            fr(ex+16,ey+8,58,16,0xFFFFD700u);
            fr(ex+8,ey+20,72,40,0xFFFFE44Du);
        } else {
            fr(ex+20,ey+8,46,48,CW); rb(ex+20,ey+8,46,48,CMG,1);
            fr(ex+50,ey+8,16,14,CLG);
            int nl=(int)klen(entries[i].name);
            if(nl>4){
                char ext[5]={0};
                ext[0]=entries[i].name[nl-4]; ext[1]=entries[i].name[nl-3];
                ext[2]=entries[i].name[nl-2]; ext[3]=entries[i].name[nl-1];
                if(ext[1]=='e'&&ext[2]=='x'&&ext[3]=='e'){
                    fr(ex+21,ey+36,44,14,CRD);
                    dsc(ex+21,ey+38,44,".exe",CW,CRD,1);
                }
            }
        }
        char sn[12]={0};
        int nl=(int)klen(entries[i].name);
        if(nl>10){memcpy_k(sn,entries[i].name,9);sn[9]='.';sn[10]='.';}
        else kcpy(sn,entries[i].name);
        ds(ex+4,ey+72,sn,CDG,bg,1);

        if(CLK(ex,ey,100,88)){
            FM_SEL=i;
            /* .exe tıklama = uygulama kur */
            int nln=(int)klen(entries[i].name);
            if(!entries[i].is_dir&&nln>4){
                char ext4[5]={0};
                ext4[0]=entries[i].name[nln-4]; ext4[1]=entries[i].name[nln-3];
                ext4[2]=entries[i].name[nln-2]; ext4[3]=entries[i].name[nln-1];
                if(ext4[1]=='e'&&ext4[2]=='x'&&ext4[3]=='e')
                    for(int a=0;a<8;a++) if(!APPS[a].inst){APPS[a].inst=1;break;}
            }
        }
    }

    /* Durum çubuğu */
    fr(fx,fy+fh-20,fw,20,CLG);
    ds(fx+8,fy+fh-14,USB_OK?"USB takili":"USB takili degil",CGY,CLG,1);

    /* Pencere sürükleme */
    if(!FM_DRAG&&MLB&&!PMLB&&MY>=fy&&MY<fy+34&&MX>=fx&&MX<fx+fw){
        FM_DRAG=1; FM_DOX=MX-fx; FM_DOY=MY-fy;
    }
    if(FM_DRAG){
        if(MLB){
            FM_X=MX-FM_DOX; FM_Y=MY-FM_DOY;
            if(FM_X<0)FM_X=0; if(FM_Y<0)FM_Y=0;
            if(FM_X>(i32)SCR_W-fw)FM_X=(i32)SCR_W-fw;
            if(FM_Y>(i32)SCR_H-fh)FM_Y=(i32)SCR_H-fh;
        } else FM_DRAG=0;
    }
}

/* ================================================================
   MASAÜSTÜ ANA EKRANI
   ================================================================ */
static void DESKTOP(void){
    /* Arkaplan gradyanı */
    grad(0xFF0A0A1Eu,0xFF1A2840u);

    /* Filigran */
    dsc(0,(i32)SCR_H/2-8,(i32)SCR_W,"Wind OS",0xFF1E1E3Au,0xFF000000u,2);

    /* Hava durumu widget */
    rr((i32)SCR_W-194,10,184,72,6,0xAA1A2040u);
    ds((i32)SCR_W-190,18,"Hava Durumu ve Saat",CW,0xFF1A2040u,1);
    ds((i32)SCR_W-190,32,"Istanbul:  22 C",CW,0xFF1A2040u,1);
    ds((i32)SCR_W-190,46,"Parcali Bulutlu",CMG,0xFF1A2040u,1);
    ds((i32)SCR_W-190,60,"26:03  01.10",CW,0xFF1A2040u,1);

    /* Çekmece (Start Menu) */
    DRAWER();

    /* Dosya yöneticisi */
    FILEMGR();

    /* ── GÖREV ÇUBUĞU ── */
    fr(0,(i32)SCR_H-50,(i32)SCR_W,50,CTB);

    /* Windows logo butonu */
    WIN_LOGO(26,(i32)SCR_H-25,20);
    if(CLK(8,(i32)SCR_H-44,36,38)) DRW_OPEN=!DRW_OPEN;

    /* Görev çubuğu butonları */
    typedef struct{const char*lbl;int*flag;}TB;
    TB tbs[]={{"FM",&FM_OPEN},{"TB",NULL},{"MS",NULL},{"TM",NULL}};
    for(int i=0;i<4;i++){
        i32 tx=62+i*52, ty=(i32)SCR_H-44;
        int active=(tbs[i].flag&&*tbs[i].flag);
        rr(tx,ty,44,34,4,active?CTH:CTB);
        if(active){ fr(tx+16,ty+32,12,4,CBL); }
        dsc(tx,ty+12,44,tbs[i].lbl,CW,active?CTH:CTB,1);
        if(CLK(tx,ty,44,34)&&tbs[i].flag) *tbs[i].flag=!*tbs[i].flag;
    }

    /* Saat */
    ds((i32)SCR_W-92,(i32)SCR_H-38,"26:03",CW,CTB,1);
    ds((i32)SCR_W-96,(i32)SCR_H-22,"01.10.27",CW,CTB,1);

    /* USB göstergesi */
    if(USB_OK){
        circ((i32)SCR_W-112,(i32)SCR_H-25,5,CBL);
        ds((i32)SCR_W-104,(i32)SCR_H-29,"USB",CW,CTB,1);
    }

    /* USB yeniden tarama */
    if(++USB_TICK>3000){ pci_scan(); USB_TICK=0; }
}

/* ================================================================
   GECIKME
   ================================================================ */
static void delay(int n){ volatile int x=n*6000; while(x--)__asm__("nop"); }

/* ================================================================
   KERNEL MAIN
   ================================================================ */
void kernel_main(multiboot_info_t *mbi){
    /* Framebuffer */
    FB    = (u32*)(unsigned long)mbi->framebuffer_addr;
    SCR_W = mbi->framebuffer_width;
    SCR_H = mbi->framebuffer_height;
    SCR_P = mbi->framebuffer_pitch / 4;

    /* Yedek (GRUB VBE atamadıysa) */
    if(!FB||SCR_W==0){
        FB=(u32*)0xFD000000u; SCR_W=1024; SCR_H=768; SCR_P=1024;
    }

    mouse_init();
    pci_scan();

    while(1){
        mouse_poll();
        u8 key=kbd_poll();

        switch(g_state){
            case STATE_SETUP_1_NAME:      S1(key); break;
            case STATE_SETUP_2_REGION:    S2();    break;
            case STATE_SETUP_3_KEYBOARD:  S3();    break;
            case STATE_SETUP_4_NETWORK:   S4(key); break;
            case STATE_SETUP_5_PRIVACY:   S5();    break;
            case STATE_SETUP_6_CUSTOMIZE: S6();    break;
            case STATE_SETUP_7_WELCOME:   S7();    break;
            case STATE_DESKTOP:           DESKTOP();break;
        }
        CURSOR();
        delay(5);
    }
}
