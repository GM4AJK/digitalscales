#include "hx711.h"

// ---------------------------------------------------------------------------
// Bit-bang timing
//
// The HX711 requires:
//   T3  PD_SCK high time  min 0.2µs, typ 1µs
//   T4  PD_SCK low  time  min 0.2µs, typ 1µs
//
// We use the Cortex-M4 DWT cycle counter for accurate 1µs delays so the
// driver is correct regardless of compiler optimisation level.
// DWT is enabled once inside hx711_init().
// ---------------------------------------------------------------------------

static void delay_us(uint32_t us)
{
	uint32_t target = DWT->CYCCNT + us * (SystemCoreClock / 1000000U);
	while ((int32_t)(DWT->CYCCNT - target) < 0);
}

// ---------------------------------------------------------------------------
// Internal: clock out one complete conversion and return the signed value.
// Caller must have confirmed DOUT is low before calling.
// ---------------------------------------------------------------------------
static int32_t read_raw(HX711_t *hx)
{
	uint32_t raw = 0;

	// Read 24 bits, MSB first.
	// Data is valid on DOUT after each PD_SCK rising edge (T2 max 0.1µs);
	// we sample it after the falling edge which gives maximum setup margin.
	for (int i = 0; i < 24; i++) {
		HAL_GPIO_WritePin(hx->CLK_GPIO, hx->CLK_Pin, GPIO_PIN_SET);
		delay_us(1);                                    // T3: CLK high ≥ 0.2µs
		HAL_GPIO_WritePin(hx->CLK_GPIO, hx->CLK_Pin, GPIO_PIN_RESET);
		raw <<= 1;
		if (HAL_GPIO_ReadPin(hx->DATA_GPIO, hx->DATA_Pin) == GPIO_PIN_SET) {
			raw |= 1UL;
		}
		delay_us(1);                                    // T4: CLK low ≥ 0.2µs
	}

	// Extra pulses beyond 24 programme the channel/gain for the NEXT
	// conversion: 25 → Ch.A gain 128, 26 → Ch.B gain 32, 27 → Ch.A gain 64.
	for (int i = 24; i < (int)hx->gain; i++) {
		HAL_GPIO_WritePin(hx->CLK_GPIO, hx->CLK_Pin, GPIO_PIN_SET);
		delay_us(1);
		HAL_GPIO_WritePin(hx->CLK_GPIO, hx->CLK_Pin, GPIO_PIN_RESET);
		delay_us(1);
	}

	// Sign-extend 24-bit 2's complement to int32_t.
	if (raw & 0x00800000UL) {
		raw |= 0xFF000000UL;
	}
	return (int32_t)raw;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

HAL_StatusTypeDef hx711_init(HX711_t *hx, HX711_Gain gain)
{
	hx->gain = gain;

	// Enable the DWT cycle counter used by delay_us().
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT->CTRL        |= DWT_CTRL_CYCCNTENA_Msk;

	// CLK low = normal operating mode (not power-down).
	HAL_GPIO_WritePin(hx->CLK_GPIO, hx->CLK_Pin, GPIO_PIN_RESET);

	// Wait for the first conversion to complete after power-up reset.
	// At 10 SPS the output settling time can be up to 400ms (datasheet Table 2).
	uint32_t start = HAL_GetTick();
	while (HAL_GPIO_ReadPin(hx->DATA_GPIO, hx->DATA_Pin) == GPIO_PIN_SET) {
		if (HAL_GetTick() - start > 500U) {
			return HAL_TIMEOUT;
		}
	}

	// Dummy read: clocks out the pending sample and programs the requested
	// gain so all subsequent hx711_get_data() calls return at the right gain.
	read_raw(hx);
	return HAL_OK;
}

uint8_t hx711_is_ready(HX711_t *hx)
{
	GPIO_PinState dt = HAL_GPIO_ReadPin(hx->DATA_GPIO, hx->DATA_Pin);
	return dt == GPIO_PIN_RESET;
}

HAL_StatusTypeDef hx711_get_data(HX711_t *hx, int32_t *outval)
{
	if (!hx711_is_ready(hx)) {
		return HAL_BUSY;
	}
	*outval = -read_raw(hx);
	return HAL_OK;
}

void hx711_power_down(HX711_t *hx)
{
	// Datasheet Fig.3: PD_SCK held high for >60µs triggers power-down.
	// Bring CLK low first to ensure a clean rising edge, then hold high.
	// HAL_Delay(1) gives 1ms which is well above the 60µs threshold.
	HAL_GPIO_WritePin(hx->CLK_GPIO, hx->CLK_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(hx->CLK_GPIO, hx->CLK_Pin, GPIO_PIN_SET);
	HAL_Delay(1);
}

void hx711_power_up(HX711_t *hx)
{
	// CLK low exits power-down. Chip performs internal reset and begins
	// a fresh conversion. Caller must call hx711_init() again before reading.
	HAL_GPIO_WritePin(hx->CLK_GPIO, hx->CLK_Pin, GPIO_PIN_RESET);
}
