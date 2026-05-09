#ifndef INC_HX711_H_
#define INC_HX711_H_

#include <stdint.h>
#include "stm32g4xx_hal.h"

// Number of PD_SCK pulses sent after the 24 data bits selects the channel
// and gain for the NEXT conversion (datasheet Table 3).
typedef enum {
	HX711_GAIN_A_128 = 25,   // Channel A, gain 128 — ±20mV FS at 5V AVDD
	HX711_GAIN_B_32  = 26,   // Channel B, gain 32  — ±80mV FS
	HX711_GAIN_A_64  = 27,   // Channel A, gain 64  — ±40mV FS
} HX711_Gain;

typedef struct {
	GPIO_TypeDef  *CLK_GPIO;
	uint16_t       CLK_Pin;
	GPIO_TypeDef  *DATA_GPIO;
	uint16_t       DATA_Pin;
	HX711_Gain     gain;
} HX711_t;

// Initialise the driver, set gain/channel, and perform one dummy read to
// prime the gain selection for all subsequent reads.
// Returns HAL_TIMEOUT if the chip does not respond within 500ms.
HAL_StatusTypeDef hx711_init(HX711_t *hx, HX711_Gain gain);

// Returns 1 if a new sample is ready (DOUT low), 0 if still converting.
uint8_t hx711_is_ready(HX711_t *hx);

// Non-blocking read. Returns HAL_OK and writes the 24-bit 2's complement
// value sign-extended to int32_t, or HAL_BUSY if not yet ready.
HAL_StatusTypeDef hx711_get_data(HX711_t *hx, int32_t *outval);

// Hold PD_SCK high for >60us to power down the chip and load cell excitation.
void hx711_power_down(HX711_t *hx);

// Release PD_SCK (CLK low) to exit power-down. Chip resets internally;
// call hx711_init() again before reading.
void hx711_power_up(HX711_t *hx);

#endif /* INC_HX711_H_ */
