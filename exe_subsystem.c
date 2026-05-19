#include "sky_core.h"

void sky_execute_file(char* filename) {
    // Dosya uzantısını kontrol et
    if (strstr(filename, ".exe")) {
        // PE formatını işle
    } else if (strstr(filename, ".deb")) {
        // Deb paketi aç
    } else if (strstr(filename, ".sky")) {
        // Kendi formatını işle
    }
}
