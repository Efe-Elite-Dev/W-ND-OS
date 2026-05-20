#include "kernel.h"

/* =========================================================
   PORT G/Ç FONKSİYONLARI (Giriş/Çıkış Makroları)
   ========================================================= */
static inline u8 inb(u16 port) {
    u8 value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void outb(u16 port, u8 value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline u32 inl(u16 port) {
    u32 value;
    __asm__ volatile("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void outl(u16 port, u32 value) {
    __asm__ volatile("outl %0, %1" : : "a"(value), "Nd"(port));
}

/* =========================================================
   GLOBAL VE STATİK DEĞİŞKENLER
   ========================================================= */
static u32* FB        = (u32*)0;
static u32  SCR_W     = 1024;
static u32  SCR_H     = 768;
static u32  SCR_PITCH = 1024;

static OS_State state = STATE_SETUP_1_NAME;

// Kullanılmayan değişken uyarılarını engellemek için attribute eklendi
static u8 start_menu_open   __attribute__((unused)) = 0;
static u8 file_manager_open __attribute__((unused)) = 0;

static i32 mouse_x = 512;
static i32 mouse_y = 384;
static u8  mouse_btn = 0;
static u8  prev_mouse_btn = 0;

static char username[32] __attribute__((unused)) = "Efe";
static i32  user_len __attribute__((unused)) = 3;

/* =========================================================
   SÜRÜCÜ VE POLLING FONKSİYONLARI
   ========================================================= */
void mouse_init(void) {
    u32 timeout = 100000;
    
    // PS/2 Fare aktifleştirme komut dizisi
    while(timeout--) { if((inb(0x64) & 2) == 0) break; }
    outb(0x64, 0xA8); 
    
    timeout = 100000;
    while(timeout--) { if((inb(0x64) & 2) == 0) break; }
    outb(0x64, 0x20);
}

void mouse_poll(void) {
    // Kontrolcüde okunabilir veri var mı?
    if (inb(0x64) & 1) {
        // Veri fareye mi ait? (0x20 biti)
        if (inb(0x64) & 0x20) {
            u8 status = inb(0x60);
            i8 rel_x  = (i8)inb(0x60);
            i8 rel_y  = (i8)inb(0x60);

            mouse_x += rel_x;
            mouse_y -= rel_y; // PS/2 Y ekseni terstir

            // Ekran sınırları koruması
            if(mouse_x < 0) mouse_x = 0;
            if(mouse_y < 0) mouse_y = 0;
            if(mouse_x > (i32)SCR_W - 1) mouse_x = SCR_W - 1;
            if(mouse_y > (i32)SCR_H - 1) mouse_y = SCR_H - 1;

            prev_mouse_btn = mouse_btn;
            mouse_btn = (status & 0x07);
        }
    }
}

u8 kbd_poll(void) {
    if (inb(0x64) & 1) {
        // Fareden gelmiyorsa klavye verisidir
        if (!(inb(0x64) & 0x20)) {
            return inb(0x60);
        }
    }
    return 0; 
}

void pci_scan(void) {
    // PCI Tarama gövdesi (İçi şimdilik boş)
}

/* =========================================================
   ARAYÜZ VE KURULUM EKRANLARI (OOBE)
   ========================================================= */
void screen1(u8 key) {
    // Adım 1: Kullanıcı adı giriş ekranı mantığı buraya gelecek
}

void screen2(void) {
    // Adım 2: Bölge seçimi ekranı mantığı buraya gelecek
}

/* =========================================================
   ANA GİRİŞ NOKTASI (KERNEL MAIN)
   ========================================================= */
void kernel_main(multiboot_info_t* mbi){
    // Multiboot yapısından ekran kartı bellek adresini alıyoruz
    FB        = (u32*)(u32)mbi->framebuffer_addr;
    SCR_W     = mbi->framebuffer_width;
    SCR_H     = mbi->framebuffer_height;
    SCR_PITCH = mbi->framebuffer_pitch / 4;

    // Eğer GRUB veri geçemediyse varsayılan VESA Fallback moduna geç
    if(!FB || SCR_W == 0){
        FB        = (u32*)0xFD000000u;
        SCR_W     = 1024;
        SCR_H     = 768;
        SCR_PITCH = 1024;
    }

    // Donanım ilklendirmeleri
    mouse_init();
    pci_scan();

    // Sonsuz İşletim Sistemi Döngüsü
    while(1){
        mouse_poll();
        u8 key = kbd_poll();

        switch(state){
            case STATE_SETUP_1_NAME:    
                screen1(key);  
                break;
            case STATE_SETUP_2_REGION:  
                screen2();     
                break;
            case STATE_SETUP_3_KEYBOARD:
                break;
            case STATE_SETUP_4_NETWORK:
                break;
            case STATE_SETUP_5_PRIVACY:
                break;
            case STATE_SETUP_6_UPDATE:
                break;
            case STATE_SETUP_7_FINISH:
                break;
            case STATE_DESKTOP:
                break;
        }
    }
}
