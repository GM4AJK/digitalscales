
#include <stdio.h>
#include "main.h"
#include "ssd1306.h"
#include "fontx.h"
#include "hx711.h"

#define STR(x) (const uint8_t*)x,sizeof(x)-1

// Defined in main.c
extern I2C_HandleTypeDef hi2c1;
extern TIM_HandleTypeDef htim6;
extern UART_HandleTypeDef hlpuart1;

volatile static uint32_t millisecond_counter;

HX711_t hx711;

// Static function prototypes
static void splashscreen(void);



void app_init(void)
{
	millisecond_counter = 0;
	ssd1306_Init(&hi2c1);
	HAL_Delay(1000);
	ssd1306_Fill(Black);
	ssd1306_UpdateScreen(&hi2c1);
	HAL_Delay(1000);

	hx711.CLK_GPIO = HX711_CLK_GPIO_Port;
	hx711.CLK_Pin   = HX711_CLK_Pin;
	hx711.DATA_GPIO = HX711_DT_GPIO_Port;
	hx711.DATA_Pin  = HX711_DT_Pin;
	hx711_init(&hx711, HX711_GAIN_A_128);

	//fontx_DrawString("34.5", FONTX_CENTER_X, FONTX_CENTER_Y, White);
	//ssd1306_UpdateScreen(&hi2c1);

	HAL_TIM_Base_Start_IT(&htim6);

	HAL_UART_Transmit(&hlpuart1, STR("Startup\r\n"), HAL_MAX_DELAY);
}

// Throwaway: map 24-bit signed raw value to 0.0 – 99.9 for display testing.
// Remove when real load cell calibration is implemented.
static float raw_to_display(int32_t raw)
{
	if (raw < -8388608) raw = -8388608;
	if (raw >  8388607) raw =  8388607;
	return ((float)(raw + 8388608) / 16777215.0f) * 99.9f;
}



void app_loop(void)
{
	splashscreen();
	HAL_Delay(10000);
	ssd1306_Fill(Black);
	ssd1306_UpdateScreen(&hi2c1);

	for(;;) {
		int32_t raw;
		char buffer[8] = {};
		if (hx711_get_data(&hx711, &raw) == HAL_OK) {
			float val = raw_to_display(raw);
			sprintf(buffer, "%04.1f", val);
			fontx_DrawString(buffer, FONTX_CENTER_X, FONTX_CENTER_Y, White);
			ssd1306_UpdateScreen(&hi2c1);
		}
	}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim == &htim6) {
		++millisecond_counter;
	}
}

static void drawboarder(void)
{
	for(uint8_t i=2; i < 126; i++) {
		ssd1306_DrawPixel(i, 0, White);
		ssd1306_DrawPixel(i, 1, White);
		ssd1306_DrawPixel(i, 2, White);
		ssd1306_DrawPixel(i, 61, White);
		ssd1306_DrawPixel(i, 62, White);
		ssd1306_DrawPixel(i, 63, White);
	}
	for(uint8_t i=2; i < 62; i++) {
		ssd1306_DrawPixel(1, i, White);
		ssd1306_DrawPixel(2, i, White);
		ssd1306_DrawPixel(3, i, White);
		ssd1306_DrawPixel(125, i, White);
		ssd1306_DrawPixel(126, i, White);
		ssd1306_DrawPixel(127, i, White);
	}
	ssd1306_UpdateScreen(&hi2c1);
}

static void splashscreen(void)
{
	uint8_t x = 18;
	uint8_t y = 15;
	FontDef f = Font_7x10;
	const char *txt[4] = {
		" Digi Scales\0",
		"     for    \0",
		" Riley & Max\0",
		"\0"
	};
	ssd1306_SetCursor(x, y);
	ssd1306_WriteString(txt[0], f, White);
	y += 12;
	ssd1306_SetCursor(x, y);
	ssd1306_WriteString(txt[1], f, White);
	y += 12;
	ssd1306_SetCursor(x, y);
	ssd1306_WriteString(txt[2], f, White);
	y += 12;
	//ssd1306_SetCursor(x, y);
	//ssd1306_WriteString(txt[3], f, White);
	ssd1306_UpdateScreen(&hi2c1);
	drawboarder();
}
