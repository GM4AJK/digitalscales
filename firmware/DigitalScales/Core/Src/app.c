
#include <stdio.h>
#include <stdbool.h>
#include "main.h"
#include "app.h"
#include "ssd1306.h"
#include "fontx.h"
#include "hx711.h"
#include "measure.h"

#define STR(x) (const uint8_t*)x,sizeof(x)-1

typedef enum {
	APP_STATE_STARTUP = 0,
	APP_STATE_SPLASHSCREEN_BEGIN,
	APP_STATE_SPLASHSCREEN_CONTINUE,
	APP_STATE_MEASURING,
	APP_STATE_LAST
} APP_STATE_e;

volatile APP_STATE_e app_state;
volatile uint32_t dncnt_arr[DNCNT_LAST];

// Defined in main.c
extern I2C_HandleTypeDef hi2c1;
extern TIM_HandleTypeDef htim6;
extern UART_HandleTypeDef hlpuart1;

volatile static uint32_t millisecond_counter;

HX711_t hx711;

// Static function prototypes
static void splashscreen(void);
static void SH_startup(void);
static void SH_splashscreen_begin(void);
static void SH_splashscreen_continue(void);
static void SH_measuring(void);
static inline APP_STATE_e SM_set(APP_STATE_e val);
static inline APP_STATE_e SM_get();
static void display_clear(SSD1306_COLOR color);

static void app_reinit(void)
{
	millisecond_counter = 0;
	for(int i = 0; i < DNCNT_LAST; i++) {
		dncnt_set((DNCNT_e)i, 0);
	}

	ssd1306_Init(&hi2c1);
	HAL_Delay(100);
	ssd1306_Fill(Black);
	ssd1306_UpdateScreen(&hi2c1);
	HAL_Delay(100);

	hx711.CLK_GPIO = HX711_CLK_GPIO_Port;
	hx711.CLK_Pin   = HX711_CLK_Pin;
	hx711.DATA_GPIO = HX711_DT_GPIO_Port;
	hx711.DATA_Pin  = HX711_DT_Pin;
	hx711_init(&hx711, HX711_GAIN_A_128);
	measure_set_i2c(&hi2c1);
	HAL_UART_Transmit(&hlpuart1, STR("DigiScales v1.0\r\nStartup\r\n"), HAL_MAX_DELAY);
}

void app_init(void)
{
	app_state = APP_STATE_STARTUP;
	HAL_TIM_Base_Start_IT(&htim6);
}

/**
 * Main application loop.
 * This function handles the state machine. All states should be handled
 * by a static state machine handler function SM_x().
 */
void app_loop(void)
{
	for(;;) {
		switch(app_state) {
		default:
		case APP_STATE_STARTUP:
			SH_startup();
			break;
		case APP_STATE_SPLASHSCREEN_BEGIN:
			SH_splashscreen_begin();
			break;
		case APP_STATE_SPLASHSCREEN_CONTINUE:
			SH_splashscreen_continue();
			break;
		case APP_STATE_MEASURING:
			SH_measuring();
			break;
		}
	}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim == &htim6) {
		++millisecond_counter;
		for(int i = 0; i < DNCNT_LAST; i++) {
			if(dncnt_arr[i] > 0) {
				--dncnt_arr[i];
			}
		}
	}
}

static void SH_startup(void)
{
	app_reinit();
	SM_set(APP_STATE_SPLASHSCREEN_BEGIN);
}

static void SH_splashscreen_begin(void)
{
	splashscreen();
	dncnt_set(DNCNT_SHARED, 10000); // Set a timer...
	SM_set(APP_STATE_SPLASHSCREEN_CONTINUE); // ...and wait for it to time out.
}

static void SH_splashscreen_continue(void)
{
	if(dncnt_timedout(DNCNT_SHARED)) {
		display_clear(Black);
		SM_set(APP_STATE_MEASURING);
	}
}

static void SH_measuring(void)
{
	int32_t raw;
	if (hx711_get_data(&hx711, &raw) == HAL_OK) {
		measure(raw);
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

static inline APP_STATE_e SM_set(APP_STATE_e val) {
	APP_STATE_e prev = app_state;
	app_state = val;
	return prev;
}
static inline APP_STATE_e SM_get() {
	return app_state;
}

bool dncnt_timedout(DNCNT_e i) {
	return dncnt_get(i) == 0;
}

uint32_t dncnt_get(DNCNT_e i) {
	return dncnt_arr[i];
}

uint32_t dncnt_set(DNCNT_e i, uint32_t val) {
	uint32_t v = dncnt_arr[i];
	dncnt_arr[i] = val;
	return v;
}

static void display_clear(SSD1306_COLOR color) {
	ssd1306_Fill(color);
	ssd1306_UpdateScreen(&hi2c1);
}

