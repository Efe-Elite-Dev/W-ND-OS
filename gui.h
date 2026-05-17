#ifndef GUI_H
#define GUI_H

#include <stdint.h>

/* =============================================================================
   MERKEZİ GRAFİK KULLANICI ARAYÜZÜ (GUI) PROTOTİPLERİ
   ============================================================================= */

/**
 * @brief Masaüstü arka plan grafiklerini ve temel GUI bileşenlerini tazeler.
 * Çift arabellekleme (Double Buffering) mekanizmasını besler.
 */
void gui_refresh_desktop(void);

/**
 * @brief Sky OS / Wind OS standartlarında saf ve güvenli bir pencere çizer.
 * @param x Pencerenin sol üst X koordinatı
 * @param y Pencerenin sol üst Y koordinatı
 * @param width Pencerenin genişliği (piksel cinsinden)
 * @param height Pencerenin yüksekliği (piksel cinsinden)
 * @param border_color Pencerenin çerçeve ve başlık çubuğu rengi (32-bit ARGB/XRGB)
 */
void draw_window_pure(int x, int y, int width, int height, uint32_t border_color);

#endif /* GUI_H */
