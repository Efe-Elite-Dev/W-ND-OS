#include "wind_subsystem.h"
#include "ai_subsystem.h"

/* Rüzgar Alt Sistemi Global Durum Değişkenleri */
static int system_initialized = 0;
static uint32_t system_uptime_ticks = 0;

/**
 * @brief Rüzgar Alt Sistemini (Wind Subsystem) ilk fırlatan ana motor.
 */
void init_wind_subsystem(void) {
    system_initialized = 1;
    system_uptime_ticks = 0;
}

/**
 * @brief Alt sistemin kalbini vuran, her PIT kesmesinde tetiklenen rutin döngü.
 */
void wind_subsystem_tick(void) {
    if (!system_initialized) return;
    system_uptime_ticks++;
}

/**
 * @brief Rüzgar alt sisteminin mevcut çalışma zamanı tik değerini döner.
 */
uint32_t get_wind_subsystem_uptime(void) {
    return system_uptime_ticks;
}
