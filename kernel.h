#ifndef KERNEL_H
#define KERNEL_H

// Görseldeki 7 Kurulum Ekranı ve Masaüstü Durum Yapısı
typedef enum {
    STATE_SETUP_1_NAME = 0,    // Bilgisayarınıza bir ad verin
    STATE_SETUP_2_REGION,      // Bu doğru ülke/bölge mi?
    STATE_SETUP_3_KEYBOARD,    // Bu doğru klavye düzeni?
    STATE_SETUP_4_NETWORK,     // Hadi sizi bir ağa bağlayalım
    STATE_SETUP_5_PRIVACY,     // Cihazınız için gizlilik ayarlarını seçin
    STATE_SETUP_6_CUSTOMIZE,   // Deneyiminizi özelleştirin
    STATE_SETUP_7_WELCOME,     // Hoş Geldiniz / Sisteme Giriş
    STATE_DESKTOP              // 8. Ekran: Masaüstü (Hava Durumu, Uygulamalar, Saat)
} OS_State;

// GRUB'dan gelen ekran kartı (Framebuffer) bilgilerini okumak için Multiboot yapısı
typedef struct {
    unsigned int flags;
    unsigned int mem_lower;
    unsigned int mem_upper;
    unsigned int boot_device;
    unsigned int cmdline;
    unsigned int mods_count;
    unsigned int mods_addr;
    unsigned int num;
    unsigned int size;
    unsigned int addr;
    unsigned int shndx;
    unsigned int mmap_length;
    unsigned int mmap_addr;
    unsigned int drives_length;
    unsigned int drives_addr;
    unsigned int config_table;
    unsigned int boot_loader_name;
    unsigned int apm_table;
    unsigned int vbe_control_info;
    unsigned int vbe_mode_info;
    unsigned short vbe_mode;
    unsigned short vbe_interface_seg;
    unsigned short vbe_interface_off;
    unsigned short vbe_interface_len;
    
    // Grafik Modu için kritik adresler (Linear Framebuffer)
    unsigned int framebuffer_addr; 
    unsigned int framebuffer_pitch;
    unsigned int framebuffer_width;
    unsigned int framebuffer_height;
    unsigned char framebuffer_bpp;
    unsigned char framebuffer_type;
} __attribute__((packed)) multiboot_info_t;

#endif
