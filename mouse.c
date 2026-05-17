#include "wind_subsystem.h"

/* Fare Donanım Portları ve Durum Tanımlamaları */
#define MOUSE_DATA_PORT 0x60
#define MOUSE_STATUS_PORT 0x64

static int mouse_x_position = 400;
static int mouse_y_position = 300;

/**
 * @brief Fare sürücüsünü ve ekran üzerindeki başlangıç koordinatlarını hazırlar.
 */
void init_mouse(void) {
    mouse_x_position = 400;
    mouse_y_position = 300;
    
    // Fare donanımını etkinleştirmek için (PS/2) komut dizilimleri ileride buraya genişletilebilir
}

/**
 * @brief Fare hareket verilerini port üzerinden sorgulayarak (polling) yakalar.
 */
void handle_mouse_polling(void) {
    uint8_t status = inb(MOUSE_STATUS_PORT);
    
    if (status & 0x01) {
        int8_t delta_x = (int8_t)inb(MOUSE_DATA_PORT);
        
        mouse_x_position += delta_x;
        if (mouse_x_position < 0) mouse_x_position = 0;
        if (mouse_x_position > 799) mouse_x_position = 799;
    }
}

/**
 * @brief Güncel fare X koordinatını döner.
 */
int get_mouse_x(void) {
    return mouse_x_position;
}

/**
 * @brief Güncel fare Y koordinatını döner.
 */
int get_mouse_y(void) {
    return mouse_y_position;
}
