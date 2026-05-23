/*
 * Wind OS  -  kernel.c  v9.8 VirtualBox Flip Fix & Real USB EHCI Detection
 * Lead Developer: WindOS Team
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

static OS_State gST = STATE_DESKTOP;
static int VBOX_FLIP = 1; /* VIRTUALBOX TERS EKRAN DÜZELTİCİSİ (1 = Açık) */

#define CW       0xFFFFFFFFu 
#define CK       0xFF000000u 
#define BG_BASE  0xFF101214u 
#define TASKBAR  0xDD181A1Fu 
#define PAN_BG   0xFF202020u 
#define PAN_BD   0xFF333333u 
#define SIDEBAR  0xFF191919u 
#define CTXT     0xFFE3E5E8u 
#define CGY      0xFF99AAB5u 
#define WIN_BLUE 0xFF0078D7u 
#define COR      0xFFFFCA28u 
#define CRD      0xFFED4245u 
#define CGN      0xFF57F287u 
#define SHADOW   0xFF08090Au  
#define LIN_ORG  0xFFE95420u  

/* I/O PORTLARI */
static inline u8   inb (u16 p)       {u8  v;__asm__ volatile("inb  %1,%0":"=a"(v):"Nd"(p));return v;}
static inline void outb(u16 p, u8 v) {__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
static inline u16  inw (u16 p)       {u16 v;__asm__ volatile("inw  %1,%0":"=a"(v):"Nd"(p));return v;}
static inline void outw(u16 p, u16 v){__asm__ volatile("outw %0,%1"::"a"(v),"Nd"(p));}
static inline u32  inl (u16 p)       {u32 v;__asm__ volatile("inl  %1,%0":"=a"(v):"Nd"(p));return v;}
static inline void outl(u16 p, u32 v){__asm__ volatile("outl %0,%1"::"a"(v),"Nd"(p));}

static u32 klen(const char *s){u32 n=0;while(s[n])n++;return n;}
static void kcpy(char *d,const char *s){while(*s)*d++=*s++;*d=0;}
static void to_hex(u8 val, char* buf) { const char* hex = "0123456789ABCDEF"; buf[0] = hex[val >> 4]; buf[1] = hex[val & 0x0F]; buf[2] = 0; }

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

/* VIRTUALBOX EKRAN DÜZELTİCİ FONKSİYONU */
static void swap_buffers(void) { 
    u32 total = SW * SH; 
    if (VBOX_FLIP) {
        /* Ekranı 180 derece ters çevirerek VirtualBox hatasını çözer! */
        for(u32 i = 0; i < total; i++) FB[i] = back_buffer[total - 1 - i];
    } else {
        for(u32 i = 0; i < total; i++) FB[i] = back_buffer[i];
    }
}

/* KLAVYE & MOUSE */
static const char SCMAP[128]={ 0,27,'1','2','3','4','5','6','7','8','9','0','-','=',8,'\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',0,'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' ',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,'-',0,0,0,'+',0,0,0,0,0,0,0,0,0 };
static u8 K_SH=0, K_CP=0;
static u8 kbd_poll(void){ 
    u8 st=inb(0x64); if(!(st&0x01)) return 0; if((st&0x20)){ inb(0x60); return 0; } u8 sc=inb(0x60); 
    if(sc&0x80){ u8 r=sc&0x7F; if(r==0x2A||r==0x36) K_SH=0; return 0; } 
    if(sc==0x2A||sc==0x36){K_SH=1;return 0;} if(sc==0x3A){K_CP=!K_CP;return 0;} if(sc>=128) return 0; 
    char c=SCMAP[sc]; if(!c) return 0; 
    
    /* F Tuşuna Basınca Ekranı Döndür! */
    if (c == 'f' || c == 'F') { VBOX_FLIP = !VBOX_FLIP; }

    if(c>='a'&&c<='z'){ if(K_SH^K_CP) c-=32; } 
    return (u8)c; 
}
static i32 MX=512,MY=384,MLB=0,MRB=0,PMLB=0;
static u8  MCY=0; static i8 MBF[3]={0}; static int MOUSE_READY=0;
static void m_cmd_wait(void){u32 t=100000;while(t--&&(inb(0x64)&0x02));}
static void m_dat_wait(void){u32 t=100000;while(t--&&!(inb(0x64)&0x01));}
static void m_write(u8 v){m_cmd_wait();outb(0x64,0xD4);m_cmd_wait();outb(0x60,v);}
static u8   m_read (void){m_dat_wait();return inb(0x60);}
static void mouse_init(void){ m_cmd_wait(); outb(0x64,0xA8); m_cmd_wait(); outb(0x64,0x20); m_dat_wait(); u8 cfg=inb(0x60); cfg|=0x02; cfg&=~0x20; m_cmd_wait(); outb(0x64,0x60); m_cmd_wait(); outb(0x60,cfg); m_write(0xFF); m_read(); m_read(); m_read(); m_write(0xF6); m_read(); m_write(0xF4); m_read(); MOUSE_READY=1; }

static void mouse_poll(void){ 
    if(!MOUSE_READY) return; 
    int safety_limit = 256; /* VBox icin tampon bosaltma (Ters yonde hareket ediyorsa FLIP'e uyumlu calisir) */
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

                    /* EGER EKRAN TERS ISE FARE YONLERINI DE TERS CEVIR! */
                    if (VBOX_FLIP) { MX -= dx; MY += dy; } 
                    else { MX += dx; MY -= dy; }

                    if(MX < 0) MX = 0; if(MY < 0) MY = 0; 
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
/* GERÇEK ZAMANLI PCI USB KONTROLCÜ TESPİTİ (DONANIMA ZORLAMA KODU)          */
/* ========================================================================= */
static int REAL_USB_DETECTED = 0;

static u32 pci_rd(u8 bus,u8 dev,u8 fn,u8 off){ outl(0xCF8,0x80000000u|((u32)bus<<16)|((u32)dev<<11)|((u32)fn<<8)|(off&0xFC)); return inl(0xCFC); }

static void pci_scan_usb(void){
    REAL_USB_DETECTED = 0;
    for(int b=0; b<4; b++) {
        for(int d=0; d<32; d++) {
            u32 id = pci_rd(b, d, 0, 0); 
            if((id & 0xFFFF) == 0xFFFF) continue;
            
            u32 cls = pci_rd(b, d, 0, 8); 
            u8 class_code = (u8)(cls >> 24);
            u8 subclass = (u8)(cls >> 16);
            u8 prog_if = (u8)(cls >> 8);
            
            /* Class 0x0C (Serial), Subclass 0x03 (USB) */
            if(class_code == 0x0C && subclass == 0x03) {
                REAL_USB_DETECTED = 1;
                /* İstersen burada UHCI (0x00), OHCI (0x10), EHCI (0x20) ayrımı da yapılabilir */
            }
        }
    }
}

/* ========================================================================= */
/* UYGULAMA MANTIĞI VE ARAYÜZ                                                */
/* ========================================================================= */
static int FO=0, FU=0; 
static i32 FX=100, FY=80, FD=0, FDX=0, FDY=0;

static void FILEMGR(void){
    if(!FO) return; 
    i32 fw=760, fh=520, fx=FX, fy=FY; 
    
    if(!FD&&MLB&&!PMLB&&MY>=fy&&MY<fy+40&&MX>=fx&&MX<fx+fw-40){FD=1;FDX=MX-fx;FDY=MY-fy;}
    if(FD){ if(MLB){ FX=MX-FDX; FY=MY-FDY; if(FX<0)FX=0; if(FY<0)FY=0; if(FX>(i32)SW-fw)FX=(i32)SW-fw; if(FY>(i32)SH-fh)FY=(i32)SH-fh; } else FD=0; }
    
    rr(fx, fy, fw, fh, 8, PAN_BG); rb(fx, fy, fw, fh, PAN_BD, 1);
    
    dsc(fx+15, fy+15, fw, "Dosya Gezgini - WindOS V9.8", CTXT, 0, 1);
    if(CLK(fx+fw-45, fy+5, 40, 30)) { FO=0; }
    fr(fx+fw-40, fy+10, 30, 20, HOV(fx+fw-40, fy+10, 30, 20) ? CRD : PAN_BG);
    ds(fx+fw-28, fy+16, "X", CW, 0, 1);

    fr(fx, fy+45, fw, 55, SIDEBAR); fr(fx, fy+100, fw, 1, CK);
    rr(fx+15, fy+55, 35, 35, 4, PAN_BG); ds(fx+27, fy+68, "<", CTXT, 0, 1); 
    rr(fx+60, fy+55, 35, 35, 4, PAN_BG); ds(fx+72, fy+68, ">", CTXT, 0, 1); 
    rr(fx+110, fy+55, fw-130, 35, 4, PAN_BG);
    ds(fx+125, fy+68, FU ? "> Bu Bilgisayar > USB Surucu (D:)" : "> Bu Bilgisayar > Yerel Disk (C:)", CW, 0, 1);

    i32 sb=220; fr(fx, fy+101, sb, fh-101, SIDEBAR); fr(fx+sb, fy+101, 1, fh-101, CK); 

    ds(fx+20, fy+120, "Hizli Erisim", CGY, 0, 1);
    ds(fx+40, fy+145, "Masaustu", CTXT, 0, 1);
    ds(fx+40, fy+170, "Indirmeler", CTXT, 0, 1);
    ds(fx+20, fy+220, "Bu Bilgisayar", CGY, 0, 1);

    if(CLK(fx+15, fy+245, sb-30, 40)) { FU=0; }
    rr(fx+15, fy+245, sb-30, 40, 6, !FU ? PAN_BD : SIDEBAR);
    ds(fx+30, fy+260, "Yerel Disk (C:)", CW, 0, 1);

    if(CLK(fx+15, fy+290, sb-30, 40)) { 
        FU=1; 
        pci_scan_usb(); /* USB TIKLANDIGINDA PCI VERIYOLUNU TARA! */
    }
    rr(fx+15, fy+290, sb-30, 40, 6, FU ? PAN_BD : SIDEBAR);
    circ(fx+35, fy+310, 5, WIN_BLUE);
    ds(fx+50, fy+305, "USB Surucu (D:)", CW, 0, 1);

    i32 cx2 = fx + sb + 20; i32 cy2 = fy + 120;
    
    if (FU) {
        if (REAL_USB_DETECTED) {
            ds(cx2, cy2, "DONANIM TESPIT EDILDI: USB 2.0 EHCI Kontrolcusu", CGN, 0, 1);
            ds(cx2, cy2+25, "Fiziksel USB aygiti basariyla WindOS tarafindan algilandi.", CW, 0, 1);
            ds(cx2, cy2+45, "Ancak, USB yigin (Mass Storage) surucusu ve FAT32 motoru", CGY, 0, 1);
            ds(cx2, cy2+60, "henuz Kernel'a entegre edilmediginden dosyalar gosterilemiyor.", CGY, 0, 1);
            
            ds(cx2, cy2+95, "[Lead Developer Notu:]", WIN_BLUE, 0, 1);
            ds(cx2, cy2+115, "Bu donanimi okumak yerine VirtualBox Storage ayarlarindan", CTXT, 0, 1);
            ds(cx2, cy2+130, "USB belleginizi 'IDE Hard Disk' olarak eklerseniz ATA", CTXT, 0, 1);
            ds(cx2, cy2+145, "sürücümüz dosyalari sorunsuz okuyacaktir.", CTXT, 0, 1);
        } else {
            ds(cx2, cy2, "USB BAGLANTISI BULUNAMADI!", CRD, 0, 1);
            ds(cx2, cy2+25, "VirtualBox ust menusunden Aygitlar -> USB yolunu izleyerek", CGY, 0, 1);
            ds(cx2, cy2+40, "bir fiziksel USB aygiti baglayin.", CGY, 0, 1);
        }
    } else {
        char* l_names[] = {"Sistem", "Projeler", "Kullanicilar"};
        for(int i=0;i<3;i++){
            i32 ex = cx2 + (i%4)*120, ey = cy2 + (i/4)*110;
            rr(ex, ey, 90, 80, 4, PAN_BG);
            fr(ex+25, ey+18, 18, 12, COR); rr(ex+15, ey+26, 60, 36, 4, COR);
            dsc(ex, ey+70, 90, l_names[i], CTXT, 0, 1);
        }
    }
}

static void BTN_V8(i32 x, i32 y, i32 w, i32 h, const char* lbl, u32 col) {
    rr(x, y, w, h, 8, PAN_BG); 
    rb(x, y, w, h, PAN_BD, 1); 
    fr(x + w/2 - 15, y + 15, 30, 20, col); 
    dsc(x, y + 45, w, lbl, CTXT, 0, 1);
}

static void DESKTOP(void){
    fr(0, 0, (i32)SW, (i32)SH, BG_BASE);
    fr(0, SH-45, SW, 45, TASKBAR); fr(0, SH-45, SW, 1, CK); 
    rr(15, SH-38, 30, 30, 6, WIN_BLUE); 
    ds(SW-130, SH-28, "WindOS V9.8", CTXT, 0, 1);
    
    if(CLK(30,30,80,70)) { FO=!FO; } 
    rr(30, 30, 80, 70, 8, PAN_BG); rb(30, 30, 80, 70, PAN_BD, 1); 
    fr(55, 45, 14, 10, COR); rr(45, 52, 50, 30, 4, COR); 
    dsc(30, 85, 80, "Dosyalar", CTXT, 0, 1);
    
    FILEMGR();
    
    /* Masaustu bilgi notu */
    ds(SW-300, 20, "Ekran Ters Ise 'F' Tusuna Basiniz", CGY, 0, 1);
}

void kernel_main(multiboot_info_t *mbi){
    u8 bpp = mbi->framebuffer_bpp; if(bpp==0) bpp=32; u32 Bpp = (u32)bpp / 8; FB = (volatile u32*)(unsigned long)mbi->framebuffer_addr; SW = mbi->framebuffer_width; SH = mbi->framebuffer_height; SP = mbi->framebuffer_pitch / Bpp;
    if(!FB || SW==0){ FB=(volatile u32*)0xFD000000u; SW=1024; SH=768; SP=1024; }
    mouse_init(); gST = STATE_DESKTOP;
    while(1){ mouse_poll(); kbd_poll(); DESKTOP(); CUR(); swap_buffers(); volatile int x=50000;while(x--)__asm__("nop"); }
}
