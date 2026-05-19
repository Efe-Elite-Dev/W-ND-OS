#ifndef SETUP_UI_H
#define SETUP_UI_H

typedef struct {
    char username[64];
    int selected_region; // 0: Türkiye, 1: Diğer
    bool wifi_enabled;
} SetupData;

void draw_setup_screen(int step, SetupData *data);
void handle_mouse_click(int x, int y, int step, SetupData *data);

#endif
