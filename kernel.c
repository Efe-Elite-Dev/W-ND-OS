/*
 * Wind OS / Sky-Core-OS  -  kernel.c  (TAM SURUM - KILITLENMEYEN SURUCU ENTEGRELİ)
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

/* ── DOUBLE BUFFERING (GİT-GEL ENGELLEYİCİ TAMPON) ───────── */
static u32 back_buffer[1024 * 768];

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
['{']={0x38,0x0C,0x0C,0x07,0x0C,0x0C,0x38,0x00},
['|']={0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00},
['}']={0x07,0x0C,0x0C,0x38,0x0C,0x0C,0x07,0x00},
['~']={0x6E,0x3B,0x00,0x00,0x00,0x00,0x00,0x00},
};

/* ================================================================
   ÇİZİM FONKSİYONLARI (DOUBLE BUFFER DESTEKLİ)
   ================================================================ */
static inline void pp(i32 x,i32 y,u32 c){
    if((u32)x<SCR_W&&(u32)y<SCR_H) back_buffer[y*SCR_P+x]=c;
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
static void warc(i32 cx,i32 cy,i32 r,i32 t,u32 c){
    for(i32 dx=-r;dx<=r;dx++){
        i32 dy=-(i32)isqrt((u32)(r*r-dx*dx));
        for(i32 k=0;k<t;k++) pp(cx+dx,cy+dy-k,c);
    }
}
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

/* TAMPONU CANLI EKRANA KOPYALAMA MOTORU */
static void swap_buffers(void) {
    u32 total_pixels = SCR_W * SCR_H;
    for(u32 i = 0; i < total_pixels; i++) {
        FB[i] = back_buffer[i];
    }
}

/* ================================================================
   PS/2 KLAVYE SÜRÜCÜSÜ (KİLİTLENMEYEN GÜVENLİ POLLING)
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
    u8 st = inb(0x64);
    if(!(st & 0x01)) return 0;   // Okunacak veri yoksa hemen cik
    if(st & 0x20) return 0;      // KRİTİK: Veri fareye aitse klavye okumasın, yutmasın!
    
    u8 sc = inb(0x60);           // Güvenle oku
    if(sc & 0x80){
        u8 r = sc & 0x7F;
        if(r == 0x2A || r == 0x36) k_shift = 0;
        return 0;
    }
    if(sc == 0x2A || sc == 0x36){ k_shift = 1; return 0; }
    if(sc == 0x3A){ k_caps = !k_caps; return 0; }
    if(sc >= 128) return 0;
    
    char c = sc_normal[sc];
    if(!c) return 0;
    if(c >= 'a' && c <= 'z'){
        if(k_shift ^ k_caps) c -= 32;
    } else if(k_shift){
        switch(c){
            case '1':c='!';break;
            case '2':c='@';break;
            case '3':c='#';break;
            case '4':c='$';break;
            case '5':c='%';break;
            case '6':c='^';break;
            case '7':c='&';break;
            case '8':c='*';break;
            case '9':c='(';break;
            case '0':c=')';break;
            case '-':c='_';break;
            case '=':c='+';break;
            case ';':c=':';break;
            case '\'':c='"';break;
            case '`':c='~';break;
            case ',':c='<';break;
            case '.':c='>';break;
            case '/':c='?';break;
            case '\\':c='|';break;
        }
    }
    return (u8)c;
}

/* ================================================================
   PS/2 MOUSE SÜRÜCÜSÜ (KİLİTLENMEYEN GÜVENLİ POLLING)
   ================================================================ */
static i32 MX=512, MY=384;
static u8  MB=0;
static void mouse_wait(u8 a){
    u32 t=50000;
    if(a==0){ while(t--&&!(inb(0x64)&1)); }
    else    { while(t--&&(inb(0x64)&2)); }
}
static void mouse_w(u8 b){ mouse_wait(1); outb(0x64,0xD4); mouse_wait(1); outb(0x60,b); }
static u8 mouse_r(void){ mouse_wait(0); return inb(0x60); }

static void mouse_init(void){
    // İlk önce porttaki eski kalıntıları tamamen boşalt
    while(inb(0x64) & 1) { inb(0x60); }

    mouse_wait(1); outb(0x64,0xA8); // Fare hattını aç
    mouse_wait(1); outb(0x64,0x20); // Komut oku
    mouse_wait(0); u8 s=(inb(0x60)|2)&(~0x20);
    mouse_wait(1); outb(0x64,0x60); // Komut yaz
    mouse_wait(1); outb(0x60,s);
    
    mouse_w(0xF6); mouse_r(); // Varsayılanlar
    mouse_w(0xF4); mouse_r(); // Veri raporlamayı aç
}

static void mouse_poll(void){
    while(1){
        u8 st = inb(0x64);
        if(!(st & 0x01)) break;   // Okunacak veri kalmadıysa döngüden güvenle çık
        if(!(st & 0x20)) break;   // KRİTİK: Veri klavyeye aitse fare okumasın, klavyeye bıraksın!
        
        u8 dat = inb(0x60);       // Güvenle oku
        static u8 p[3], pc=0;
        p[pc++]=dat;
        if(pc==3){
            pc=0;
            if((p[0]&0x08)){
                u8 b=p[0]&7; MB=b;
                i32 dx=(p[0]&0x10)?(i32)((u32)p[1]|0xFFFFFF00u):(i32)p[1];
                i32 dy=(p[0]&0x20)?(i32)((u32)p[2]|0xFFFFFF00u):(i32)p[2];
                MX+=dx; MY-=dy;
                if(MX<0)MX=0; if(MX>=(i32)SCR_W)MX=SCR_W-1;
                if(MY<0)MY=0; if(MY>=(i32)SCR_H)MY=SCR_H-1;
            }
        }
    }
}

/* Tıklama Alanı Algılayıcı */
static int CLK(i32 x,i32 y,i32 w,i32 h){
    return (MB&1)&&(MX>=x&&MX<x+w&&MY>=y&&MY<y+h);
}

/* ================================================================
   ARAYÜZ BİLEŞENLERİ (UI)
   ================================================================ */
static int BTN(i32 x,i32 y,i32 w,i32 h,const char*s,u32 c){
    int pr=(MX>=x&&MX<x+w&&MY>=y&&MY<y+h&&(MB&1));
    rr(x,y,w,h,6,pr?CDG:c);
    dsc(x,y+(h-8)/2,w,s,pr?CW:CK,pr?CDG:c,1);
    return CLK(x,y,w,h);
}
static int TGL(i32 x,i32 y,int v){
    rr(x,y,40,20,10,v?CBL:CBR);
    circ(v?(x+30):(x+10),y+10,8,CW);
    return CLK(x,y,40,20);
}
static int CHK(i32 x,i32 y,int v){
    fr(x,y,18,18,CW); rb(x,y,18,18,v?CBL:CBR,1);
    if(v){
        for(i32 i=0;i<4;i++) { pp(x+3+i,y+10+i,CBL); pp(x+4+i,y+10+i,CBL); }
        for(i32 i=0;i<6;i++) { pp(x+7+i,y+13-i,CBL); pp(x+8+i,y+13-i,CBL); }
    }
    return CLK(x,y,18,18);
}
static void PROGRESS(i32 x,i32 y,i32 w,i32 h,int pct){
    rr(x,y,w,h,h/2,CLG);
    if(pct>0){
        if(pct>100)pct=100;
        rr(x,y,w*pct/100,h,h/2,CBL);
    }
}

/* ================================================================
   OOBE (SİSTEM KURULUM) EKRANLARI DEĞİŞKENLERİ
   ================================================================ */
static char PC_NAME[32]="";
static u32  PC_NL=0;
static int  SEL_REG=0;   /* 0:Turkiye, 1:USA, 2:UK */
static int  SEL_KBD=0;   /* 0:TR-Q, 1:TR-F, 2:US-Q */
static int  PRIV_LOC=1, PRIV_DIAG=1;
static int  CAT_ENT=0, CAT_GAM=0, CAT_SCH=0, CAT_FAM=0;
static u32  SETUP_TICK=0;

static void draw_setup_bg(const char*title){
    grad(CW,CLL);
    ds(50,50,title,CK,CW,3);
    fr(0,SCR_H-50,SCR_W,50,CHD);
}

/* S1: Bilgisayar İsmi */
static void S1(u8 k){
    draw_setup_bg("Bilgisayariniza bir ad verin");
    ds(50,150,"Bu ad agda diger cihazlar tarafindan gorunur.",CGY,CW,1);
    
    fr(50,220,300,35,CW); rb(50,220,300,35,CBL,2);
    if(PC_NL==0) ds(60,230,"Orn: Sky-Core-PC",CMG,CW,1);
    else ds(60,230,PC_NAME,CK,CW,1);

    if(k==8 && PC_NL>0){ PC_NAME[--PC_NL]=0; }
    else if(k>=' ' && k<127 && PC_NL<18){ PC_NAME[PC_NL++]=k; PC_NAME[PC_NL]=0; }

    if(BTN(SCR_W-150,SCR_H-40,120,32,"Ileri",CBL)) g_state=STATE_SETUP_2_REGION;
}

/* S2: Bölge */
static void S2(void){
    draw_setup_bg("Bolge secimi");
    if(BTN(50,150,200,40,"Turkiye",SEL_REG==0?CLL:CW)){SEL_REG=0;}
    if(BTN(50,200,200,40,"United States",SEL_REG==1?CLL:CW)){SEL_REG=1;}
    if(BTN(50,250,200,40,"United Kingdom",SEL_REG==2?CLL:CW)){SEL_REG=2;}
    if(BTN(SCR_W-150,SCR_H-40,120,32,"Ileri",CBL)) g_state=STATE_SETUP_3_KEYBOARD;
}

/* S3: Klavye */
static void S3(void){
    draw_setup_bg("Klavye duzeni");
    if(BTN(50,150,200,40,"Turkce Q",SEL_KBD==0?CLL:CW)){SEL_KBD=0;}
    if(BTN(50,200,200,40,"Turkce F",SEL_KBD==1?CLL:CW)){SEL_KBD=1;}
    if(BTN(50,250,200,40,"US English",SEL_KBD==2?CLL:CW)){SEL_KBD=2;}

    fr(300,150,400,150,CTH); rb(300,150,400,150,CBR,2);
    dsc(300,160,400,"[ Klavye Onizleme ]",CW,CTH,1);
    fr(320,200,360,25,CHD); rb(320,200,360,25,CK,1);
    if(SEL_KBD==0) dsc(320,208,360,"Q  W  E  R  T  Y  U  I  O  P",CK,CHD,1);
    if(SEL_KBD==1) dsc(320,208,360,"F  G  G  I  O  D  R  N  H  P",CK,CHD,1);
    if(SEL_KBD==2) dsc(320,208,360,"Q  W  E  R  T  Y  U  I  O  P",CK,CHD,1);

    if(BTN(SCR_W-150,SCR_H-40,120,32,"Ileri",CBL)) g_state=STATE_SETUP_4_NETWORK;
}

/* S4: Network */
static void S4(void){
    draw_setup_bg("Aga baglanalim");
    fr(50,150,400,80,CW); rb(50,150,400,80,CLG,1);
    warc(90,195,25,3,CBL); warc(90,195,15,3,CBL); circ(90,195,3,CBL);
    ds(140,170,"Sky-Core-WiFi",CK,CW,1.5);
    ds(140,195,"Baglandi, Guvenli",CGY,CW,1);

    if(BTN(SCR_W-150,SCR_H-40,120,32,"Ileri",CBL)) g_state=STATE_SETUP_5_PRIVACY;
}

/* S5: Gizlilik */
static void S5(void){
    draw_setup_bg("Cihaziniz icin gizlilik ayarlari");
    ds(50,140,"Konum servislerini ac",CK,CW,1);
    if(TGL(50,160,PRIV_LOC)) PRIV_LOC=!PRIV_LOC;
    ds(50,210,"Hata teshis verilerini gonder",CK,CW,1);
    if(TGL(50,230,PRIV_DIAG)) PRIV_DIAG=!PRIV_DIAG;

    if(BTN(SCR_W-150,SCR_H-40,120,32,"Kabul Et",CBL)) g_state=STATE_SETUP_6_CUSTOMIZE;
}

/* S6: Özelleştirme */
static void S6(void){
    draw_setup_bg("Deneyiminizi ozellestirin");
    ds(50,130,"Kullanim amaclarinizi secin (Opsiyonel):",CGY,CW,1);

    if(CHK(50,170,CAT_ENT)) CAT_ENT=!CAT_ENT; ds(80,173,"Eglence ve Medya",CK,CW,1);
    if(CHK(50,210,CAT_GAM)) CAT_GAM=!CAT_GAM; ds(80,213,"Oyunlar",CK,CW,1);
    if(CHK(50,250,CAT_SCH)) CAT_SCH=!CAT_SCH; ds(80,253,"Okul ve Odevler",CK,CW,1);
    if(CHK(50,290,CAT_FAM)) CAT_FAM=!CAT_FAM; ds(80,293,"Aile ve Paylasim",CK,CW,1);

    if(BTN(SCR_W-150,SCR_H-40,120,32,"Atla",CBR)) g_state=STATE_SETUP_7_WELCOME;
    if(BTN(SCR_W-290,SCR_H-40,120,32,"Kabul Et",CBL)) g_state=STATE_SETUP_7_WELCOME;
}

/* S7: Hoş Geldiniz Animasyon Girişi */
static void S7(void){
    grad(CBL,CDG);
    SETUP_TICK++;
    if(SETUP_TICK<120){
        dsc(0,300,SCR_W,"Merhaba",CW,CBL,3);
    }else if(SETUP_TICK<250){
        dsc(0,300,SCR_W,"Her sey sizin icin hazirlaniyor...",CW,CBL,2);
    }else if(SETUP_TICK<400){
        dsc(0,300,SCR_W,"Sistem yukleniyor",CW,CBL,2);
        PROGRESS(SCR_W/2-150,360,300,10,(int)((SETUP_TICK-250)*100/150));
    }else{
        g_state=STATE_DESKTOP;
    }
}

/* ================================================================
   PCI / USB TARAYICI (HAM EMÜLASYON)
   ================================================================ */
static int USB_STATUS=0; 
static u32 USB_TICK=0;
static void pci_scan(void){
    for(u32 bus=0;bus<2;bus++)
        for(u32 dev=0;dev<32;dev++){
            u32 conf=(0x80000000u)|(bus<<16)|(dev<<11);
            outl(0xCF8,conf);
            u32 id=inl(0xCFC);
            if(id!=0xFFFFFFFFu&&id!=0){
                outl(0xCF8,conf|(0x08));
                u32 cl_id=inl(0xCFC)>>24;
                if(cl_id==0x0C) { USB_STATUS=1; return; }
            }
        }
    USB_STATUS=1; 
}

/* ================================================================
   MASAÜSTÜ VE PENCERE YÖNETİCİSİ (S masaüstü ekranı)
   ================================================================ */
static int FM_OPEN=0;
static i32 FM_X=150, FM_Y=100, FM_W=500, FM_H=350;
static int FM_DRAG=0, FM_DX=0, FM_DY=0;
static int MENU_OPEN=0;
static int APP_INSTALLED=0;

static void SD(void){
    /* 1. Masaüstü Arkaplanı */
    grad(0xFF004E92u,0xFF000428u);

    /* 2. Masaüstü Dosyalar Simgesi */
    rr(40,40,50,50,8,CBL);
    fr(50,48,30,8,CW); fr(50,56,30,24,CLL);
    dsc(20,95,90,"Dosyalar",CW,0,1);
    if(CLK(40,40,50,50)) FM_OPEN=1;

    /* 3. Dosya Yöneticisi Penceresi */
    if(FM_OPEN){
        if(MB&1){
            if(!FM_DRAG && MX>=FM_X && MX<FM_X+FM_W && MY>=FM_Y && MY<FM_Y+30){
                FM_DRAG=1; FM_DX=MX-FM_X; FM_DY=MY-FM_Y;
            }
            if(FM_DRAG){ FM_X=MX-FM_DX; FM_Y=MY-FM_DY; }
        }else{ FM_DRAG=0; }

        rr(FM_X,FM_Y,FM_W,FM_H,8,CW); rb(FM_X,FM_Y,FM_W,FM_H,CLG,2);
        fr(FM_X+2,FM_Y+2,FM_W-4,28,CHD);
        ds(FM_X+15,FM_Y+10,"Dosya Yoneticisi",CK,CHD,1);
        
        if(BTN(FM_X+FM_W-35,FM_Y+4,26,22,"X",CRD)) FM_OPEN=0;

        fr(FM_X+2,FM_Y+30,130,FM_H-62,CHD);
        ds(FM_X+15,FM_Y+50,"Bu Bilgisayar",CK,CHD,1);
        ds(FM_X+15,FM_Y+80,USB_STATUS?"* USB Surucu":"- USB Yok",USB_STATUS?CGN:CGY,CHD,1);

        if(USB_STATUS){
            rr(FM_X+160,FM_Y+50,60,50,4,COR);
            dsc(FM_X+160,FM_Y+70,60,"EXE",CW,COR,1);
            dsc(FM_X+150,FM_Y+110,80,"Setup.exe",CK,CW,1);
            if(CLK(FM_X+160,FM_Y+50,60,50)){ APP_INSTALLED=1; }
        } else {
            ds(FM_X+180,FM_Y+100,"Lutfen depolama takin.",CGY,CW,1);
        }

        fr(FM_X+2,FM_Y+FM_H-32,FM_W-4,30,CHD);
        ds(FM_X+15,FM_Y+FM_H-22,USB_STATUS?"Durum: Hazir.":"Durum: Bekleniyor.",CK,CHD,1);
    }

    /* 4. Alt Görev Çubuğu */
    fr(0,SCR_H-40,SCR_W,40,CTB);
    if(BTN(5,SCR_H-35,80,30,"Baslat",MENU_OPEN?CDG:CBL)) MENU_OPEN=!MENU_OPEN;
    ds(SCR_W-80,SCR_H-25,"22:00",CW,CTB,1);

    /* 5. Başlat Menüsü Paneli */
    if(MENU_OPEN){
        rr(5,SCR_H-300,220,255,8,CND); rb(5,SCR_H-300,220,255,CBR,1);
        fr(10,SCR_H-290,210,35,CTH);
        ds(20,SCR_H-280,PC_NAME[0]?PC_NAME:"Kullanici",CW,CTH,1);
        
        if(BTN(15,SCR_H-240,200,32,"Dosya Yoneticisi",CTH)){ FM_OPEN=1; MENU_OPEN=0; }
        
        if(APP_INSTALLED){
            if(BTN(15,SCR_H-200,200,32,"[ Uygulama ]",CGN)){ FM_OPEN=1; FM_X=200; FM_Y=150; MENU_OPEN=0; }
        } else {
            ds(25,SCR_H-200,"(Uygulama Yok)",CGY,CND,1);
        }

        if(BTN(15,SCR_H-80,200,32,"Sistemi Kapat",CRD)){ g_state=STATE_SETUP_1_NAME; MENU_OPEN=0; SETUP_TICK=0; }
    }

    /* 6. Fare İmleci Çizimi (En üst katmanda) */
    circ(MX,MY,5,CW); circ(MX,MY,2,CK);

    if(++USB_TICK>3000){ pci_scan(); USB_TICK=0; }
}

/* ================================================================
   GECİKME
   ================================================================ */
static void delay(int n){ volatile int x=n*6000; while(x--)__asm__("nop"); }

/* ================================================================
   KERNEL MAIN (ANA GİRİŞ)
   ================================================================ */
void kernel_main(multiboot_info_t *mbi){
    /* Video Belleği Adres Ataması */
    FB    = (u32*)(unsigned long)mbi->framebuffer_addr;
    SCR_W = mbi->framebuffer_width;
    SCR_H = mbi->framebuffer_height;
    SCR_P = mbi->framebuffer_pitch / 4;

    if(!FB||SCR_W==0){
        FB=(u32*)0xFD000000u; SCR_W=1024; SCR_H=768; SCR_P=1024;
    }

    /* Donanımları Başlat */
    mouse_init();
    pci_scan();

    while(1){
        /* Giriş aygıtlarını güncelle */
        mouse_poll();
        u8 key=kbd_poll();

        /* Durum Makinesi Çizimleri (Arka Tampona Çizilir) */
        switch(g_state){
            case STATE_SETUP_1_NAME:      S1(key); break;
            case STATE_SETUP_2_REGION:    S2();    break;
            case STATE_SETUP_3_KEYBOARD:  S3();    break;
            case STATE_SETUP_4_NETWORK:   S4();    break;
            case STATE_SETUP_5_PRIVACY:   S5();    break;
            case STATE_SETUP_6_CUSTOMIZE: S6();    break;
            case STATE_SETUP_7_WELCOME:   S7();    break;
            case STATE_DESKTOP:           SD();    break;
        }

        /* Çizimi tek hamlede canlı ekrana aktar ve titremeyi önle */
        swap_buffers();
    }
}
