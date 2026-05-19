#include "globals.h"
#include "gui.h"

// Varsayılan güvenli LFB adresi
uint32_t* gfx_framebuffer = (uint32_t*)0xE0000000; 

SystemState current_state = STATE_WELCOME;
bool ai_hud_visible = false;
SetupData os_setup_data;

void kernel_main(uint32_t magic, uint32_t* mbi) {
    if (magic == 0x2BADB002 && mbi != NULL) {
        
        // Multiboot flags içindeki grafik tablosu bitini kontrol et
        if (mbi[0] & (1 << 12)) {
            
            // mbi[27] yapısının ikinci byte'ı (bits 8-15) bize framebuffer tipini verir.
            // 1 = Gerçek Grafik Modu (LFB), 2 = EGA Text Modu.
            uint8_t fb_type = (mbi[27] >> 8) & 0xFF;
            
            // GÜVENLİK KORUMASI: Sadece GRUB başarıyla grafik modunu açtıysa adresi eşitle!
            if (fb_type == 1) {
                uint32_t real_fb = mbi[22]; 
                if (real_fb != 0) {
                    gfx_framebuffer = (uint32_t*)real_fb;
                }
            }
        }
    }

    init_vga();
    os_setup_data.wifi_connected = false;

    while (true) {
        switch (current_state) {
            case STATE_WELCOME:
                draw_setup_welcome();
                break;
            case STATE_LOCATION:
                draw_setup_location();
                break;
            case STATE_COMPLETING:
                draw_setup_completing();
                break;
            case STATE_DESKTOP:
                draw_main_desktop();
                break;
        }

        if (ai_hud_visible) {
            draw_ai_subsystem_hud();
        }
    }
}
