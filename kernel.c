#include "globals.h"
#include "gui.h"

void kernel_main() {
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
