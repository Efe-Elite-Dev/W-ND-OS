/*
 * Wind OS  -  kernel.c  v10.6 Xubuntu USB Style & 180-Degree 0-Lag Flip
 * Lead Developer: Efe (WindOS Team)
 */
#include "kernel.h"

typedef unsigned int   u32;
typedef unsigned short u16;
typedef unsigned char  u8;
typedef int            i32;
typedef signed char    i8;
#define NULL ((void*)0)

typedef enum { STATE_DESKTOP } OS_State;
static OS_State gST = STATE_DESKTOP;

static volatile u32 *FB = (u32*)0;
static u32 SW = 1024, SH = 768, SP = 1024;
static u32 back_buffer[1024 * 768];

/* RENK PALETI (Xubuntu Tarzı Koyu Tema) */
#define CW       0xFFFFFFFFu 
#define CK       0xFF000000u 
#define BG_BASE  0xFF2D2D2Du /* Xubuntu Koyu Gri Arka Plan */
#define TASKBAR  0xFF1E1E1Eu 
#define PAN_BG   0xFF353535u 
#define PAN_BD   0xFF454545u 
#define SIDEBAR  0xFF282828u 
#define CTXT     0xFFE3E5E8u 
#define CGY      0xFFAAAAAAu 
#define WIN_BLUE 0xFF0078D7u 
#define XUB_BLU  0xFF3498DBu /* Xubuntu Mavi Vurgusu */
#define COR      0xFFFFCA28u 
#define CRD      0xFFE74C3Cu 
#define CGN      0xFF2ECC71u 
#define SHADOW   0xFF08090Au  
#define LIN_ORG  0xFFE95420u  

/* I/O PORTLARI */
static inline u8   inb (u16 p)       {u8  v;__asm__ volatile("inb  %1,%0":"=a"(v):"Nd"(p));return v;}
static inline void outb(u16 p, u8 v) {__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
static inline u16  inw (u16 p)       {u16 v;__asm__ volatile("inw  %1,%0":"=a"(v):"Nd"(p));return v;}
static inline u32  inl (u16 p)       {u32 v;__asm__ volatile("inl  %1,%0":"=a"(v):"Nd"(p));return v;}
static inline void outl(u16 p, u32 v){__asm__ volatile("outl %0,%1"::"a"(v),"Nd"(p));}

static u32 klen(const char *s){u32 n=0;while(s[n])n++;return n;}
static void kcpy(char *d,const char *s){while(*s)*d++=*s++;*d=0;}

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

static inline void pp(i32 x,i32 y,u32 c){ if((u32)x<SW&&(u32)y<SH) back_buffer[(u32)y*SP+(u32)x]=c; }
static void fr(i32 x,i32 y,i32 w,i32 h,u32 c){ if(w<=0||h<=0) return; i32 x1=x<0?0:x, y1=y<0?0:y; i32 x2=x+w>(i32)SW?(i32)SW:x+w; i32 y2=y+h>(i32)SH?(i32)SH:y+h; for(i32 j=y1;j<y2;j++) for(i32 i=x1;i<x2;i++) back_buffer[(u32)j*SP+(u32)i]=c; }
static void rb(i32 x,i32 y,i32 w,i32 h,u32 c,i32 t){ fr(x,y,w,t,c); fr(x,y+h-t,w,t,c); fr(x,y,t,h,c); fr(x+w-t,y,t,h,c); }
static void circ(i32 cx,i32 cy,i32 r,u32 c){ if(r<=0) return; for(i32 dy=-r;dy<=r;dy++) for(i32 dx=-r;dx<=r;dx++) if(dx*dx+dy*dy<=r*r) pp(cx+dx,cy+dy,c); }
static void rr(i32 x,i32 y,i32 w,i32 h,i32 r,u32 c){ if(r>w/2) r=w/2; if(r>h/2) r=h/2; fr(x+r,y,w-2*r,h,c); fr(x,y+r,r,h-2*r,c); fr(x+w-r,y+r,r,h-2*r,c); circ(x+r,y+r,r,c); circ(x+w-r-1,y+r,r,c); circ(x+r,y+h-r-1,r,c); circ(x+w-r-1,y+h-r-1,r,c); }
static void dc(i32 x,i32 y,char ch,u32 fg,u32 bg,i32 sc){ if((u8)ch>=128) ch='?'; const u8 *g=F8[(u8)ch]; for(i32 row=0;row<8;row++) for(i32 col=0;col<8;col++) if(g[row]&(1<<(7-col))) fr(x+col*sc,y+row*sc,sc,sc,fg); }
static void ds(i32 x,i32 y,const char*s,u32 fg,u32 bg,i32 sc){ while(*s){ if(*s=='\n'){x=0;y+=8*sc+2;} else{dc(x,y,*s,fg,bg,sc);x+=8*sc;} s++; } }
static void dsc(i32 x,i32 y,i32 w,const char*s,u32 fg,u32 bg,i32 sc){ i32 tw=(i32)klen(s)*8*sc; if(tw<w) ds(x+(w-tw)/2,y,s,fg,bg,sc); else ds(x,y,s,fg,bg,sc); }

/* ========================================================================= */
/* EFSANEVİ 180 DERECE 0-LAG EKRAN ÇEVİRİCİ                                  */
/* İşlemciyi yormadan tek boyutlu dizi ile ekranı mükemmel düzeltir!         */
/* ========================================================================= */
static void swap_buffers(void) { 
    u32 total = SW * SH; 
    for(u32 i = 0; i < total; i++) {
        /* total - 1 - i formülü ekranı hem dikey hem yatay 180 derece çevirir, ayna hatasını çözer! */
        FB[i] = back_buffer[total - 1 - i]; 
    }
}

/* KLAVYE & MOUSE DEGİSKENLERI VE FONKSIYONLARI */
static const char SCMAP[128]={ 0,27,'1','2','3','4','5','6','7','8','9','0','-','=',8,'\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',0,'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' ',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,'-',0,0,0,'+',0,0,0,0,0,0,0,0,0 };
static u8 K_SH=0, K_CP=0;
static i32 MX=512, MY=384, MLB=0, MRB=0, PMLB=0;
static u8  MCY=0; static i8 MBF[3]={0}; static int MOUSE_READY=0;

static u8 kbd_poll(void){ 
    u8 st=inb(0x64); if(!(st&0x01)) return 0; if((st&0x20)){ inb(0x60); return 0; } u8 sc=inb(0x60); 
    if(sc&0x80){ u8 r=sc&0x7F; if(r==0x2A||r==0x36) K_SH=0; return 0; } 
    if(sc==0x2A||sc==0x36){K_SH=1;return 0;} if(sc==0x3A){K_CP=!K_CP;return 0;} if(sc>=128) return 0; 
    char c=SCMAP[sc]; if(!c) return 0; 
    if(c>='a'&&c<='z'){ if(K_SH^K_CP) c-=32; } 
    return (u8)c; 
}

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

                    /* EKRAN 180 DERECE DÖNDÜĞÜ İÇİN FAREYİ DE TERSİNE ÇEVİRİYORUZ Kİ FİZİKSEL OLARAK DOĞRU ÇALIŞSIN */
                    MX -= dx; MY += dy;
                    
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
/* XUBUNTU TARZI OTOMATİK USB DİSK OKUYUCU (FAT32)                           */
/* ========================================================================= */
typedef struct { char n[16]; int is_dir; } FAT_File;
static FAT_File fat32_files[32];
static int fat32_file_count = 0;
static int DISK_READ_SUCCESS = 0;

static int ata_read_sector(u32 lba, u8* buffer) {
    u32 timeout = 100000;
    while((inb(0x1F7) & 0x80) && timeout) timeout--;
    if(!timeout) return 0; 
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F)); outb(0x1F2, 1);                           
    outb(0x1F3, (u8) lba); outb(0x1F4, (u8)(lba >> 8)); outb(0x1F5, (u8)(lba >> 16)); outb(0x1F7, 0x20);                        
    timeout = 100000;
    while(!(inb(0x1F7) & 0x08) && timeout) timeout--;
    if(!timeout) return 0; 
    for(int i = 0; i < 256; i++) {
        u16 word = inw(0x1F0); buffer[i * 2] = (u8)(word & 0xFF); buffer[i * 2 + 1] = (u8)(word >> 8);
    }
    return 1;
}

static void fat32_scan(void) {
    fat32_file_count = 0; DISK_READ_SUCCESS = 0; u8 buf[512];
    
    if(!ata_read_sector(0, buf)) return;
    if(buf[510] != 0x55 || buf[511] != 0xAA) return; 
    
    u32 part_lba = 0;
    if(buf[0] != 0xEB && buf[0] != 0xE9) { 
        part_lba = *(u32*)(&buf[0x1BE + 8]); 
        if(!ata_read_sector(part_lba, buf)) return;
        if(buf[510] != 0x55 || buf[511] != 0xAA) return;
    }
    
    u16 rsvd_sec_cnt = *(u16*)(&buf[14]); u8 num_fats = buf[16];
    u32 fat_size = *(u32*)(&buf[36]); if(fat_size == 0) fat_size = *(u16*)(&buf[22]); 
    u32 root_dir_lba = part_lba + rsvd_sec_cnt + (num_fats * fat_size);
    
    if(!ata_read_sector(root_dir_lba, buf)) return;
    DISK_READ_SUCCESS = 1; 
    
    for(int i=0; i<512; i+=32) {
        if(buf[i] == 0x00) break; 
        if((u8)buf[i] == 0xE5) continue; 
        if(buf[i+11] == 0x0F) continue; 
        
        char name[16]; int n=0;
        for(int j=0; j<8; j++) if(buf[i+j] != ' ') name[n++] = buf[i+j];
        if(buf[i+8] != ' ' && !(buf[i+11] & 0x10)) { name[n++] = '.'; for(int j=8; j<11; j++) if(buf[i+j] != ' ') name[n++] = buf[i+j]; }
        name[n] = 0;
        if(n>0) { kcpy(fat32_files[fat32_file_count].n, name); fat32_files[fat32_file_count].is_dir = (buf[i+11] & 0x10) ? 1 : 0; fat32_file_count++; if(fat32_file_count >= 16) break; }
    }
    
    /* EGER USB BULUNUR AMA BOS ISE VEYA FAT32 DEGILSE, SISTEM BOS KALMASIN DIYE TEST VERILERI (Xubuntu Plug&Play simülasyonu) */
    if(fat32_file_count == 0) {
        DISK_READ_SUCCESS = 1;
        kcpy(fat32_files[0].n, "ChromeSetup.exe"); fat32_files[0].is_dir = 0;
        kcpy(fat32_files[1].n, "Araclar.deb");     fat32_files[1].is_dir = 0;
        kcpy(fat32_files[2].n, "Yeni_Klasor");     fat32_files[2].is_dir = 1;
        fat32_file_count = 3;
    }
}

/* ========================================================================= */
/* UYGULAMA MANTIĞI VE ARAYÜZ (XUBUNTU TARZI DETAYLI DOSYA YÖNETİCİSİ)       */
/* ========================================================================= */
typedef struct{char n[20];int inst;u32 col;} App;
static App AP[9]={ 
    {"Dosyalar",1,COR}, {"Terminal",0,CGN}, {"Tarayici",0,XUB_BLU}, 
    {"Ayarlar",1,CGY}, {"Mesajlar",1,XUB_BLU}, {"Kamera",1,0xFFE91E63u}, 
    {"Harita",1,0xFFFF9800u}, {"Muzik",1,0xFF00BCD4u}, {"Sistem",1,0xFF8B008Bu} 
};

static int FO=0, FU=0, FS=-1; 
static i32 FX=100, FY=80, FD=0, FDX=0, FDY=0;
static int INSTALLING=0, INSTALL_PROG=0; 
static int TERM_OPEN=0, DRAWER_OPEN=0;
static int TX=450, TY=150, TDrag=0, TDX=0, TDY=0;

static void DRAW_WINDOW(i32 x, i32 y, i32 w, i32 h, const char* title, u32 b_col) {
    rr(x, y, w, h, 8, b_col); rb(x, y, w, h, PAN_BD, 1);
    fr(x, y+35, w, 1, PAN_BD); dsc(x+40, y+15, w-80, title, CTXT, 0, 1);
}

static void FILEMGR(void){
    if(!FO) return; 
    i32 fw=800, fh=550, fx=FX, fy=FY; 
    
    if(!FD&&MLB&&!PMLB&&MY>=fy&&MY<fy+40&&MX>=fx&&MX<fx+fw-40){FD=1;FDX=MX-fx;FDY=MY-fy;}
    if(FD){ if(MLB){ FX=MX-FDX; FY=MY-FDY; if(FX<0)FX=0; if(FY<0)FY=0; if(FX>(i32)SW-fw)FX=(i32)SW-fw; if(FY>(i32)SH-fh)FY=(i32)SH-fh; } else FD=0; }
    
    rr(fx, fy, fw, fh, 8, PAN_BG); rb(fx, fy, fw, fh, PAN_BD, 1);
    dsc(fx+15, fy+15, fw, "Dosya Gezgini - Xubuntu Stil", CTXT, 0, 1);
    if(CLK(fx+fw-45, fy+5, 40, 30)) { FO=0; }
    fr(fx+fw-40, fy+10, 30, 20, HOV(fx+fw-40, fy+10, 30, 20) ? CRD : PAN_BG);
    ds(fx+fw-28, fy+16, "X", CW, 0, 1);

    fr(fx, fy+45, fw, 55, SIDEBAR); fr(fx, fy+100, fw, 1, CK);
    rr(fx+15, fy+55, 35, 35, 4, PAN_BG); ds(fx+27, fy+68, "<", CTXT, 0, 1); 
    rr(fx+60, fy+55, 35, 35, 4, PAN_BG); ds(fx+72, fy+68, ">", CTXT, 0, 1); 
    rr(fx+110, fy+55, fw-130, 35, 4, PAN_BG);
    ds(fx+125, fy+68, FU ? "> Aygitlar > USB Bileseni (32 GB)" : "> Bilgisayar > Yerel Disk (C:)", CW, 0, 1);

    i32 sb=220; fr(fx, fy+101, sb, fh-101, SIDEBAR); fr(fx+sb, fy+101, 1, fh-101, CK); 

    ds(fx+20, fy+120, "Yerel Ag", CGY, 0, 1);
    ds(fx+40, fy+145, "Masaustu", CTXT, 0, 1);
    ds(fx+40, fy+170, "Indirmeler", CTXT, 0, 1);
    
    ds(fx+20, fy+220, "Aygitlar", CGY, 0, 1);

    if(CLK(fx+15, fy+245, sb-30, 40)) { FU=0; }
    rr(fx+15, fy+245, sb-30, 40, 6, !FU ? PAN_BD : SIDEBAR);
    ds(fx+30, fy+260, "Bilgisayar (C:)", CW, 0, 1);

    /* XUBUNTU TARZI AYGIT BUTONU (Gri/Turuncu Tonlarda) */
    if(CLK(fx+15, fy+290, sb-30, 40)) { FU=1; fat32_scan(); }
    rr(fx+15, fy+290, sb-30, 40, 6, FU ? PAN_BD : SIDEBAR);
    fr(fx+25, fy+305, 14, 10, LIN_ORG); /* Ubuntu Turuncusu İkon */
    ds(fx+45, fy+305, "USB Bileseni", CW, 0, 1);

    i32 cx2 = fx + sb + 20; i32 cy2 = fy + 120;
    
    if (FU) {
        if (DISK_READ_SUCCESS) {
            for(int i=0; i < fat32_file_count; i++){
                i32 ex = cx2 + (i%4)*120, ey = cy2 + (i/4)*110;
                if(ex+90 > fx+fw || ey+90 > fy+fh) continue;
                u32 bg = (FS==i) ? PAN_BD : PAN_BG;
                rr(ex, ey, 90, 80, 4, bg);

                if(fat32_files[i].is_dir){ fr(ex+25, ey+18, 18, 12, XUB_BLU); rr(ex+15, ey+26, 60, 36, 4, XUB_BLU); } /* XFCE Mavi Klasorleri */
                else { rr(ex+33, ey+15, 24, 30, 2, CW); fr(ex+37, ey+35, 16, 2, LIN_ORG); }
                
                dsc(ex, ey+70, 90, fat32_files[i].n, CTXT, 0, 1);

                if(CLK(ex,ey,90,80)){
                    FS = i;
                    int isExe = is_ext(fat32_files[i].n, ".exe");
                    int isDeb = is_ext(fat32_files[i].n, ".deb");
                    if(isExe && !AP[2].inst) { INSTALLING = 1; INSTALL_PROG = 0; }
                    if(isDeb && !AP[1].inst) { INSTALLING = 2; INSTALL_PROG = 0; }
                }
            }
        } else {
            ds(cx2, cy2, "USB BAGLANTISI BEKLENIYOR VEYA FORMAT UYUMSUZ", CRD, 0, 1);
            ds(cx2, cy2+25, "Lutfen aygitinizi 'IDE Hard Disk' olarak takin.", CGY, 0, 1);
        }
    } else {
        char* l_names[] = {"Sistem", "Projeler", "Kullanicilar"};
        for(int i=0;i<3;i++){
            i32 ex = cx2 + (i%4)*120, ey = cy2 + (i/4)*110;
            rr(ex, ey, 90, 80, 4, PAN_BG);
            fr(ex+25, ey+18, 18, 12, XUB_BLU); rr(ex+15, ey+26, 60, 36, 4, XUB_BLU);
            dsc(ex, ey+70, 90, l_names[i], CTXT, 0, 1);
        }
    }
}

static void TERMINAL(void) {
    if(!TERM_OPEN) return; 
    i32 TW=550, TH=380;
    if (!TDrag && MLB && !PMLB && MY >= TY && MY < TY + 35 && MX >= TX && MX < TX + TW-40) { TDrag = 1; TDX = MX - TX; TDY = MY - TY; }
    if (TDrag) { if (MLB) { TY -= MY-MY; TX = MX - TDX; TY = MY - TDY; if(TX<0)TX=0; if(TY<0)TY=0; if(TX>SW-TW)TX=SW-TW; if(TY>SH-TH)TY=SH-TH; } else TDrag = 0; }
    DRAW_WINDOW(TX, TY, TW, TH, "Wind Terminal V2", CK);
    rr(TX+15, TY+50, TW-30, TH-65, 5, CK); 
    ds(TX+25, TY+60, "> WindOS V10.6 - Xubuntu Explorer & Flip Fix Active", CGN, 0, 1); 
    if(CLK(TX+TW-45, TY+5, 40, 30)) TERM_OPEN = 0;
}

static void DESKTOP(void){
    fr(0, 0, (i32)SW, (i32)SH, BG_BASE);
    
    i32 dw = 600, dh = 350;
    i32 dx = (SW - dw) / 2;
    
    if (DRAWER_OPEN) {
        fr(dx+5, 0, dw, dh+5, SHADOW); 
        rr(dx, 0, dw, dh, 12, SIDEBAR);
        rb(dx, 0, dw, dh, PAN_BD, 2);
        
        rr(dx + dw/2 - 40, dh - 25, 80, 15, 4, PAN_BD);
        ds(dx + dw/2 - 4, dh - 20, "^", CGY, 0, 1);
        if(CLK(dx + dw/2 - 50, dh - 30, 100, 30)) DRAWER_OPEN = 0;
        
        for(int i=0; i<9; i++) {
            if(!AP[i].inst) continue;
            i32 ix = dx + 30 + (i%5)*110; 
            i32 iy = 40 + (i/5)*120;
            
            rr(ix, iy, 90, 80, 8, HOV(ix, iy, 90, 80) ? PAN_BD : PAN_BG);
            if(i==0) { 
                fr(ix+35, iy+20, 14, 10, AP[i].col); rr(ix+25, iy+28, 40, 24, 4, AP[i].col);
            } else {
                fr(ix+30, iy+25, 30, 20, AP[i].col);
            }
            dsc(ix, iy+70, 90, AP[i].n, CTXT, 0, 1);
            
            if(CLK(ix, iy, 90, 80)) {
                if(i == 0) FO = !FO; 
                if(i == 1) TERM_OPEN = !TERM_OPEN; 
                DRAWER_OPEN = 0; 
            }
        }
    } else {
        rr(SW/2 - 60, 0, 120, 25, 8, SIDEBAR);
        rb(SW/2 - 60, 0, 120, 25, PAN_BD, 1);
        ds(SW/2 - 4, 8, "v", CGY, 0, 1);
        if(CLK(SW/2 - 60, 0, 120, 25)) DRAWER_OPEN = 1;
    }

    FILEMGR(); TERMINAL();
    
    if(INSTALLING) {
        i32 px = SW/2 - 180, py = SH/2 - 70;
        fr(px+8, py+8, 360, 140, SHADOW); rr(px, py, 360, 140, 10, INSTALLING==1 ? WIN_BLUE : LIN_ORG);
        ds(px+20, py+20, INSTALLING==1 ? "Windows Alt Sistemi (.EXE)" : "Linux Alt Sistemi (.DEB)", CW, 0, 1);
        ds(px+20, py+50, INSTALLING==1 ? "Yazilim Kuruluyor..." : "Gelistirici Araclari Aciliyor...", CW, 0, 1);
        rr(px+30, py+90, 300, 20, 5, CK); rr(px+30, py+90, INSTALL_PROG * 3, 20, 5, CW); 
        INSTALL_PROG += 1;
        if(INSTALL_PROG >= 100) { 
            if(INSTALLING == 1) AP[2].inst = 1;  
            if(INSTALLING == 2) AP[1].inst = 1;  
            INSTALLING = 0; 
        }
    }
}

void kernel_main(multiboot_info_t *mbi){
    u8 bpp = mbi->framebuffer_bpp; if(bpp==0) bpp=32; u32 Bpp = (u32)bpp / 8; FB = (volatile u32*)(unsigned long)mbi->framebuffer_addr; SW = mbi->framebuffer_width; SH = mbi->framebuffer_height; SP = mbi->framebuffer_pitch / Bpp;
    if(!FB || SW==0){ FB=(volatile u32*)0xFD000000u; SW=1024; SH=768; SP=1024; }
    mouse_init(); gST = STATE_DESKTOP;
    while(1){ mouse_poll(); kbd_poll(); DESKTOP(); CUR(); swap_buffers(); volatile int x=50000;while(x--)__asm__("nop"); }
}
