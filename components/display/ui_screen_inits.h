#ifndef UI_SCREEN_INTIS_H
#define UI_SCREEN_INITS_H

// Functions called to send strings to screen
void startup_screen_init();
void temp_humid_screen_init();
void co2_screen_init();
void voc_screen_init();
void set_co2_thresh_screen_init();
void set_voc_thresh_screen_init(); 
void error_screen_init();
void set_powering_down_screen();

#endif // UI_SCREEN_INITS_H