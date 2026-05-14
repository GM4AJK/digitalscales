
#include <stdio.h>
#include <stdbool.h>
#include "main.h"
#include "app.h"
#include "ssd1306.h"
#include "fontx.h"
#include "hx711.h"
#include "measure.h"

// ToDo, we need to calibrate the real COUNTS_PER_KG value, below is an estimate.
#define COUNTS_PER_KG  84000.0f
#define INHIBIT_RETARE_WEIGHT 0.5f
#define RETARE_TIME (5*60*1000)
#define RETARE_RETRY_MAX 5
#define STR(x) (const uint8_t*)x,sizeof(x)-1

typedef enum {
	APP_STATE_STARTUP = 0,
	APP_STATE_SPLASHSCREEN_BEGIN,
	APP_STATE_SPLASHSCREEN_CONTINUE,
	APP_STATE_CALIBRATION_BEGIN,
	APP_STATE_CALIBRATION_CONTINUE,
	APP_STATE_MEASURING_BEGIN,
	APP_STATE_MEASURING_CONTINUE,
	APP_STATE_POWER_DOWN_BEGIN,
	APP_STATE_POWER_DOWN_CONTINUE,
	APP_STATE_LAST
} APP_STATE_e;

volatile APP_STATE_e app_state;
volatile uint32_t dncnt_arr[DNCNT_LAST];

static float current_weight;
static int auto_tare_retry_counter;

// Defined in main.c
extern I2C_HandleTypeDef hi2c1;
//extern TIM_HandleTypeDef htim6;
extern UART_HandleTypeDef hlpuart1;

volatile static uint64_t millisecond_counter;

HX711_t hx711;

measure_t measure;

// Static function prototypes
static void drawboarder(void);
static void zero_display(void);
static void splashscreen(void);
static void powerdownscreen(void);
static void calibrationscreen(void);

static void SH_startup(void);
static void SH_splashscreen_begin(void);
static void SH_splashscreen_continue(void);
static void SH_calibration_begin(void);
static void SH_calibration_continue(void);
static void SH_measuring_begin(void);
static void SH_measuring_continue(void);
static void SH_power_down_begin(void);
static void SH_power_down_continue(void);
static inline APP_STATE_e SM_set(APP_STATE_e val);
static inline APP_STATE_e SM_get();
static void display_clear(SSD1306_COLOR color);

static void app_reinit(void)
{
	millisecond_counter = 0;
	for(int i = 0; i < DNCNT_LAST; i++) {
		dncnt_set((DNCNT_e)i, 0);
	}
	current_weight = 0.0f;
	auto_tare_retry_counter = 0;

	ssd1306_Init(&hi2c1);
	HAL_Delay(100);
	display_clear(Black);
	HAL_Delay(100);
	hx711.CLK_GPIO = HX711_CLK_GPIO_Port;
	hx711.CLK_Pin   = HX711_CLK_Pin;
	hx711.DATA_GPIO = HX711_DT_GPIO_Port;
	hx711.DATA_Pin  = HX711_DT_Pin;
	hx711_init(&hx711, HX711_GAIN_A_128);
	measure_init(&measure);
	//HAL_UART_Transmit(&hlpuart1, STR("DigiScales v1.0\r\nStartup\r\n"), HAL_MAX_DELAY);
}

void app_init(void)
{
	app_state = APP_STATE_STARTUP;
	current_weight = 0.0f;
	auto_tare_retry_counter = 0;
}

/**
 * Main application loop.
 * This function handles the state machine. All states should be handled
 * by a static state machine handler function SH_x().
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
		case APP_STATE_CALIBRATION_BEGIN:
			SH_calibration_begin();
			break;
		case APP_STATE_CALIBRATION_CONTINUE:
			SH_calibration_continue();
			break;
		case APP_STATE_MEASURING_BEGIN:
			SH_measuring_begin();
			break;
		case APP_STATE_MEASURING_CONTINUE:
			SH_measuring_continue();
			break;
		case APP_STATE_POWER_DOWN_BEGIN:
			SH_power_down_begin();
			break;
		case APP_STATE_POWER_DOWN_CONTINUE:
			SH_power_down_continue();
			break;
		}
	}
}

void app_tick(void)
{
	++millisecond_counter;
	for(int i = 0; i < DNCNT_LAST; i++) {
		if(dncnt_arr[i] > 0) {
			--dncnt_arr[i];
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
	dncnt_set(DNCNT_SHARED, 5000); // Set a timer...
	SM_set(APP_STATE_SPLASHSCREEN_CONTINUE); // ...and wait for it to time out.
}

static void SH_splashscreen_continue(void)
{
	if(dncnt_timedout(DNCNT_SHARED)) {
		display_clear(Black);
		SM_set(APP_STATE_CALIBRATION_BEGIN);
	}
}

static void SH_calibration_begin(void)
{
	if(auto_tare_retry_counter >= RETARE_RETRY_MAX) {
		// If we reach the maximum number of re-tares
		// then we assume one of the dogs has fallen
		// asleep on the scales meaning it's bedtime
		// so in this case we shutdown and wait for
		// a reboot/power cycle.
		SM_set(APP_STATE_POWER_DOWN_BEGIN);
	}
	else if(current_weight > INHIBIT_RETARE_WEIGHT) {
		auto_tare_retry_counter++;
		dncnt_set(DNCNT_AUTO_TARE, RETARE_TIME);
		SM_set(APP_STATE_MEASURING_CONTINUE);
	}
	else {
		display_clear(Black);
		calibrationscreen();
		auto_tare_retry_counter = 0;
		measure_init(&measure);
		SM_set(APP_STATE_CALIBRATION_CONTINUE); // ...and wait for it to time out.
	}
}

static void SH_calibration_continue(void)
{
	int32_t raw;
	if (hx711_get_data(&hx711, &raw) == HAL_OK) {
		// We are calibrating, wait for calibration buffer to fill
		measure_calibrate_put(&measure, raw);
		if(measure.calibration_complete == true) {
			HAL_UART_Transmit(&hlpuart1, STR("Millisec,Raw,Avg,Adj\r\n"), HAL_MAX_DELAY);
			SM_set(APP_STATE_MEASURING_BEGIN);
		}
	}
}

static void SH_measuring_begin(void)
{
	display_clear(Black);
	drawboarder();
	zero_display();
	dncnt_set(DNCNT_AUTO_TARE, RETARE_TIME);
	SM_set(APP_STATE_MEASURING_CONTINUE);
}

static void SH_measuring_continue(void)
{
	int32_t raw = 0;
	if(dncnt_timedout(DNCNT_AUTO_TARE)) {
		SM_set(APP_STATE_CALIBRATION_BEGIN);
		return;
	}

	if (hx711_get_data(&hx711, &raw) == HAL_OK) {
		measure_put(&measure, raw);
		int32_t avg = measure_get_avg(&measure);
		int32_t adj = avg - measure.calibration_value;
		current_weight = (float)((float)adj / COUNTS_PER_KG);
		char buffer[128] = {0};
		sprintf(buffer, "%04.1f", current_weight);
		buffer[4] = '\0';
		fontx_DrawString(buffer, FONTX_CENTER_X, FONTX_CENTER_Y, White);
		ssd1306_UpdateScreen(&hi2c1);
		buffer[0] = '\0';
		int len = sprintf(buffer, "%d,%i,%i,%i\r\n", (int)millisecond_counter, (int)raw, (int)avg, (int)adj);
		if(len > 0) {
			HAL_UART_Transmit(&hlpuart1, (uint8_t*)buffer, len, HAL_MAX_DELAY);
		}
	}
	else {
		HAL_Delay(1);
	}
}

static void SH_power_down_begin(void)
{
	display_clear(Black);
	powerdownscreen();
	dncnt_set(DNCNT_POWER_DOWN, 5000); // Set a timer...
	SM_set(APP_STATE_POWER_DOWN_CONTINUE);
}

static void SH_power_down_continue(void)
{
	if(dncnt_timedout(DNCNT_POWER_DOWN)) {
		SM_set(APP_STATE_STARTUP);
		ssd1306_DisplayOff(&hi2c1);
		HAL_SuspendTick();
		HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
	}
}


static void drawboarder(void)
{
	for(uint8_t i=0; i < SSD1306_WIDTH; i++) {
		ssd1306_DrawPixel(i, 0, White);
		ssd1306_DrawPixel(i, 1, White);
		ssd1306_DrawPixel(i, 2, White);
		ssd1306_DrawPixel(i, SSD1306_HEIGHT - 3, White);
		ssd1306_DrawPixel(i, SSD1306_HEIGHT - 2, White);
		ssd1306_DrawPixel(i, SSD1306_HEIGHT - 1, White);
	}
	for(uint8_t i=0; i < SSD1306_HEIGHT; i++) {
		ssd1306_DrawPixel(0, i, White);
		ssd1306_DrawPixel(1, i, White);
		ssd1306_DrawPixel(2, i, White);
		ssd1306_DrawPixel(SSD1306_WIDTH - 3, i, White);
		ssd1306_DrawPixel(SSD1306_WIDTH - 2, i, White);
		ssd1306_DrawPixel(SSD1306_WIDTH - 1, i, White);
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

static void calibrationscreen(void)
{
	uint8_t x = 18;
	uint8_t y = 15;
	FontDef f = Font_7x10;
	const char *txt[4] = {
		" Digi Scales\0",
		"            \0",
		" Calibrating\0",
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

static void powerdownscreen(void)
{
	ssd1306_SetCursor(18, 27);
	ssd1306_WriteString(" Power Down ", Font_7x10, White);
	ssd1306_UpdateScreen(&hi2c1);
	drawboarder();
}

static void zero_display(void)
{
	const char *s = "00.0";
	fontx_DrawString(s, FONTX_CENTER_X, FONTX_CENTER_Y, White);
	ssd1306_UpdateScreen(&hi2c1);
}

static inline APP_STATE_e SM_set(APP_STATE_e val)
{
	APP_STATE_e prev = app_state;
	app_state = val;
	return prev;
}
static inline APP_STATE_e SM_get()
{
	return app_state;
}

bool dncnt_timedout(DNCNT_e i)
{
	return dncnt_get(i) == 0;
}

uint32_t dncnt_get(DNCNT_e i)
{
	return dncnt_arr[i];
}

uint32_t dncnt_set(DNCNT_e i, uint32_t val)
{
	uint32_t v = dncnt_arr[i];
	dncnt_arr[i] = val;
	return v;
}

static void display_clear(SSD1306_COLOR color)
{
	ssd1306_Fill(color);
	ssd1306_UpdateScreen(&hi2c1);
}

