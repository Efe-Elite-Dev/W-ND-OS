#ifndef SKY_CORE_H
#define SKY_CORE_H

#include "io.h"
#include "gui.h"
#include "mouse.h"
#include "ai_subsystem.h"
#include "wind_subsystem.h"
#include "exe_subsystem.h"

// Sistem durumu
typedef enum { BOOT, SETUP, DESKTOP, LOADING_APP } SystemState;
extern SystemState current_state;

// Merkezi sistem çağrısı
void sky_execute_file(char* filename); 
void sky_switch_state(SystemState new_state);

#endif
