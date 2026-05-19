#include <stdint.h>
#include <stddef.h>

volatile uint16_t* VGA = (uint16_t*)0xB8000;

static int cursor_x = 0;
static int cursor_y = 0;

void putc(char c)
{
    if (c == '\n')
    {
        cursor_x = 0;
        cursor_y++;
        return;
    }

    VGA[cursor_y * 80 + cursor_x] =
        (0x0F << 8) | c;

    cursor_x++;

    if (cursor_x >= 80)
    {
        cursor_x = 0;
        cursor_y++;
    }
}

void print(const char* s)
{
    while (*s)
    {
        putc(*s);
        s++;
    }
}

/* STUBLAR */

void force_graphics_hardware(void) {}
int ai_hud_visible = 0;

void screen_init(void) {}
void setup_init(void) {}

void setup_handle_input(uint8_t sc)
{
    (void)sc;
}

void keyboard_init(void) {}
void wind_subsystem_init(void) {}
void ai_subsystem_init(void) {}

/* KERNEL */

void kernel_main(uint32_t magic, void* mboot_ptr)
{
    (void)mboot_ptr;

    print("WIND OS v1.5\n");
    print("Kernel basladi\n");

    if (magic == 0x2BADB002)
    {
        print("GRUB OK\n");
    }
    else
    {
        print("GRUB FAIL\n");
    }

    while (1)
    {
        __asm__ volatile("hlt");
    }
}
