/* =========================================================
 * EKSİK SYMBOL FIXLERI
 * ========================================================= */

/* Grafik donanım stub */
void force_graphics_hardware(void)
{
    /* şimdilik boş */
}

/* AI HUD global değişkeni */
int ai_hud_visible = 0;

/* setup.c stub */
void screen_init(void) {}
void setup_init(void) {}
void setup_handle_input(uint8_t sc)
{
    (void)sc;
}

/* diğer subsystem stub */
void keyboard_init(void) {}
void wind_subsystem_init(void) {}
void ai_subsystem_init(void) {}
