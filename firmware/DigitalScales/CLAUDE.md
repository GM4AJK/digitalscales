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
  Inc/
    main.h          GPIO pin definitions (HX711_CLK_Pin PA4, HX711_DT_Pin PA5)
    app.h
    ssd1306.h
    fonts.h
    fontx.h
Drivers/            STM32 HAL + CMSIS (CubeMX-managed, do not edit)
DigitalScales.ioc   CubeMX project file
STM32G431KBTX_FLASH.ld  Linker script
```

## Key Peripherals (main.c)

- **I2C1** — OLED display (timing 0x40B285C2)
- **TIM6** — 1 ms software tick; prescaler 170, period 1000 at 170 MHz PCLK
- **GPIO PA4** output push-pull high-speed — HX711 CLK
- **GPIO PA5** input no-pull — HX711 DATA

## SH1106 / SSD1306 Display Notes

The OLED module uses an **SH1106** controller, not a true SSD1306, despite being labelled as such. Differences that matter:

- SH1106 has 132 columns of RAM; only columns 2–129 are visible
- In `ssd1306_UpdateScreen` the lower column start address is set to `0x02` (not `0x00`) to correct for this offset — **do not change this back to 0x00**
- SH1106 only supports page addressing mode (already used by this driver)

## fontx — Large Scale Readout Font

`fontx.c` / `fontx.h` provides a 30×56 pixel 7-segment style bitmap font for displaying scale readings.

- Characters supported: `'.'` (ASCII 46) through `'9'` (ASCII 57)
- Each `uint32_t` row uses bits 31..2 (MSB = leftmost pixel); bits 1..0 always 0
- 2-pixel blank margin on all four sides inside the 30×56 cell
- Drawn glyph occupies cols 2–27, rows 2–53

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

Both functions write into the SSD1306 screen buffer only; call `ssd1306_UpdateScreen(&hi2c1)` afterwards to push to the display.

## Timing

TIM6 fires an interrupt every 1 ms and increments `millisecond_counter` (volatile uint32_t in app.c). The main loop polls this counter for timed work.

## Build & Flash

STM32CubeIDE project. Open `DigitalScales.ioc` to regenerate HAL code (CubeMX). Flash via the included `.launch` debug configuration (`DigitalScales Debug.launch`).

**Do not edit files inside `Drivers/`** — they are managed by CubeMX. User code belongs inside the `/* USER CODE BEGIN/END */` blocks in CubeMX-generated files, or in `app.c` / `fontx.c` etc.
