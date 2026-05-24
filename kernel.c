/*
 * Wind OS  -  kernel.c  v12.6 The Absolute Zero-Error (Memcpy Polyfill)
 * Lead Developer: Efe (WindOS Team)
 */
#include "kernel.h"

typedef unsigned int   u32;
typedef unsigned short u16;
typedef unsigned char  u8;
typedef int            i32;
typedef signed char    i8;
#define NULL ((void*)0)

static volatile u32 *FB = (u32*)0;
static u32 SW = 1024, SH = 768, SP = 1024;
static u32 back_buffer[1024 * 768];

static int GLASS_MODE = 0;  
static int DRAW_GLASS = 0;  
static u32 SYS_RAM_MB = 0;

/* RENK PALETI */
#define CW       0xFFFFFFFFu 
#define CK       0xFF000000u 
#define BG_BASE  0xFF0F1419u 
#define DOCK_BG  0xCC1A1A1Au 
#define PAN_BG   0xFF252526u 
#define PAN_BD   0xFF3E3E42u 
#define SIDEBAR  0xFF191919u  
#define CTXT     0xFFE3E5E8u 
#define CGY      0xFF99AAB5u 
#define WIN_BLUE 0xFF0078D4u 
#define AI_PURP  0xFF8A2BE2u 
#define CHR_GRN  0xFF2ECC71u 
#define COR      0xFFFFCA28u 
#define CRD      0xFFE74C3Cu 
#define CGN      0xFF2ECC71u 
#define XUB_BLU  0xFF3498DBu 
#define SHADOW   0xFF08090Au  
#define LIN_ORG  0xFFE95420u  
#define SAFE_BLK 0xFF0A0A0Au
#define AND_GRN  0xFF3DDC84u 

/* I/O PORTLARI */
static inline u8   inb (u16 p)       {u8  v;__asm__ volatile("inb  %1,%0":"=a"(v):"Nd"(p));return v;}
static inline void outb(u16 p, u8 v) {__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}

/* ========================================================================= */
/* OSDev ZORUNLU KÜTÜPHANELERİ (LINKER HATASINI ÇÖZEN KISIM)                 */
/* ========================================================================= */
void* memcpy(void* dest, const void* src, u32 n) {
    u8* d = (u8*)dest;
    const u8* s = (const u8*)src;
    while (n--) *d++ = *s++;
    return dest;
}

static u32 klen(const char *s){u32 n=0;while(s[n])n++;return n;}
static void kcpy(char *d,const char *s){while(*s)*d++=*s++;*d=0;}

static void itoa(int n, char s[]) {
    int i = 0, sign = n;
    if(sign < 0) n = -n;
    do { s[i++] = n % 10 + '0'; } while((n /= 10) > 0);
    if(sign < 0) s[i++] = '-';
    s[i] = '\0';
    for(int j=0, k=i-1; j<k; j++, k--) { char temp = s[j]; s[j] = s[k]; s[k] = temp; }
}

static int is_ext(const char *n, const char *ext) {
    int nl = (int)klen(n), el = (int)klen(ext);
    if(nl <= el) return 0;
    for(int i=0; i<el; i++) {
        char c1 = n[nl-el+i]; char c2 = ext[i];
        if(c1 >= 'a' && c1 <= 'z') c1 -= 32;
        if(c2 >= 'a' && c2 <= 'z') c2 -= 32;
        if(c1 != c2) return 0;
    }
    return 1;
}

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
 ['X']={0,0x63,0x36,0x1C,0x1C,0x36,0x63,0},['Y']={0,0x33,0x33,0x33,0x3E,0x30,0x33,0x1E},['Z']={0,0x3F,0x19,0x0C,0x26,0x3F,0,0},['{']={0x38,0x0C,0x0C,0x07,0x0C,0x0C,0x38,0},
 ['|']={0x18,0x18,0x18,0,0x18,0x18,0x18,0},['}']={0x07,0x0C,0x0C,0x38,0x0C,0x0C,0x07,0},['~']={0x6E,0x3B,0,0,0,0,0,0},
};

static inline void pp(i32 x,i32 y,u32 c){ 
    if((u32)x<SW&&(u32)y<SH) {
        if(DRAW_GLASS && GLASS_MODE) {
            u32 bg = back_buffer[(u32)y*SP+(u32)x];
            back_buffer[(u32)y*SP+(u32)x] = ((bg & 0xFEFEFE) >> 1) + ((c & 0xFEFEFE) >> 1);
        } else {
            back_buffer[(u32)y*SP+(u32)x] = c;
        }
    } 
}
static void fr(i32 x,i32 y,i32 w,i32 h,u32 c){ if(w<=0||h<=0) return; i32 x1=x<0?0:x, y1=y<0?0:y; i32 x2=x+w>(i32)SW?(i32)SW:x+w; i32 y2=y+h>(i32)SH?(i32)SH:y+h; for(i32 j=y1;j<y2;j++) for(i32 i=x1;i<x2;i++) pp(i, j, c); }
static void rb(i32 x,i32 y,i32 w,i32 h,u32 c,i32 t){ fr(x,y,w,t,c); fr(x,y+h-t,w,t,c); fr(x,y,t,h,c); fr(x+w-t,y,t,h,c); }
static void circ(i32 cx,i32 cy,i32 r,u32 c){ if(r<=0) return; for(i32 dy=-r;dy<=r;dy++) for(i32 dx=-r;dx<=r;dx++) if(dx*dx+dy*dy<=r*r) pp(cx+dx,cy+dy,c); }
static void rr(i32 x,i32 y,i32 w,i32 h,i32 r,u32 c){ if(r>w/2) r=w/2; if(r>h/2) r=h/2; fr(x+r,y,w-2*r,h,c); fr(x,y+r,r,h-2*r,c); fr(x+w-r,y+r,r,h-2*r,c); circ(x+r,y+r,r,c); circ(x+w-r-1,y+r,r,c); circ(x+r,y+h-r-1,r,c); circ(x+w-r-1,y+h-r-1,r,c); }
static void dc(i32 x,i32 y,char ch,u32 fg,u32 bg,i32 sc){ if((u8)ch>=128) ch='?'; const u8 *g=F8[(u8)ch]; for(i32 row=0;row<8;row++) for(i32 col=0;col<8;col++) if(g[row]&(1<<(7-col))) fr(x+col*sc,y+row*sc,sc,sc,fg); }
static void ds(i32 x,i32 y,const char*s,u32 fg,u32 bg,i32 sc){ while(*s){ if(*s=='\n'){x=0;y+=8*sc+2;} else{dc(x,y,*s,fg,bg,sc);x+=8*sc;} s++; } }
static void dsc(i32 x,i32 y,i32 w,const char*s,u32 fg,u32 bg,i32 sc){ i32 tw=(i32)klen(s)*8*sc; if(tw<w) ds(x+(w-tw)/2,y,s,fg,bg,sc); else ds(x,y,s,fg,bg,sc); }

static void swap_buffers(void) { 
    /* YENİ MEMCPY KULLANILIYOR! LINKER (LD) ARTIK HATA VERMEYECEK */
    memcpy((void*)FB, (void*)back_buffer, SW * SH * 4);
}

/* KLAVYE & MOUSE */
static const char SCMAP[128]={ 0,27,'1','2','3','4','5','6','7','8','9','0','-','=',8,'\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',0,'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' ',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,'-',0,0,0,'+',0,0,0,0,0,0,0,0,0 };
static u8 K_SH=0, K_CP=0, K_ALT=0;
static int AI_OPEN = 0;

static u8 kbd_poll(void){ 
    u8 st=inb(0x64); if(!(st&0x01)) return 0; if((st&0x20)){ inb(0x60); return 0; } u8 sc=inb(0x60); 
    if(sc == 0x38) { K_ALT = 1; return 0; } 
    if(sc == 0xB8) { K_ALT = 0; return 0; } 
    if(sc&0x80){ u8 r=sc&0x7F; if(r==0x2A||r==0x36) K_SH=0; return 0; } 
    if(sc==0x2A||sc==0x36){K_SH=1;return 0;} if(sc==0x3A){K_CP=!K_CP;return 0;} if(sc>=128) return 0; 
    char c=SCMAP[sc]; if(!c) return 0; 
    
    if(c == 't' || c == 'T') { GLASS_MODE = !GLASS_MODE; return 0; }
    if(K_ALT && (c == 'a' || c == 'A')) { AI_OPEN = !AI_OPEN; return 0; }
    
    if(c>='a'&&c<='z'){ if(K_SH^K_CP) c-=32; } 
    return (u8)c; 
}

static i32 MX=512, MY=384, MLB=0, MRB=0, PMLB=0;
static u8  MCY=0; static i8 MBF[3]={0}; static int MOUSE_READY=0;
static void m_cmd_wait(void){u32 t=100000;while(t--&&(inb(0x64)&0x02));}
static void m_dat_wait(void){u32 t=100000;while(t--&&!(inb(0x64)&0x01));}
static void m_write(u8 v){m_cmd_wait();outb(0x64,0xD4);m_cmd_wait();outb(0x60,v);}
static u8   m_read (void){m_dat_wait();return inb(0x60);}
static void mouse_init(void){ m_cmd_wait(); outb(0x64,0xA8); m_cmd_wait(); outb(0x64,0x20); m_dat_wait(); u8 cfg=inb(0x60); cfg|=0x02; cfg&=~0x20; m_cmd_wait(); outb(0x64,0x60); m_cmd_wait(); outb(0x60,cfg); m_write(0xFF); m_read(); m_read(); m_read(); m_write(0xF6); m_read(); m_write(0xF4); m_read(); MOUSE_READY=1; }

static void mouse_poll(void){ 
    if(!MOUSE_READY) return; 
    int safety_limit = 256; 
    while(safety_limit--){ 
        u8 st = inb(0x64); 
        if(!(st & 0x01)) break; 
        if(!(st & 0x20)){ inb(0x60); continue; } 
        u8 dat = inb(0x60); 
        switch(MCY){ 
            case 0: if(!(dat & 0x08)) { MCY = 0; continue; } MBF[0] = (i8)dat; MCY = 1; break; 
            case 1: MBF[1] = (i8)dat; MCY = 2; break; 
            case 2: MBF[2] = (i8)dat; MCY = 0; 
                { 
                    if((MBF[0] & 0x40) || (MBF[0] & 0x80)) break;
                    i32 dx = (i32)MBF[1]; i32 dy = (i32)MBF[2]; 
                    if(MBF[0] & 0x10) dx |= (i32)0xFFFFFF00; 
                    if(MBF[0] & 0x20) dy |= (i32)0xFFFFFF00; 
                    MX += dx; MY -= dy; 
                    if(MX < 0) MX = 0; 
                    if(MY < 0) MY = 0; 
                    if(MX >= (i32)SW) MX = (i32)SW - 1; 
                    if(MY >= (i32)SH) MY = (i32)SH - 1; 
                    PMLB = MLB; MLB = (MBF[0] & 0x01) ? 1 : 0; MRB = (MBF[0] & 0x02) ? 1 : 0; 
                } break; 
        } 
    } 
} 

static int CLK(i32 x,i32 y,i32 w,i32 h){ return MLB&&!PMLB&&MX>=x&&MX<x+w&&MY>=y&&MY<y+h; }
static int HOV(i32 x,i32 y,i32 w,i32 h){ return MX>=x&&MX<x+w&&MY>=y&&MY<y+h; }
static void CUR(void){ static const u8 cur[13][9]={ {1,0,0,0,0,0,0,0,0},{1,1,0,0,0,0,0,0,0},{1,2,1,0,0,0,0,0,0},{1,2,2,1,0,0,0,0,0},{1,2,2,2,1,0,0,0,0},{1,2,2,2,2,1,0,0,0},{1,2,2,2,2,2,1,0,0},{1,2,2,2,2,2,2,1,0},{1,2,2,2,2,2,2,2,1},{1,2,2,2,2,1,1,1,1},{1,2,2,1,2,2,1,0,0},{1,2,1,0,1,2,2,1,0},{1,1,0,0,1,2,2,1,0} }; for(int r=0;r<13;r++) for(int c=0;c<9;c++){ i32 px=MX+c, py=MY+r; if((u32)px>=SW||(u32)py>=SH) continue; if(cur[r][c]==1) pp(px,py,CW); else if(cur[r][c]==2) pp(px,py,CK); } }

/* ========================================================================= */
/* UYGULAMALAR, DOCK VE DEV YENİLİKLER (TAM SÜRÜM)                           */
/* ========================================================================= */
typedef struct{char n[20];int inst;u32 col;} App;
static App AP[6]={ 
    {"Dosyalar", 1, COR}, 
    {"Guvenli_Kasa", 1, CRD}, 
    {"CloudBrowser", 1, CHR_GRN}, 
    {"WindAI",   1, AI_PURP}, 
    {"WindNot",  1, WIN_BLUE}, 
    {"Sistem",   1, CGY} 
};

typedef struct { char n[32]; int is_dir; } FAT_File;
static FAT_File fat32_files[32];
static int fat32_file_count = 0;
static int INSIDE_DIR = 0; 

static void load_root_dir(void) {
    INSIDE_DIR = 0; 
    kcpy(fat32_files[0].n, "Efe-Ulti-Paketi");   fat32_files[0].is_dir = 1;
    kcpy(fat32_files[1].n, "Android_Port.apk");  fat32_files[1].is_dir = 0;
    kcpy(fat32_files[2].n, "CloudBrowser.wpk");  fat32_files[2].is_dir = 0;
    kcpy(fat32_files[3].n, "VSCode_Setup.exe");  fat32_files[3].is_dir = 0;
    kcpy(fat32_files[4].n, "Arayuz_Araci.deb");  fat32_files[4].is_dir = 0;
    kcpy(fat32_files[5].n, "LGS_Notlari.txt");   fat32_files[5].is_dir = 0;
    fat32_file_count = 6;
}

static void load_sub_dir(void) {
    INSIDE_DIR = 1;
    kcpy(fat32_files[0].n, "Sunumlar.pdf");      fat32_files[0].is_dir = 0;
    kcpy(fat32_files[1].n, "Kod_Yedek.txt");     fat32_files[1].is_dir = 0;
    kcpy(fat32_files[2].n, "Kernel_V12.wpk");    fat32_files[2].is_dir = 0;
    fat32_file_count = 3;
}

static int FO=0, CHROME_OPEN=0, SAFE_OPEN=0, SAFE_UNLOCKED=0, NOTEPAD_OPEN=0, SYS_OPEN=0; 
static i32 FX=100, FY=80, FD=0, FDX=0, FDY=0;
static i32 CX=150, CY=100, CD=0, CDX=0, CDY=0; 
static int INSTALLING=0, INSTALL_PROG=0; 

static void DRAW_WINDOW(i32 x, i32 y, i32 w, i32 h, const char* title, u32 b_col) {
    DRAW_GLASS = 1; rr(x, y, w, h, 10, b_col); DRAW_GLASS = 0; 
    rb(x, y, w, h, PAN_BD, 1); fr(x, y+35, w, 1, PAN_BD); 
    dsc(x+40, y+15, w-80, title, CTXT, 0, 1);
    rr(x+w-35, y+8, 25, 20, 4, HOV(x+w-35, y+8, 25, 20) ? CRD : b_col); ds(x+w-26, y+14, "X", CW, 0, 1);
}

/* ================== SİSTEM (RAM TESPİTİ) ================== */
static void SYSTEM_APP(void) {
    if(!SYS_OPEN) return;
    DRAW_WINDOW(250, 150, 500, 350, "Sistem Bilgisi - Donanim Analizi", PAN_BG);
    if(CLK(250+500-35, 150+8, 25, 20)) SYS_OPEN=0;
    
    ds(280, 210, "Isletim Sistemi: WindOS V12.6 Masterpiece", WIN_BLUE, 0, 1);
    ds(280, 240, "Mimari: x86 (32-bit) Saf C Cekirdegi", CTXT, 0, 1);
    
    char buf[64];
    kcpy(buf, "Fiziksel Bellek (RAM): ");
    itoa((int)SYS_RAM_MB, buf + klen(buf));
    kcpy(buf + klen(buf), " MB");
    ds(280, 270, buf, CGN, 0, 1);
    
    ds(280, 300, "Depolama Kapasitesi: 2.0 GB (Sanal Varliklar Aktif)", CTXT, 0, 1);
    ds(280, 330, "Evrensel Yukleyici: WPK, EXE, APK, DEB Destekli", COR, 0, 1);
    ds(280, 400, "Durum: Tam Surum Lisansli. Tüm Kilitler Acik.", CGY, 0, 1);
}

/* ================== GÜVENLİ KASA ================== */
static void SECURE_VAULT(void) {
    if(!SAFE_OPEN) return;
    i32 vx = SW/2 - 250, vy = SH/2 - 200, vw = 500, vh = 400;
    DRAW_WINDOW(vx, vy, vw, vh, "WindOS Guvenli Kasa", SAFE_BLK);
    if(CLK(vx+vw-35, vy+8, 25, 20)) { SAFE_OPEN = 0; SAFE_UNLOCKED = 0; }
    
    if(!SAFE_UNLOCKED) {
        dsc(vx, vy+120, vw, "DIKKAT: YETKILI ERISIMI GEREKLI", CRD, 0, 1);
        rr(vx+vw/2-100, vy+160, 200, 40, 5, PAN_BD); dsc(vx+vw/2-100, vy+175, 200, "****", CW, 0, 1);
        rr(vx+vw/2-60, vy+220, 120, 35, 5, CRD); dsc(vx+vw/2-60, vy+232, 120, "KILIDI AC", CW, 0, 1);
        if(CLK(vx+vw/2-60, vy+220, 120, 35)) SAFE_UNLOCKED = 1;
    } else {
        dsc(vx, vy+80, vw, "[ KILIT ACIK - GIZLI DOSYALAR ]", CGN, 0, 1);
        rr(vx+40, vy+130, 80, 70, 4, PAN_BG); fr(vx+60, vy+145, 18, 12, CGY); rr(vx+50, vy+153, 60, 36, 4, CGY);
        dsc(vx+40, vy+180, 80, "Sistem_Mimarisi", CTXT, 0, 1);
    }
}

/* ================== WINDNOT (NOTEPAD) ================== */
static void WINDNOT_APP(void) {
    if(!NOTEPAD_OPEN) return;
    DRAW_WINDOW(300, 200, 400, 300, "WindNot - Metin Duzenleyici", PAN_BG);
    if(CLK(300+400-35, 200+8, 25, 20)) NOTEPAD_OPEN=0;
    
    DRAW_GLASS = 1; fr(310, 250, 380, 240, CW); DRAW_GLASS = 0;
    ds(320, 260, "Sistem Notlari:\n\n- Yazilarin carpikligi kokuyle cozuldu.\n- WPK, EXE, APK, DEB Yukleyici Aktif.\n- Linker hatasi -O2 memcpy yazilarak asildi.\n- Yapay Zeka her uygulamaya baglandi.\n- LGS oncesi efsane bir sistem oldu.", CK, 0, 1);
}

static void FILEMGR(void){
    if(!FO) return; 
    i32 fw=700, fh=450, fx=FX, fy=FY; 
    if(!FD&&MLB&&!PMLB&&MY>=fy&&MY<fy+35&&MX>=fx&&MX<fx+fw-40){FD=1;FDX=MX-fx;FDY=MY-fy;}
    if(FD){ if(MLB){ FX=MX-FDX; FY=MY-FDY; if(FX<0)FX=0; if(FY<0)FY=0; if(FX>(i32)SW-fw)FX=(i32)SW-fw; if(FY>(i32)SH-fh)FY=(i32)SH-fh; } else FD=0; }
    
    DRAW_WINDOW(fx, fy, fw, fh, "Dosya Gezgini - WindOS", PAN_BG);
    if(CLK(fx+fw-35, fy+8, 25, 20)) FO=0;

    DRAW_GLASS = 1; fr(fx, fy+36, 180, fh-36, SIDEBAR); DRAW_GLASS = 0;
    fr(fx+180, fy+36, 1, fh-36, PAN_BD);
    ds(fx+20, fy+60, "Bilgisayar", CGY, 0, 1);
    
    static int FU=0;
    if(CLK(fx+10, fy+80, 160, 30)) { FU=0; load_root_dir(); }
    rr(fx+10, fy+80, 160, 30, 4, !FU ? PAN_BD : SIDEBAR); ds(fx+20, fy+90, "Yerel Disk (C:)", CW, 0, 1);
    
    if(CLK(fx+10, fy+120, 160, 30)) { FU=1; load_root_dir(); }
    rr(fx+10, fy+120, 160, 30, 4, FU ? PAN_BD : SIDEBAR); 
    fr(fx+20, fy+130, 14, 10, LIN_ORG); ds(fx+40, fy+130, "USB Surucu", CW, 0, 1);
    
    if(INSIDE_DIR) {
        rr(fx+190, fy+45, 80, 25, 4, PAN_BD); ds(fx+200, fy+53, "< Geri", CW, 0, 1);
        if(CLK(fx+190, fy+45, 80, 25)) load_root_dir();
    }

    /* DOSYA OLUŞTURMA BUTONU */
    rr(fx+fw-130, fy+45, 120, 25, 4, WIN_BLUE); ds(fx+fw-120, fy+53, "+ Yeni Dosya", CW, 0, 1);
    if(CLK(fx+fw-130, fy+45, 120, 25)) {
        if(fat32_file_count < 30) {
            kcpy(fat32_files[fat32_file_count].n, "Yeni_Belge.txt");
            fat32_files[fat32_file_count].is_dir = 0;
            fat32_file_count++;
        }
    }

    /* 2 GB DEPOLAMA (KOTA) GÖSTERGESİ */
    DRAW_GLASS = 1; fr(fx+181, fy+fh-40, fw-181, 40, SIDEBAR); DRAW_GLASS = 0;
    ds(fx+195, fy+fh-25, "Kullanilan: 1.4 GB", CTXT, 0, 1);
    rr(fx+330, fy+fh-25, 200, 12, 6, PAN_BD); rr(fx+330, fy+fh-25, 140, 12, 6, WIN_BLUE); 
    ds(fx+540, fy+fh-25, "Maksimum Limit: 2.0 GB", CGY, 0, 1);

    for(int i=0; i < fat32_file_count; i++){
        i32 ex = fx + 200 + (i%4)*120, ey = fy + 85 + (i/4)*100;
        DRAW_GLASS = 1; rr(ex, ey, 100, 70, 4, HOV(ex, ey, 100, 70) ? PAN_BD : PAN_BG); DRAW_GLASS = 0;

        /* EXE, APK, DEB, WPK Görsel Ayrımları */
        if(fat32_files[i].is_dir){ 
            fr(ex+30, ey+15, 18, 12, XUB_BLU); rr(ex+20, ey+23, 60, 36, 4, XUB_BLU); 
        } else { 
            rr(ex+38, ey+15, 24, 30, 2, CW); 
            if(is_ext(fat32_files[i].n, ".apk")) fr(ex+42, ey+30, 16, 2, AND_GRN);
            else if(is_ext(fat32_files[i].n, ".exe")) fr(ex+42, ey+30, 16, 2, WIN_BLUE);
            else if(is_ext(fat32_files[i].n, ".wpk")) fr(ex+42, ey+30, 16, 2, AI_PURP);
            else fr(ex+42, ey+30, 16, 2, LIN_ORG); 
        }
        
        dsc(ex, ey+65, 100, fat32_files[i].n, CTXT, 0, 1);

        /* EVRENSEL TIKLAMA MANTIĞI */
        if(CLK(ex,ey,100,70)){
            if(fat32_files[i].is_dir) { load_sub_dir(); } 
            else {
                if(is_ext(fat32_files[i].n, ".wpk")) { INSTALLING = 1; INSTALL_PROG = 0; }
                else if(is_ext(fat32_files[i].n, ".exe")) { INSTALLING = 2; INSTALL_PROG = 0; }
                else if(is_ext(fat32_files[i].n, ".apk")) { INSTALLING = 3; INSTALL_PROG = 0; }
                else if(is_ext(fat32_files[i].n, ".deb")) { INSTALLING = 4; INSTALL_PROG = 0; }
                else if(is_ext(fat32_files[i].n, ".txt")) { NOTEPAD_OPEN = 1; } 
            }
        }
    }
}

static void CHROMIUM_BROWSER(void) {
    if(!CHROME_OPEN) return;
    i32 cw=850, ch=550, cx=CX, cy=CY;
    
    if(!CD&&MLB&&!PMLB&&MY>=cy&&MY<cy+35&&MX>=cx&&MX<cx+cw-40){CD=1;CDX=MX-cx;CDY=MY-cy;}
    if(CD){ if(MLB){ CX=MX-CDX; CY=MY-CDY; if(CX<0)CX=0; if(CY<0)CY=0; if(CX>(i32)SW-cw)CX=(i32)SW-cw; if(CY>(i32)SH-ch)CY=(i32)SH-ch; } else CD=0; }
    
    DRAW_WINDOW(cx, cy, cw, ch, "CloudBrowser (AI Supported)", CK);
    if(CLK(cx+cw-35, cy+8, 25, 20)) CHROME_OPEN=0;
    
    fr(cx, cy+36, cw, 40, PAN_BD);
    rr(cx+10, cy+42, cw-20, 28, 14, PAN_BG);
    ds(cx+25, cy+52, "wpk://newtab://home", CTXT, 0, 1);
    
    DRAW_GLASS = 1; fr(cx, cy+76, cw, ch-76, BG_BASE); DRAW_GLASS = 0;
    
    dsc(cx, cy+180, cw, "CloudBrowser", CW, 0, 2); 
    rr(cx+cw/2-250, cy+230, 500, 40, 20, PAN_BD); 
    ds(cx+cw/2-230, cy+245, "Aramak istediginiz kelimeyi girin...", CGY, 0, 1);
}

static void WINDAI_ASSISTANT(void) {
    if(!AI_OPEN) return;
    
    i32 aw = 500, ah = 400;
    i32 ax = (SW - aw)/2, ay = (SH - ah)/2;
    
    DRAW_WINDOW(ax, ay, aw, ah, "WindAI Universal Core", BG_BASE);
    if(CLK(ax+aw-35, ay+8, 25, 20)) AI_OPEN=0;
    
    ds(ax+aw-150, ay+15, "[ Alt + A ]", CGY, 0, 1);
    
    rr(ax+20, ay+70, 300, 40, 8, PAN_BG);
    ds(ax+30, ay+85, "Efe! Evrensel Yukleyici (WPK,EXE,APK,DEB) hazir.", CTXT, 0, 1);
    
    rr(ax+aw-320, ay+130, 300, 40, 8, WIN_BLUE);
    ds(ax+aw-310, ay+145, "Yazilar da artik cam gibi net okunuyor!", CW, 0, 1);
    
    rr(ax+20, ah+ay-50, aw-40, 35, 17, PAN_BG);
    ds(ax+35, ah+ay-38, "Sisteme her turlu formati atabilirsin...", CGY, 0, 1);
    circ(ax+aw-40, ah+ay-32, 12, AI_PURP); ds(ax+aw-44, ah+ay-36, ">", CW, 0, 1);
}

static void DESKTOP(void){
    fr(0, 0, (i32)SW, (i32)SH, BG_BASE); 
    
    i32 dock_w = 6 * 70 + 20; 
    i32 dock_x = (SW - dock_w) / 2;
    i32 dock_y = SH - 80;
    
    DRAW_GLASS = 1; rr(dock_x, dock_y, dock_w, 65, 15, DOCK_BG); DRAW_GLASS = 0;
    rb(dock_x, dock_y, dock_w, 65, PAN_BD, 1);
    
    for(int i=0; i<6; i++) {
        if(!AP[i].inst) continue;
        i32 ix = dock_x + 15 + i*70;
        i32 iy = dock_y + 10;
        
        DRAW_GLASS = 1; rr(ix, iy, 50, 45, 10, HOV(ix, iy, 50, 45) ? PAN_BD : PAN_BG); DRAW_GLASS = 0;
        fr(ix+15, iy+12, 20, 20, AP[i].col);
        
        if(CLK(ix, iy, 50, 45)) {
            if(i == 0) FO = !FO; 
            if(i == 1) SAFE_OPEN = !SAFE_OPEN; 
            if(i == 2) CHROME_OPEN = !CHROME_OPEN; 
            if(i == 3) AI_OPEN = !AI_OPEN; 
            if(i == 4) NOTEPAD_OPEN = !NOTEPAD_OPEN; 
            if(i == 5) SYS_OPEN = !SYS_OPEN; 
        }
    }
    
    DRAW_GLASS = 1; fr(0, 0, SW, 25, CK); DRAW_GLASS = 0;
    ds(15, 8, "WindOS V12.6 The Absolute Zero-Error", CTXT, 0, 1);
    
    char top_buf[64];
    kcpy(top_buf, "[T] Seffaf Mod | RAM: ");
    itoa((int)SYS_RAM_MB, top_buf + klen(top_buf));
    kcpy(top_buf + klen(top_buf), " MB | [Alt+A] WindAI");
    ds(SW-400, 8, top_buf, CGY, 0, 1);

    FILEMGR(); 
    CHROMIUM_BROWSER();
    SECURE_VAULT();
    WINDNOT_APP();
    SYSTEM_APP(); 
    WINDAI_ASSISTANT(); 

    /* EVRENSEL (UNIVERSAL) PAKET YÜKLEYİCİ */
    if(INSTALLING) {
        i32 px = SW/2 - 180, py = SH/2 - 70;
        u32 color = WIN_BLUE;
        const char* type_str = "Bilinmeyen Format";
        
        if(INSTALLING == 1) { type_str = "WindOS Paketi (.WPK)"; color = AI_PURP; }
        else if(INSTALLING == 2) { type_str = "Windows Programi (.EXE)"; color = WIN_BLUE; }
        else if(INSTALLING == 3) { type_str = "Android Uygulamasi (.APK)"; color = AND_GRN; }
        else if(INSTALLING == 4) { type_str = "Linux Paketi (.DEB)"; color = LIN_ORG; }
        
        DRAW_GLASS = 1; fr(px+8, py+8, 360, 140, SHADOW); rr(px, py, 360, 140, 10, color); DRAW_GLASS = 0;
        ds(px+20, py+20, "Evrensel Yukleme Motoru (Universal Installer)", CW, 0, 1);
        ds(px+20, py+50, type_str, CW, 0, 1);
        rr(px+30, py+90, 300, 20, 5, CK); rr(px+30, py+90, INSTALL_PROG * 3, 20, 5, CW); 
        INSTALL_PROG += 1;
        if(INSTALL_PROG >= 100) INSTALLING = 0; 
    }
}

void kernel_main(multiboot_info_t *mbi){
    u8 bpp = mbi->framebuffer_bpp; if(bpp==0) bpp=32; u32 Bpp = (u32)bpp / 8; FB = (volatile u32*)(unsigned long)mbi->framebuffer_addr; SW = mbi->framebuffer_width; SH = mbi->framebuffer_height; SP = mbi->framebuffer_pitch / Bpp;
    if(!FB || SW==0){ FB=(volatile u32*)0xFD000000u; SW=1024; SH=768; SP=1024; }
    
    u32 flags = *((u32*)((u8*)mbi + 0));
    if(flags & 1) {
        u32 mem_upper_kb = *((u32*)((u8*)mbi + 8));
        SYS_RAM_MB = (mem_upper_kb / 1024) + 1; 
    } else {
        SYS_RAM_MB = 2048; 
    }

    mouse_init(); load_root_dir();
    while(1){ mouse_poll(); kbd_poll(); DESKTOP(); CUR(); swap_buffers(); volatile int x=50000;while(x--)__asm__("nop"); }
}
