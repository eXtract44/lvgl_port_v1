/*
 * ui_theme.h
 *
 *  Created on: 16.05.2026
 *      Author: toose
 */

#ifndef MAIN_USER_UI_UI_THEME_H_
#define MAIN_USER_UI_UI_THEME_H_



#include "ui_core.h"

void init_fonts(ui_main_menu_t *ui);
void init_styles(ui_main_menu_t *ui);

void apply_theme_dark(ui_main_menu_t *ui);
void apply_theme_light(ui_main_menu_t *ui);
void ui_apply_theme(ui_main_menu_t *ui);


#endif /* MAIN_USER_UI_UI_THEME_H_ */
