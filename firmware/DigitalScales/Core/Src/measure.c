/*
 * measure.c
 *
 *  Created on: 10 May 2026
 *      Author: kirkh
 */

#include <stdio.h>

#include "ssd1306.h"
#include "fontx.h"
#include "measure.h"
#include "app.h"

static I2C_HandleTypeDef *p_display = NULL;

// Throwaway: map 24-bit signed raw value to 0.0 – 99.9 for display testing.
// Remove when real load cell calibration is implemented.
static float raw_to_display(int32_t raw)
{
	if (raw < -8388608) raw = -8388608;
	if (raw >  8388607) raw =  8388607;
	return ((float)(raw + 8388608) / 16777215.0f) * 99.9f;
}

void measure_set_i2c(I2C_HandleTypeDef *p_hi2c)
{
	p_display = p_hi2c;
}

static void display_measurement_value(float val)
{
	char buffer[8] = {0};
	sprintf(buffer, "%04.1f", val);
	buffer[4] = '\0';
	fontx_DrawString(buffer, FONTX_CENTER_X, FONTX_CENTER_Y, White);
	ssd1306_UpdateScreen(p_display);
}

void measure(int32_t measurement)
{
	float val = raw_to_display(measurement);
	display_measurement_value(val);
}


