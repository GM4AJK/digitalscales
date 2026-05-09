#include "fontx.h"

// ---------------------------------------------------------------------------
// 30-pixel wide, 56-pixel tall 7-segment style bitmap font.
//
// Bit layout: each uint32_t encodes one row, MSB = leftmost pixel (col 0).
// Only bits 31..2 are used (cols 0..29); bits 1..0 are always 0.
//
// A 2-pixel blank margin is kept on all four sides of each glyph, so the
// drawn segment occupies cols 2-27 and rows 2-53.
//
// Segment geometry (0-indexed cols/rows within the 30x56 cell):
//   A  top horizontal : cols  6-23  (rows  2- 5)
//   F  upper-left     : cols  2- 5  (rows  6-27)
//   B  upper-right    : cols 24-27  (rows  6-27)
//   G  middle horiz   : cols  6-23  (rows 26-29)
//   E  lower-left     : cols  2- 5  (rows 28-49)
//   C  lower-right    : cols 24-27  (rows 28-49)
//   D  bot horizontal : cols  6-23  (rows 50-53)
//
// Bit masks per row region (col c occupies bit 31-c):
//   SEG_H  = 0x03FFFF00  cols  6-23  (horizontal A / G / D)
//   SEG_L  = 0x3C000000  cols  2- 5  (left  vert F / E)
//   SEG_R  = 0x000000F0  cols 24-27  (right vert B / C)
//   BV     = 0x3C0000F0  both verticals
//   LH     = 0x3FFFFF00  left  vert + horizontal
//   RH     = 0x03FFFFF0  right vert + horizontal
//   BVH    = 0x3FFFFFF0  both verts + horizontal
//   DOT    = 0x001FE000  decimal dot, cols 11-18
// ---------------------------------------------------------------------------

// Expander macros — produce N copies of value v as initialiser list elements.
#define R2(v)  (v),(v)
#define R4(v)  R2(v),R2(v)
#define R8(v)  R4(v),R4(v)
#define R16(v) R8(v),R8(v)
#define R20(v) R16(v),R4(v)

// Row-group layout per character (total = 56):
//   R2  rows  0- 1  top blank margin
//   R4  rows  2- 5  top horizontal (A) or blank
//   R20 rows  6-25  upper verticals (F+B or subset)
//   R2  rows 26-27  upper vert / G overlap zone
//   R2  rows 28-29  lower vert / G overlap zone
//   R20 rows 30-49  lower verticals (E+C or subset)
//   R4  rows 50-53  bottom horizontal (D) or blank
//   R2  rows 54-55  bottom blank margin

// Characters '.' (ASCII 46) through '9' (ASCII 57) — 12 entries, 56 rows each.
static const uint32_t FontX_Data[12 * 56] = {

    // '.' — decimal point dot, centred cols 11-18, rows 46-53
    R2(0x00000000), R4(0x00000000), R20(0x00000000),
    R2(0x00000000), R2(0x00000000),
    R16(0x00000000), R4(0x001FE000),
    R4(0x001FE000), R2(0x00000000),

    // '/' — unused placeholder, all dark
    R2(0x00000000), R4(0x00000000), R20(0x00000000),
    R2(0x00000000), R2(0x00000000),
    R20(0x00000000), R4(0x00000000), R2(0x00000000),

    // '0' — A B C D E F
    R2(0x00000000), R4(0x03FFFF00), R20(0x3C0000F0),
    R2(0x3C0000F0), R2(0x3C0000F0),
    R20(0x3C0000F0), R4(0x03FFFF00), R2(0x00000000),

    // '1' — B C
    R2(0x00000000), R4(0x00000000), R20(0x000000F0),
    R2(0x000000F0), R2(0x000000F0),
    R20(0x000000F0), R4(0x00000000), R2(0x00000000),

    // '2' — A B G E D
    R2(0x00000000), R4(0x03FFFF00), R20(0x000000F0),
    R2(0x03FFFFF0), R2(0x3FFFFF00),
    R20(0x3C000000), R4(0x03FFFF00), R2(0x00000000),

    // '3' — A B G C D
    R2(0x00000000), R4(0x03FFFF00), R20(0x000000F0),
    R2(0x03FFFFF0), R2(0x03FFFFF0),
    R20(0x000000F0), R4(0x03FFFF00), R2(0x00000000),

    // '4' — F G B C
    R2(0x00000000), R4(0x00000000), R20(0x3C0000F0),
    R2(0x3FFFFFF0), R2(0x03FFFFF0),
    R20(0x000000F0), R4(0x00000000), R2(0x00000000),

    // '5' — A F G C D
    R2(0x00000000), R4(0x03FFFF00), R20(0x3C000000),
    R2(0x3FFFFF00), R2(0x03FFFFF0),
    R20(0x000000F0), R4(0x03FFFF00), R2(0x00000000),

    // '6' — A F G E C D
    R2(0x00000000), R4(0x03FFFF00), R20(0x3C000000),
    R2(0x3FFFFF00), R2(0x3FFFFFF0),
    R20(0x3C0000F0), R4(0x03FFFF00), R2(0x00000000),

    // '7' — A B C
    R2(0x00000000), R4(0x03FFFF00), R20(0x000000F0),
    R2(0x000000F0), R2(0x000000F0),
    R20(0x000000F0), R4(0x00000000), R2(0x00000000),

    // '8' — A B C D E F G
    R2(0x00000000), R4(0x03FFFF00), R20(0x3C0000F0),
    R2(0x3FFFFFF0), R2(0x3FFFFFF0),
    R20(0x3C0000F0), R4(0x03FFFF00), R2(0x00000000),

    // '9' — A B C D F G
    R2(0x00000000), R4(0x03FFFF00), R20(0x3C0000F0),
    R2(0x3FFFFFF0), R2(0x03FFFFF0),
    R20(0x000000F0), R4(0x03FFFF00), R2(0x00000000),
};

uint8_t fontx_DrawChar(char ch, uint8_t x, uint8_t y, SSD1306_COLOR color)
{
	if (ch < '.' || ch > '9') {
		return x + FONTX_WIDTH;
	}

	const uint32_t *bitmap = &FontX_Data[(ch - '.') * FONTX_HEIGHT];

	for (uint8_t row = 0; row < FONTX_HEIGHT; row++) {
		uint32_t bits = bitmap[row];
		for (uint8_t col = 0; col < FONTX_WIDTH; col++) {
			SSD1306_COLOR px = ((bits << col) & 0x80000000UL)
			                   ? color
			                   : (SSD1306_COLOR)!color;
			ssd1306_DrawPixel(x + col, y + row, px);
		}
	}

	return x + FONTX_WIDTH;
}

void fontx_DrawString(const char *str, uint8_t x, uint8_t y, SSD1306_COLOR color)
{
	while (*str) {
		x = fontx_DrawChar(*str++, x, y, color);
	}
}
