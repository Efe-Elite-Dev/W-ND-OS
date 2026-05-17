#ifndef WIND_SUBSYSTEM_H
#define WIND_SUBSYSTEM_H

#include <stdint.h>

/* GRUB'ın kernel.c'ye yolladığı Orijinal Multiboot Donanım Yapısı (%100 Korundu) */
struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t num;
    uint32_t size;
    uint32_t addr;
    uint32_t shndx;
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t  framebuffer_bpp;
    uint8_t  framebuffer_type;
};

/* Ana Giriş Noktası Tanımı (Linker ve Derleyici Uyumu İçin) */
void kernel_main(void* mboot_ptr);

/* Donanım Port Köprüleri */
void outb(unsigned short port, unsigned char val);
unsigned char inb(unsigned short port);

/* Grafik ve Ekran Fonksiyonları */
void init_graph_mode(void);
void draw_pixel_pure(int x, int y, uint32_t color);
void clear_screen_gfx(uint32_t color);
void swap_buffers(void);

/* KRİTİK DÜZELTME: exe_subsystem.c'nin aradığı 5 parametreli nihai standart! */
void draw_window_pure(int x, int y, int width, int height, uint32_t border_color);

/* Altsistemler */
void clear_text_screen(void);
void init_idt(void);
void init_keyboard(void);
void init_mouse(void);
void handle_mouse_polling(void);
void check_keyboard_pure(void);
void gui_refresh_desktop(void);
void run_exe_subsystem(void);

/* AI Modülleri - C Dosyalarıyla Tam Senkronize Parametreler */
int ai_mouse_analyze_stress(void);
int ai_keyboard_analyze_cadence(void);
void ai_core_predict_scheduler(int predicted_load, int anomaly_flag, int policy);

#endif
