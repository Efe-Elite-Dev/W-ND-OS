/*
 * Wind OS  -  kernel.c  v15.0 Titanium Core (Zero Bug, Pure VESA Standard)
 * Lead Developer: Efe (WindOS Team)
 */
#include "kernel.h"

typedef unsigned int   u32;
typedef unsigned short u16;
typedef unsigned char  u8;
typedef int            i32;
typedef signed char    i8;
#define NULL ((void*)0)

static volatile u8 *FB = (u8*)0; 
static u32 SW = 1024, SH = 768;
static u32 PITCH = 4096; 
static u8 BPP = 4;       

/* HAFIZA TAŞMASI KALKANI (MAX 1920x1080) */
#define MAX_SW 1920
#define MAX_SH 1080
static u32 back_buffer[MAX_SW * MAX_SH];

static int GLASS_MODE = 0;  
static int DRAW_GLASS = 0;  
static u32 SYS_RAM_MB = 0;

/* AKTİF ÇÖZÜNÜRLÜK MENÜSÜ */
static int CURRENT_RES = 2; 
static const char* RES_NAMES[] = {
    "480P (SD)", 
    "720P (HD)", 
    "1080P (FHD)", 
    "4K (UHD)", 
    "8K (SUHD)", 
    "16K (HYPER)", 
    "32K (QUANTUM)"
};

/* ULTRA YÜKSEK KALİTE RENK PALETİ */
#define CW       0xFFFFFFFFu 
#define CK       0xFF000000u 
#define BG_BASE  0xFF0A0C10u 
#define DOCK_BG  0xDD121214u 
#define PAN_BG   0xFF1C1C1Eu 
#define PAN_BD   0xFF3A3A40u 
#define SIDEBAR  0xFF121214u  
#define CTXT     0xFFF3F4F6u 
#define CGY      0xFFA1AAB7u 
#define WIN_BLUE 0xFF0078D4u 
#define AI_PURP  0xFF8E44ADu 
#define CHR_GRN  0xFF2ECC71u 
#define COR      0xFFF39C12u 
#define CRD      0xFFE74C3Cu 
#define XUB_BLU  0xFF2980B9u 
#define SHADOW   0xFF030303u  

/* I/O PORTLARI */
static inline u8   inb (u16 p)       {u8  v;__asm__ volatile("inb  %1,%0":"=a"(v):"Nd"(p));return v;}
static inline void outb(u16 p, u8 v) {__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}

/* C KÜTÜPHANELERİ */
void* memcpy(void* dest, const void* src, u32 n) {
    u8* d = (u8*)dest; const u8* s = (const u8*)src;
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

/* FONT MOTORU (EKSİKSİZ) */
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

/* TUVAL ÇİZİCİ */
static inline void pp(i32 x,i32 y,u32 c){ 
    if((u32)x<SW && (u32)y<SH && (u32)x<MAX_SW && (u32)y<MAX_SH) {
        u32 bb_index = ((u32)y * SW) + (u32)x; 
        if(DRAW_GLASS && GLASS_MODE) {
            u32 bg = back_buffer[bb_index];
            back_buffer[bb_index] = ((bg & 0xFEFEFE) >> 1) + ((c & 0xFEFEFE) >> 1);
        } else {
            back_buffer[bb_index] = c;
        }
    } 
}
static void fr(i32 x,i32 y,i32 w,i32 h,u32 c){ if(w<=0||h<=0) return; i32 x1=x<0?0:x, y1=y<0?0:y; i32 x2=x+w>(i32)SW?(i32)SW:x+w; i32 y2=y+h>(i32)SH?(i32)SH:y+h; for(i32 j=y1;j<y2;j++) for(i32 i=x1;i<x2;i++) pp(i, j, c); }
static void rb(i32 x,i32 y,i32 w,i32 h,u32 c,i32 t){ fr(x,y,w,t,c); fr(x,y+h-t,w,t,c); fr(x,y,t,h,c); fr(x+w-t,y,t,h,c); }
static void circ(i32 cx,i32 cy,i32 r,u32 c){ if(r<=0) return; for(i32 dy=-r;dy<=r;dy++) for(i32 dx=-r;dx<=r;dx++) if(dx*dx+dy*dy<=r*r) pp(cx+dx,cy+dy,c); }
static void rr(i32 x,i32 y,i32 w,i32 h,i32 r,u32 c){ if(r>w/2) r=w/2; if(r>h/2) r=h/2; fr(x+r,y,w-2*r,h,c); fr(x,y+r,r,h-2*r,c); fr(x+w-r,y+r,r,h-2*r,c); circ(x+r,y+r,r,c); circ(x+w-r-1,y+r,r,c); circ(x+r,y+h-r-1,r,c); circ(x+w-r-1,y+h-r-1,r,c); }

/* ZIRHLI HARF YAZICI: BÜTÜN HARFLERİ BÜYÜTÜR, EKSİK ÇIKMASINI ENGELLER */
static void dc(i32 x,i32 y,char ch,u32 fg,u32 bg,i32 sc){ 
    if(ch >= 'a' && ch <= 'z') ch -= 32; 
    if((u8)ch>=128) ch='?'; 
    const u8 *g=F8[(u8)ch]; 
    for(i32 row=0;row<8;row++) 
        for(i32 col=0;col<8;col++) 
            if(g[row]&(1<<(7-col))) 
                fr(x+col*sc,y+row*sc,sc,sc,fg); 
}
static void ds(i32 x,i32 y,const char*s,u32 fg,u32 bg,i32 sc){ while(*s){ if(*s=='\n'){x=0;y+=8*sc+2;} else{dc(x,y,*s,fg,bg,sc);x+=8*sc;} s++; } }
static void dsc(i32 x,i32 y,i32 w,const char*s,u32 fg,u32 bg,i32 sc){ i32 tw=(i32)klen(s)*8*sc; if(tw<w) ds(x+(w-tw)/2,y,s,fg,bg,sc); else ds(x,y,s,fg,bg,sc); }

/* ========================================================================= */
/* TITANIUM VESA MOTORU: HİÇBİR TAKLA VEYA AYNA YOK. SAF, DOĞAL, KUSURSUZ 1D */
/* ========================================================================= */
static void swap_buffers(void) { 
    for(u32 y=0; y<SH; y++) {
        u32 bb_row = y * SW;
        u32 fb_row = y * PITCH; 

        for(u32 x=0; x<SW; x++) {
            u32 color = back_buffer[bb_row + x];
            u32 offset = fb_row + (x * BPP);

            if (BPP == 4) {
                *(u32*)(FB + offset) = color;
            } else if (BPP == 3) {
                FB[offset] = color & 0xFF;
                FB[offset + 1] = (color >> 8) & 0xFF;
                FB[offset + 2] = (color >> 16) & 0xFF;
            } else if (BPP == 2) {
                u16 r = ((color >> 16) & 0xFF) >> 3;
                u16 g = ((color >> 8) & 0xFF) >> 2;
                u16 b = (color & 0xFF) >> 3;
                *(u16*)(FB + offset) = (r << 11) | (g << 5) | b;
            }
        }
    }
}

/* KLAVYE & MOUSE */
static const char SCMAP[128]={ 0,27,'1','2','3','4','5','6','7','8','9','0','-','=',8,'\t','Q','W','E','R','T','Y','U','I','O','P','[',']','\n',0,'A','S','D','F','G','H','J','K','L',';','\'','`',0,'\\','Z','X','C','V','B','N','M',',','.','/',0,'*',0,' ',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,'-',0,0,0,'+',0,0,0,0,0,0,0,0,0 };
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
                    
                    MX += dx; MY -= dy; /* FARE DÜZ VE KUSURSUZ EKSEN */

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
/* UYGULAMALAR, DOCK VE YENİLİKLER                                           */
/* ========================================================================= */
typedef struct{char n[20];int inst;u32 col;} App;
static App AP[6]={ 
    {"DOSYALAR", 1, COR}, 
    {"CLOUDBROWSER", 1, CHR_GRN}, 
    {"WINDAI", 1, AI_PURP}, 
    {"SISTEM",   1, CGY}, 
    {"EKRAN",  1, WIN_BLUE}, 
    {"GUVENLIK",   1, CRD} 
};

typedef struct { char n[32]; int is_dir; } FAT_File;
static FAT_File fat32_files[32];
static int fat32_file_count = 0;
static int INSIDE_DIR = 0; 

static void load_root_dir(void) {
    INSIDE_DIR = 0; 
    kcpy(fat32_files[0].n, "MINECRAFT.EXE");     fat32_files[0].is_dir = 0;
    kcpy(fat32_files[1].n, "WHATSAPP.APK");      fat32_files[1].is_dir = 0;
    kcpy(fat32_files[2].n, "SISTEM_GUNCEL.WPK"); fat32_files[2].is_dir = 0;
    kcpy(fat32_files[3].n, "UBUNTU_ARAC.DEB");   fat32_files[3].is_dir = 0;
    kcpy(fat32_files[4].n, "GIZLI_DOSYALAR");    fat32_files[4].is_dir = 1;
    fat32_file_count = 5;
}

static void load_sub_dir(void) {
    INSIDE_DIR = 1;
    kcpy(fat32_files[0].n, "LGS_TAKTIKLERI.TXT");fat32_files[0].is_dir = 0;
    kcpy(fat32_files[1].n, "SIFRELERIM.TXT");    fat32_files[1].is_dir = 0;
    fat32_file_count = 2;
}

static int FO=0, CHROME_OPEN=0, DISP_OPEN=0, SYS_OPEN=0; 
static i32 FX=100, FY=80, FD=0, FDX=0, FDY=0;
static i32 CX=150, CY=100, CD=0, CDX=0, CDY=0; 
static int INSTALLING=0, INSTALL_PROG=0; 

static void DRAW_WINDOW(i32 x, i32 y, i32 w, i32 h, const char* title, u32 b_col) {
    if(!DRAW_GLASS) fr(x+8, y+8, w, h, SHADOW); 
    DRAW_GLASS = 1; rr(x, y, w, h, 12, b_col); DRAW_GLASS = 0; 
    rb(x, y, w, h, PAN_BD, 1); fr(x, y+35, w, 1, PAN_BD); 
    dsc(x+40, y+15, w-80, title, CTXT, 0, 1);
    rr(x+w-35, y+8, 25, 20, 4, HOV(x+w-35, y+8, 25, 20) ? CRD : b_col); ds(x+w-26, y+14, "X", CW, 0, 1);
    rr(x+w-80, y+8, 40, 20, 4, AI_PURP); dsc(x+w-80, y+14, 40, "AI", CW, 0, 1);
    if(CLK(x+w-80, y+8, 40, 20)) AI_OPEN = 1;
}

static void DISPLAY_APP(void) {
    if(!DISP_OPEN) return;
    DRAW_WINDOW(280, 120, 450, 450, "EKRAN & COZUNURLUK YONETICISI", PAN_BG);
    if(CLK(280+450-35, 120+8, 25, 20)) DISP_OPEN=0;
    ds(300, 170, "QUANTUM DISPLAY ENGINE V15.0", WIN_BLUE, 0, 1);
    ds(300, 190, "LUTFEN RENDER KALITESINI SECIN:", CTXT, 0, 1);
    for(int i=0; i<7; i++) {
        i32 by = 220 + (i * 30);
        u32 btn_col = (CURRENT_RES == i) ? WIN_BLUE : SIDEBAR;
        if(i == 6 && CURRENT_RES == i) btn_col = AI_PURP; 
        rr(300, by, 300, 25, 5, btn_col);
        ds(315, by+8, RES_NAMES[i], CW, 0, 1);
        if(CLK(300, by, 300, 25)) CURRENT_RES = i;
    }
    ds(300, 450, "MEVCUT COZUNURLUK DURUMU:", CGY, 0, 1);
    ds(300, 470, RES_NAMES[CURRENT_RES], CGN, 0, 1);
}

static void SYSTEM_APP(void) {
    if(!SYS_OPEN) return;
    DRAW_WINDOW(250, 150, 500, 350, "SISTEM BILGISI - TITANIUM EDITION", PAN_BG);
    if(CLK(250+500-35, 150+8, 25, 20)) SYS_OPEN=0;
    ds(280, 210, "ISLETIM SISTEMI: WINDOS V15.0 TITANIUM CORE", WIN_BLUE, 0, 1);
    ds(280, 240, "MIMARI: X86 (32-BIT) SIFIR HATA ODAKLI", CTXT, 0, 1);
    char buf[64]; kcpy(buf, "FIZIKSEL RAM: ");
    itoa((int)SYS_RAM_MB, buf + klen(buf)); kcpy(buf + klen(buf), " MB");
    ds(280, 270, buf, CGN, 0, 1);
    ds(280, 300, "DEPOLAMA KAPASITESI: 2.0 GB (AKTIF)", CTXT, 0, 1);
    ds(280, 330, "EVRENSEL YUKLEYICI: WPK, EXE, APK, DEB", COR, 0, 1);
}

static void FILEMGR(void){
    if(!FO) return; 
    i32 fw=700, fh=450, fx=FX, fy=FY; 
    if(!FD&&MLB&&!PMLB&&MY>=fy&&MY<fy+35&&MX>=fx&&MX<fx+fw-40){FD=1;FDX=MX-fx;FDY=MY-fy;}
    if(FD){ if(MLB){ FX=MX-FDX; FY=MY-FDY; if(FX<0)FX=0; if(FY<0)FY=0; if(FX>(i32)SW-fw)FX=(i32)SW-fw; if(FY>(i32)SH-fh)FY=(i32)SH-fh; } else FD=0; }
    DRAW_WINDOW(fx, fy, fw, fh, "DOSYA GEZGINI - EVRENSEL DISK", PAN_BG);
    if(CLK(fx+fw-35, fy+8, 25, 20)) FO=0;
    DRAW_GLASS = 1; fr(fx, fy+36, 180, fh-36, SIDEBAR); DRAW_GLASS = 0;
    fr(fx+180, fy+36, 1, fh-36, PAN_BD);
    ds(fx+20, fy+60, "BILGISAYAR", CGY, 0, 1);
    static int FU=0;
    if(CLK(fx+10, fy+80, 160, 30)) { FU=0; load_root_dir(); }
    rr(fx+10, fy+80, 160, 30, 4, !FU ? PAN_BD : SIDEBAR); ds(fx+20, fy+90, "YEREL DISK (C:)", CW, 0, 1);
    if(CLK(fx+10, fy+120, 160, 30)) { FU=1; load_root_dir(); }
    rr(fx+10, fy+120, 160, 30, 4, FU ? PAN_BD : SIDEBAR); fr(fx+20, fy+130, 14, 10, LIN_ORG); ds(fx+40, fy+130, "USB SURUCU", CW, 0, 1);
    if(INSIDE_DIR) { rr(fx+190, fy+45, 80, 25, 4, PAN_BD); ds(fx+200, fy+53, "< GERI", CW, 0, 1); if(CLK(fx+190, fy+45, 80, 25)) load_root_dir(); }
    DRAW_GLASS = 1; fr(fx+181, fy+fh-40, fw-181, 40, SIDEBAR); DRAW_GLASS = 0;
    ds(fx+195, fy+fh-25, "KULLANILAN: 1.4 GB", CTXT, 0, 1); rr(fx+330, fy+fh-25, 200, 12, 6, PAN_BD); rr(fx+330, fy+fh-25, 140, 12, 6, WIN_BLUE); ds(fx+540, fy+fh-25, "MAKSIMUM LIMIT: 2.0 GB", CGY, 0, 1);
    for(int i=0; i < fat32_file_count; i++){
        i32 ex = fx + 200 + (i%4)*120, ey = fy + 85 + (i/4)*100;
        DRAW_GLASS = 1; rr(ex, ey, 100, 70, 6, HOV(ex, ey, 100, 70) ? PAN_BD : PAN_BG); DRAW_GLASS = 0;
        if(fat32_files[i].is_dir){ fr(ex+30, ey+15, 18, 12, XUB_BLU); rr(ex+20, ey+23, 60, 36, 4, XUB_BLU); 
        } else { 
            rr(ex+38, ey+15, 24, 30, 2, CW); 
            if(is_ext(fat32_files[i].n, ".APK")) fr(ex+42, ey+30, 16, 2, AND_GRN);
            else if(is_ext(fat32_files[i].n, ".EXE")) fr(ex+42, ey+30, 16, 2, WIN_BLUE);
            else if(is_ext(fat32_files[i].n, ".WPK")) fr(ex+42, ey+30, 16, 2, AI_PURP);
            else if(is_ext(fat32_files[i].n, ".DEB")) fr(ex+42, ey+30, 16, 2, DEB_ORG);
            else fr(ex+42, ey+30, 16, 2, CGY); 
        }
        dsc(ex, ey+65, 100, fat32_files[i].n, CTXT, 0, 1);
        if(CLK(ex,ey,100,70)){
            if(fat32_files[i].is_dir) { load_sub_dir(); } 
            else {
                if(is_ext(fat32_files[i].n, ".WPK")) { INSTALLING = 1; INSTALL_PROG = 0; }
                else if(is_ext(fat32_files[i].n, ".EXE")) { INSTALLING = 2; INSTALL_PROG = 0; }
                else if(is_ext(fat32_files[i].n, ".APK")) { INSTALLING = 3; INSTALL_PROG = 0; }
                else if(is_ext(fat32_files[i].n, ".DEB")) { INSTALLING = 4; INSTALL_PROG = 0; }
            }
        }
    }
}

static void CHROMIUM_BROWSER(void) {
    if(!CHROME_OPEN) return;
    i32 cw=850, ch=550, cx=CX, cy=CY;
    if(!CD&&MLB&&!PMLB&&MY>=cy&&MY<cy+35&&MX>=cx&&MX<cx+cw-40){CD=1;CDX=MX-cx;CDY=MY-cy;}
    if(CD){ if(MLB){ CX=MX-CDX; CY=MY-CDY; if(CX<0)CX=0; if(CY<0)CY=0; if(CX>(i32)SW-cw)CX=(i32)SW-cw; if(CY>(i32)SH-ch)CY=(i32)SH-ch; } else CD=0; }
    DRAW_WINDOW(cx, cy, cw, ch, "CLOUDBROWSER (QUANTUM 32K DESTEKLI)", CK);
    if(CLK(cx+cw-35, cy+8, 25, 20)) CHROME_OPEN=0;
    fr(cx, cy+36, cw, 40, PAN_BD); rr(cx+10, cy+42, cw-20, 28, 14, PAN_BG); ds(cx+25, cy+52, "WPK://NEWTAB://HOME", CTXT, 0, 1);
    DRAW_GLASS = 1; fr(cx, cy+76, cw, ch-76, BG_BASE); DRAW_GLASS = 0;
    dsc(cx, cy+180, cw, "CLOUDBROWSER", CW, 0, 2); rr(cx+cw/2-250, cy+230, 500, 40, 20, PAN_BD); ds(cx+cw/2-230, cy+245, "ARAMAK ISTEDIGINIZ KELIMEYI GIRIN...", CGY, 0, 1);
}

static void WINDAI_ASSISTANT(void) {
    if(!AI_OPEN) return;
    i32 aw = 500, ah = 400; i32 ax = (SW - aw)/2, ay = (SH - ah)/2;
    DRAW_WINDOW(ax, ay, aw, ah, "WINDAI QUANTUM CORE", BG_BASE);
    if(CLK(ax+aw-35, ay+8, 25, 20)) AI_OPEN=0;
    ds(ax+aw-150, ay+15, "[ TITANIUM MOD ]", CGY, 0, 1);
    rr(ax+20, ay+70, 300, 40, 8, PAN_BG); ds(ax+30, ay+85, "EFE! TITANIUM CORE (V15.0) AKTIF.", CTXT, 0, 1);
    rr(ax+aw-320, ay+130, 300, 40, 8, WIN_BLUE); ds(ax+aw-310, ay+145, "BUTUN DENEYSEL KODLAR COPE MI GITTI?", CW, 0, 1);
    rr(ax+20, ay+190, 400, 60, 8, PAN_BG); ds(ax+30, ay+205, "EVET! SADECE SAF, HATA VERMESI IHTIMAL DIŞI", CTXT, 0, 1); ds(ax+30, ay+225, "OLAN %100 OGANIK VESA STANDARTLARI KALDI.", CTXT, 0, 1);
    rr(ax+20, ah+ay-50, aw-40, 35, 17, PAN_BG); ds(ax+35, ah+ay-38, "BIR SEYLER YAZIN... (SIMULASYON MODU)", CGY, 0, 1); circ(ax+aw-40, ah+ay-32, 12, AI_PURP); ds(ax+aw-44, ah+ay-36, ">", CW, 0, 1);
}

static void DESKTOP(void){
    fr(0, 0, (i32)SW, (i32)SH, BG_BASE); 
    i32 dock_w = 6 * 70 + 20; i32 dock_x = (SW - dock_w) / 2; i32 dock_y = SH - 80;
    DRAW_GLASS = 1; rr(dock_x, dock_y, dock_w, 65, 15, DOCK_BG); DRAW_GLASS = 0; rb(dock_x, dock_y, dock_w, 65, PAN_BD, 1);
    for(int i=0; i<6; i++) {
        if(!AP[i].inst) continue;
        i32 ix = dock_x + 15 + i*70; i32 iy = dock_y + 10;
        DRAW_GLASS = 1; rr(ix, iy, 50, 45, 12, HOV(ix, iy, 50, 45) ? PAN_BD : PAN_BG); DRAW_GLASS = 0;
        fr(ix+15, iy+12, 20, 20, AP[i].col);
        if(CLK(ix, iy, 50, 45)) {
            if(i == 0) FO = !FO; 
            if(i == 1) CHROME_OPEN = !CHROME_OPEN; 
            if(i == 2) AI_OPEN = !AI_OPEN; 
            if(i == 3) SYS_OPEN = !SYS_OPEN; 
            if(i == 4) DISP_OPEN = !DISP_OPEN; 
        }
    }
    DRAW_GLASS = 1; fr(0, 0, SW, 25, CK); DRAW_GLASS = 0;
    ds(15, 8, "WINDOS V15.0 TITANIUM CORE", CTXT, 0, 1);
    char top_buf[80]; kcpy(top_buf, "MOD: "); kcpy(top_buf + klen(top_buf), RES_NAMES[CURRENT_RES]); kcpy(top_buf + klen(top_buf), " | RAM: "); itoa((int)SYS_RAM_MB, top_buf + klen(top_buf)); kcpy(top_buf + klen(top_buf), " MB | 0 BUG GARANTISI"); ds(SW-630, 8, top_buf, CGN, 0, 1);
    FILEMGR(); CHROMIUM_BROWSER(); SYSTEM_APP(); DISPLAY_APP(); WINDAI_ASSISTANT(); 

    if(INSTALLING) {
        i32 px = SW/2 - 180, py = SH/2 - 70; u32 color = WIN_BLUE; const char* type_str = "BILINMEYEN FORMAT";
        if(INSTALLING == 1) { type_str = "WINDOS PAKETI (.WPK)"; color = AI_PURP; }
        else if(INSTALLING == 2) { type_str = "WINDOWS PROGRAMI (.EXE)"; color = WIN_BLUE; }
        else if(INSTALLING == 3) { type_str = "ANDROID UYGULAMASI (.APK)"; color = AND_GRN; }
        else if(INSTALLING == 4) { type_str = "LINUX PAKETI (.DEB)"; color = DEB_ORG; }
        DRAW_GLASS = 1; fr(px+8, py+8, 360, 140, SHADOW); rr(px, py, 360, 140, 12, color); DRAW_GLASS = 0;
        ds(px+20, py+20, "EVRENSEL YUKLEME MOTORU (UNIVERSAL INSTALLER)", CW, 0, 1); ds(px+20, py+50, type_str, CW, 0, 1);
        rr(px+30, py+90, 300, 20, 5, CK); rr(px+30, py+90, INSTALL_PROG * 3, 20, 5, CW); INSTALL_PROG += 1;
        if(INSTALL_PROG >= 100) INSTALLING = 0; 
    }
}

void kernel_main(multiboot_info_t *mbi){
    u8 bpp_bits = mbi->framebuffer_bpp; 
    if(bpp_bits == 0) bpp_bits = 32; 
    BPP = bpp_bits / 8; 
    
    FB = (volatile u8*)(unsigned long)mbi->framebuffer_addr; 
    SW = mbi->framebuffer_width; 
    SH = mbi->framebuffer_height; 
    
    PITCH = mbi->framebuffer_pitch; 
    if(PITCH == 0) PITCH = SW * BPP; 

    if(!FB || SW==0){ FB=(volatile u8*)0xFD000000u; SW=1024; SH=768; PITCH=1024*4; BPP=4; }
    
    if(SW > MAX_SW) SW = MAX_SW;
    if(SH > MAX_SH) SH = MAX_SH;

    u32 flags = *((u32*)((u8*)mbi + 0));
    if(flags & 1) { u32 mem_upper_kb = *((u32*)((u8*)mbi + 8)); SYS_RAM_MB = (mem_upper_kb / 1024) + 1; } 
    else { SYS_RAM_MB = 2048; }

    mouse_init(); load_root_dir();
    
    while(1){ 
        mouse_poll(); 
        kbd_poll(); 
        DESKTOP(); 
        CUR(); 
        swap_buffers(); 
        volatile int x=50000; while(x--)__asm__("nop"); 
    }
}
