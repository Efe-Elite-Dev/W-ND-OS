#include "io.h"
#include "gui.h"

bool ai_hud_visible = false;
static bool alt_pressed = false;

void keyboard_handler() {
    uint8_t scancode = inb(0x60);

    // Alt Tuşu Durum Kontrolü (Scancode 0x38 = ALT basıldı, 0xB8 = ALT bırakıldı)
    if (scancode == 0x38) {
        alt_pressed = true;
    } else if (scancode == 0xB8) {
        alt_pressed = false;
    }

    // Space Tuşu Kontrolü (Scancode 0x39 = SPACE basıldı)
    if (scancode == 0x39 && alt_pressed) {
        // ALT + SPACE Kombinasyonu Yakalandı!
        ai_hud_visible = !ai_hud_visible; // HUD Panelini aç/kapat
    }
}
