#include "globals.h"
#include "gui.h"

// Varsayılan sanal makine LFB adresi (Aşağıda GRUB'dan gelen gerçek adrese eşitlenecek)
uint32_t* gfx_framebuffer = (uint32_t*)0xE0000000; 

SystemState current_state = STATE_WELCOME;
bool ai_hud_visible = false;
SetupData os_setup_data;

void kernel_main(uint32_t magic, uint32_t* mbi) {
    // GRUB üzerinden başarıyla boot edildiyse ve multiboot yapısı geçerliyse
    if (magic == 0x2BADB002 && mbi != NULL) {
        
        // Multiboot flags (mbi[0]) içindeki 12. bit set edilmiş mi? (Grafik tablosu var mı kontrolü)
        if (mbi[0] & (1 << 12)) {
            
            // HATA DÜZELTMESİ: mbi[18] yerine gerçek LFB adresi olan mbi[22]'yi (Offset 88) okuyoruz.
            // Böylece bellek taşması ve text-mode çakışması tamamen engelleniyor.
            uint32_t real_fb = mbi[22]; 
            if (real_fb != 0) {
                gfx_framebuffer = (uint32_t*)real_fb;
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
