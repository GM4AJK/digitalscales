# DigitalScales Firmware

STM32G431KB-based digital scale firmware. Developed in STM32CubeIDE.

## Hardware

- **MCU**: STM32G431KB (Cortex-M4, 170 MHz)
- **Load cell ADC**: HX711 — CLK on PA4, DATA on PA5
- **Display**: 128×64 OLED, I2C1 — **SH1106 controller** (sold as SSD1306-compatible)
- **HX711 datasheet**: `datasheets/hx711F.pdf`

## Project Structure

```
Core/
  Src/
    main.c          STM32CubeMX-generated HAL init, calls app_init() / app_loop()
    app.c           Application entry point — init and main loop
    ssd1306.c       OLED driver (page-addressing mode, I2C)
    fonts.c         Standard bitmap fonts: Font_7x10, Font_11x18, Font_16x26
    fontx.c         Large 7-segment font for scale readout (see below)
    hx711.c         HX711 load cell ADC driver (see below)
  Inc/
    main.h          GPIO pin definitions (HX711_CLK_Pin PA4, HX711_DT_Pin PA5)
    app.h
    ssd1306.h
    fonts.h
    fontx.h
    hx711.h
Drivers/            STM32 HAL + CMSIS (CubeMX-managed, do not edit)
datasheets/
  hx711F.pdf
DigitalScales.ioc   CubeMX project file
STM32G431KBTX_FLASH.ld  Linker script
```

## Key Peripherals (main.c)

- **I2C1** — OLED display (timing 0x4052060F)
- **TIM6** — 1 ms software tick; prescaler 170, period 1000 at 170 MHz PCLK
- **GPIO PA4** output push-pull high-speed — HX711 CLK
- **GPIO PA5** input no-pull — HX711 DATA

## Coding Style

- **Tabs** for indentation, never spaces
- Braces on all control flow, body on its own line
- HAL is acceptable throughout this project
- `/* USER CODE BEGIN/END */` blocks for anything in CubeMX-generated files

## SH1106 / SSD1306 Display Notes

The OLED module uses an **SH1106** controller, not a true SSD1306, despite being labelled as such. Differences that matter:

- SH1106 has 132 columns of RAM; only columns 2–129 are visible
- In `ssd1306_UpdateScreen` the lower column start address is set to `0x02` (not `0x00`) to correct for this offset — **do not change this back to 0x00**
- SH1106 only supports page addressing mode (already used by this driver)

## fontx — Large Scale Readout Font

`fontx.c` / `fontx.h` provides a 30×56 pixel 7-segment style bitmap font for displaying scale readings.

- Characters supported: `'.'` (ASCII 46) through `'9'` (ASCII 57)
- Each `uint32_t` row uses bits 31..2 (MSB = leftmost pixel); bits 1..0 always 0
- 2-pixel blank margin on all four sides inside the 30×56 cell; glyph occupies cols 2–27, rows 2–53
- Segment masks: `SEG_H=0x03FFFF00`, `SEG_L=0x3C000000`, `SEG_R=0x000000F0`

**Layout for "XX.X" on 128×64 display:**

```c
fontx_DrawString("00.0", FONTX_CENTER_X, FONTX_CENTER_Y, White);
// FONTX_CENTER_X = 4,  FONTX_CENTER_Y = 4
// 4 × 30px = 120px wide, centred with 4px margin each side
// 56px tall, centred with 4px margin top and bottom
```

**API:**

```c
uint8_t fontx_DrawChar(char ch, uint8_t x, uint8_t y, SSD1306_COLOR color);
void    fontx_DrawString(const char *str, uint8_t x, uint8_t y, SSD1306_COLOR color);
```

Both functions write into the screen buffer only; call `ssd1306_UpdateScreen(&hi2c1)` afterwards.

## HX711 Driver

`hx711.c` / `hx711.h` — bit-bang driver using the DWT cycle counter for 1 µs timing.

```c
typedef enum {
	HX711_GAIN_A_128 = 25,  // Channel A, gain 128 — default for load cells
	HX711_GAIN_B_32  = 26,  // Channel B, gain 32
	HX711_GAIN_A_64  = 27,  // Channel A, gain 64
} HX711_Gain;
```

The enum value equals the total PD_SCK pulse count per conversion. Channel/gain for the NEXT conversion is programmed by the pulse count at the end of each read (datasheet Table 3).

**API:**

```c
HAL_StatusTypeDef hx711_init(HX711_t *hx, HX711_Gain gain);  // waits up to 500ms for first conversion
uint8_t           hx711_is_ready(HX711_t *hx);               // 1 = DOUT low = ready
HAL_StatusTypeDef hx711_get_data(HX711_t *hx, int32_t *outval); // HAL_BUSY if not ready
void              hx711_power_down(HX711_t *hx);
void              hx711_power_up(HX711_t *hx);                // call hx711_init() again after
```

- Returns `HAL_BUSY` while conversion is in progress (~100 ms at 10 SPS) — this is normal
- Raw value is 24-bit 2's complement sign-extended to `int32_t`
- DWT cycle counter enabled in `hx711_init()` — do not reset `DWT->CYCCNT` elsewhere

**Critical:** Never toggle or drive CLK high outside the driver. PD_SCK held high >60 µs triggers power-down and DOUT stays high permanently until CLK goes low again.

**Initialisation in app.c:**

```c
hx711.CLK_GPIO  = HX711_CLK_GPIO_Port;
hx711.CLK_Pin   = HX711_CLK_Pin;
hx711.DATA_GPIO = HX711_DT_GPIO_Port;
hx711.DATA_Pin  = HX711_DT_Pin;
hx711_init(&hx711, HX711_GAIN_A_128);
```

## sprintf float formatting

```c
sprintf(buffer, "%04.1f", value);  // "03.5", "99.9" etc.
```

If output is blank or "?", enable float support in the linker:
**Properties → C/C++ Build → Settings → MCU GCC Linker → Miscellaneous**, add `-u _printf_float`.

## Application State Machine (app.c)

States: `STARTUP → SPLASHSCREEN_BEGIN → SPLASHSCREEN_CONTINUE → MEASURING`

Each state has a dedicated `SH_*()` handler. `app_init()` sets the initial state and starts TIM6; `app_reinit()` does full peripheral init (display, HX711, measure module).

### Countdown timer subsystem

`DNCNT_e` enum + `dncnt_arr[]` array of `volatile uint32_t`, decremented every 1 ms in the TIM6 ISR.

```c
dncnt_set(DNCNT_SHARED, 10000);   // arm a 10-second timer
if (dncnt_timedout(DNCNT_SHARED)) { ... }
```

Currently one timer (`DNCNT_SHARED`). Add entries to `DNCNT_e` (before `DNCNT_LAST`) to add more.

## measure.c / measure.h

Measurement and display logic extracted from app.c.

```c
void measure_set_i2c(I2C_HandleTypeDef *p_hi2c);  // call once from app_reinit()
void measure(int32_t raw);                          // called from SH_measuring() when HX711 ready
```

`raw_to_display()` and `display_measurement_value()` are private statics. `raw_to_display()` is a throwaway mapping (raw → 0.0–99.9) for testing — remove when real calibration is implemented.

## Timing

TIM6 fires an interrupt every 1 ms, increments `millisecond_counter` (volatile uint32_t in app.c), and decrements all non-zero `dncnt_arr[]` entries.

## Current Status

- OLED display working with fontx large font ✓
- SH1106 column offset fixed ✓
- HX711 driver implemented and verified ✓
- State machine with splashscreen implemented ✓
- `measure.c` module extracted ✓
- **Load cell not yet connected** — next session: connect load cell, implement tare + calibration
- `raw_to_display()` in measure.c is a throwaway mapping — remove when calibration is implemented

## Build & Flash

STM32CubeIDE project. Open `DigitalScales.ioc` to regenerate HAL code (CubeMX). Flash via the included `.launch` debug configuration (`DigitalScales Debug.launch`).

**Do not edit files inside `Drivers/`** — they are managed by CubeMX. User code belongs inside the `/* USER CODE BEGIN/END */` blocks in CubeMX-generated files, or in `app.c` / `fontx.c` / `hx711.c` etc.
