#ifndef INC_FONTX_H_
#define INC_FONTX_H_

#include <stdint.h>
#include "ssd1306.h"

// 30x56 pixel 7-segment style font for digits '0'-'9' and '.'
// Four characters ("XX.X") fills 120px, centred on a 128px wide display.
#define FONTX_WIDTH   30
#define FONTX_HEIGHT  56

// Convenience: top-left corner to centre "XX.X" on a 128x64 display
#define FONTX_CENTER_X  4   // (128 - 4*30) / 2
#define FONTX_CENTER_Y  4   // ( 64 - 56)   / 2

// Draw a single character ('0'-'9' or '.') into the screen buffer at (x, y).
// Returns the x coordinate of the next character position.
uint8_t fontx_DrawChar(char ch, uint8_t x, uint8_t y, SSD1306_COLOR color);

// Draw a null-terminated string of scale characters into the screen buffer.
void fontx_DrawString(const char *str, uint8_t x, uint8_t y, SSD1306_COLOR color);

#endif /* INC_FONTX_H_ */
