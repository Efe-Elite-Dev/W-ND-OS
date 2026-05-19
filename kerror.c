#include "sky_core.h"

void kpanic(uint8_t error_code, const char* message) {
    (void)error_code;
    (void)message;
    
    // HATA BURADAYDI: FRAMEBUFFER -> GRAPHICS_FRAMEBUFFER oldu
    if(GRAPHICS_FRAMEBUFFER) {
        // Ekranın sol üstünde kırmızı bir kare çizerek hata olduğunu belli et
        for(int i = 0; i < 100; i++) GRAPHICS_FRAMEBUFFER[i] = 0xFFFF0000;
    }
    
    while(1); // Sonsuz döngü (Kernel paniği)
}
