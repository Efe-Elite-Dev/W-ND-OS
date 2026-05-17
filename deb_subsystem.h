#ifndef DEB_SUBSYSTEM_H
#define DEB_SUBSYSTEM_H

#include <stdint.h>

/* =============================================================================
   DEB PAKET YÖNETİM ALT SİSTEMİ PROTOTİPLERİ
   ============================================================================= */

/**
 * @brief Sistem içinde tanımlı olan veya hafızaya yüklenen .deb paketlerini
 * çözerek içerisindeki ikili (binary) verileri ve meta dataları işler.
 */
void process_deb_package(void);

#endif /* DEB_SUBSYSTEM_H */
