#ifndef KERNEL_H
#define KERNEL_H

/* Eğer projenizde multiboot.h varsa dahil edin, yoksa struct tanımını buraya ekleyin */
#include "multiboot.h" 

/* Temel Tip Tanımlamaları */
typedef unsigned int      u32;
typedef unsigned short    u16;
typedef unsigned char     u8;
typedef int               i32;
typedef signed char       i8;

#define NULL ((void*)0)

/* OS Durum Makinesi Enum Tanımı (state değişkeninden önce olmak zorunda) */
typedef enum {
    STATE_SETUP_1_NAME,
    STATE_SETUP_2_REGION,
    STATE_SETUP_3_KEYBOARD,
    STATE_SETUP_4_NETWORK,
    STATE_SETUP_5_PRIVACY,
    STATE_SETUP_6_CUSTOMIZE,
    STATE_SETUP_7_WELCOME,
    STATE_DESKTOP
} OS_State;

/* Renk Makroları */
#define C_BG          0xFFF3F3F5u
#define C_WHITE       0xFFFFFFFFu
#define C_BLACK       0xFF000000u
#define C_BLUE        0xFF0078D4u
// ... diğer tüm #define renk tanımlarınız ...

/* Fonksiyon Prototipleri (Sadece bildirim, süslü parantez ve gövde YOK) */
void put_pixel(i32 x, i32 y, u32 color);
void fill_rect(i32 x, i32 y, i32 w, i32 h, u32 color);
void fill_rrect(i32 x, i32 y, i32 w, i32 h, i32 r, u32 color);
void draw_char(i32 x, i32 y, char c, u32 color);
void draw_string(i32 x, i32 y, const char* str, u32 color);
void mouse_init(void);
void mouse_poll(void);
u8 kbd_poll(void);
i32 draw_button(i32 x, i32 y, i32 w, i32 h, const char* text, u8 is_blue, u8 click);
void draw_setup_bg(const char* title, i32 step);
void screen_setup_1(u8 key, u8 click);
void screen_setup_2(u8 click);
void screen_setup_3(u8 click);
void screen_setup_4(u8 click);
void screen_setup_5(u8 click);
void screen_setup_6(u8 click);
void screen_setup_7(u8 click);
void screen_desktop(u8 click);
void draw_mouse(void);
void kernel_main(multiboot_info_t* mbi);

#endif /* KERNEL_H */
