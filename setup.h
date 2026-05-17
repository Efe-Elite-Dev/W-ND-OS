#ifndef SETUP_H
#define SETUP_H

#include <stdint.h>

typedef enum {
    STAGE_COUNTRY,
    STAGE_KEYBOARD,
    STAGE_NETWORK,
    STAGE_HOSTNAME,
    STAGE_CUSTOMIZE,
    STAGE_COMPLETE
} SetupStage;

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 200
#define VIDEO_MEMORY  ((uint8_t*)0xA0000)

#define COLOR_WIND_BG        16
#define COLOR_WIND_CARD      17
#define COLOR_WIND_TEXT      18
#define COLOR_WIND_PRIMARY   19

void setup_init(void);
void setup_render(void);
void setup_handle_input(uint8_t scancode);

#endif
