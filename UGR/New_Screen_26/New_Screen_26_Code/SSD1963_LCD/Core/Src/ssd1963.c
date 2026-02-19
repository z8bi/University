#include "ssd1963.h"
#include "main.h"
#include "gpio.h"
#include "fmc.h"
#include "tim.h"
#include "stm32f7xx_hal.h"

// -----------------------------
// FMC address mapping
// -----------------------------
// Bank1/NE1 base is 0x6000_0000.
// With LCD_RS connected to FMC_A16, the "data" address is typically base + 0x20000 on a 16-bit bus.
//
// If you ever suspect RS is inverted, swap CMD/DATA addresses below.
#define LCD_FMC_BASE   (0x60000000UL)
#define LCD_CMD_ADDR   (LCD_FMC_BASE)
#define LCD_DATA_ADDR  (LCD_FMC_BASE + 0x00020000UL)

static inline void lcd_cmd(uint16_t c)  { *(__IO uint16_t*)LCD_CMD_ADDR  = c; }
static inline void lcd_dat(uint16_t d)  { *(__IO uint16_t*)LCD_DATA_ADDR = d; }

// -----------------------------
// Panel timing constants (from vendor example)
// -----------------------------
#define SSD_HOR_RESOLUTION      800
#define SSD_VER_RESOLUTION      480

#define SSD_HOR_PULSE_WIDTH     1
#define SSD_HOR_BACK_PORCH      46
#define SSD_HOR_FRONT_PORCH     210

#define SSD_VER_PULSE_WIDTH     1
#define SSD_VER_BACK_PORCH      23
#define SSD_VER_FRONT_PORCH     22

#define SSD_HT  (SSD_HOR_RESOLUTION + SSD_HOR_BACK_PORCH + SSD_HOR_FRONT_PORCH) // 1056
#define SSD_HPS (SSD_HOR_BACK_PORCH)                                            // 46
#define SSD_VT  (SSD_VER_RESOLUTION + SSD_VER_BACK_PORCH + SSD_VER_FRONT_PORCH) // 525
#define SSD_VPS (SSD_VER_BACK_PORCH)                                            // 23

// Orientation: vendor example uses MADCTL (0x36). Many panels differ on BGR.
// Start with BGR=0; if colors are swapped (red/blue), set BGR=1.
#define SSD1963_BGR 0

static void ssd_write_reg8(uint16_t reg, uint8_t val)
{
    lcd_cmd(reg);
    lcd_dat(val);
}

static void ssd_write_reg(uint16_t reg, const uint8_t* data, uint32_t n)
{
    lcd_cmd(reg);
    for (uint32_t i = 0; i < n; i++) lcd_dat(data[i]);
}

static void ssd_soft_reset(void)
{
    lcd_cmd(0x01);
    HAL_Delay(10);
}

// Set column/page address + memory write
void SSD1963_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    // 0x2A: column address set
    lcd_cmd(0x2A);
    lcd_dat((x0 >> 8) & 0xFF);
    lcd_dat(x0 & 0xFF);
    lcd_dat((x1 >> 8) & 0xFF);
    lcd_dat(x1 & 0xFF);

    // 0x2B: page address set
    lcd_cmd(0x2B);
    lcd_dat((y0 >> 8) & 0xFF);
    lcd_dat(y0 & 0xFF);
    lcd_dat((y1 >> 8) & 0xFF);
    lcd_dat(y1 & 0xFF);

    // 0x2C: memory write
    lcd_cmd(0x2C);
}

void SSD1963_Fill(uint16_t rgb565)
{
    SSD1963_SetWindow(0, 0, SSD_HOR_RESOLUTION - 1, SSD_VER_RESOLUTION - 1);

    const uint32_t pixels = (uint32_t)SSD_HOR_RESOLUTION * (uint32_t)SSD_VER_RESOLUTION;
    for (uint32_t i = 0; i < pixels; i++)
    {
        // 16-bit 565 on 16-bit parallel bus
        *(__IO uint16_t*)LCD_DATA_ADDR = rgb565;
    }
}

// -----------------------------
// Tiny "gfx" helpers (RGB565)
// -----------------------------

void SSD1963_DrawPixel(uint16_t x, uint16_t y, uint16_t rgb565)
{
    if (x >= SSD_HOR_RESOLUTION || y >= SSD_VER_RESOLUTION) return;
    SSD1963_SetWindow(x, y, x, y);
    *(__IO uint16_t*)LCD_DATA_ADDR = rgb565;
}

void SSD1963_DrawHLine(uint16_t x, uint16_t y, uint16_t w, uint16_t rgb565)
{
    if (y >= SSD_VER_RESOLUTION || x >= SSD_HOR_RESOLUTION) return;
    if (x + w > SSD_HOR_RESOLUTION) w = (uint16_t)(SSD_HOR_RESOLUTION - x);
    if (w == 0) return;
    SSD1963_SetWindow(x, y, (uint16_t)(x + w - 1), y);
    for (uint32_t i = 0; i < w; i++) *(__IO uint16_t*)LCD_DATA_ADDR = rgb565;
}

void SSD1963_DrawVLine(uint16_t x, uint16_t y, uint16_t h, uint16_t rgb565)
{
    if (x >= SSD_HOR_RESOLUTION || y >= SSD_VER_RESOLUTION) return;
    if (y + h > SSD_VER_RESOLUTION) h = (uint16_t)(SSD_VER_RESOLUTION - y);
    if (h == 0) return;
    SSD1963_SetWindow(x, y, x, (uint16_t)(y + h - 1));
    for (uint32_t i = 0; i < h; i++) *(__IO uint16_t*)LCD_DATA_ADDR = rgb565;
}

void SSD1963_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t rgb565)
{
    if (x >= SSD_HOR_RESOLUTION || y >= SSD_VER_RESOLUTION) return;
    if (x + w > SSD_HOR_RESOLUTION) w = (uint16_t)(SSD_HOR_RESOLUTION - x);
    if (y + h > SSD_VER_RESOLUTION) h = (uint16_t)(SSD_VER_RESOLUTION - y);
    if (w == 0 || h == 0) return;

    SSD1963_SetWindow(x, y, (uint16_t)(x + w - 1), (uint16_t)(y + h - 1));
    const uint32_t pixels = (uint32_t)w * (uint32_t)h;
    for (uint32_t i = 0; i < pixels; i++) *(__IO uint16_t*)LCD_DATA_ADDR = rgb565;
}

void SSD1963_WritePixels(const uint16_t *px, uint32_t count)
{
    volatile uint16_t *dst = (volatile uint16_t*)LCD_DATA_ADDR;
    for (uint32_t i = 0; i < count; i++) {
        *dst = px[i];
    }
}

void SSD1963_TestColorBars(void)
{
    // 8 vertical bars (classic)
    const uint16_t colors[8] = {
        RGB565(255,255,255), // white
        RGB565(255,255,0),   // yellow
        RGB565(0,255,255),   // cyan
        RGB565(0,255,0),     // green
        RGB565(255,0,255),   // magenta
        RGB565(255,0,0),     // red
        RGB565(0,0,255),     // blue
        RGB565(0,0,0)        // black
    };

    const uint16_t bar_w = (uint16_t)(SSD_HOR_RESOLUTION / 8);
    for (uint16_t i = 0; i < 8; i++) {
        const uint16_t x = (uint16_t)(i * bar_w);
        const uint16_t w = (i == 7) ? (uint16_t)(SSD_HOR_RESOLUTION - x) : bar_w;
        SSD1963_FillRect(x, 0, w, SSD_VER_RESOLUTION, colors[i]);
    }
}

void SSD1963_TestCheckerboard(uint16_t tile)
{
    if (tile == 0) tile = 16;
    for (uint16_t y = 0; y < SSD_VER_RESOLUTION; y += tile) {
        for (uint16_t x = 0; x < SSD_HOR_RESOLUTION; x += tile) {
            const uint16_t w = (x + tile > SSD_HOR_RESOLUTION) ? (uint16_t)(SSD_HOR_RESOLUTION - x) : tile;
            const uint16_t h = (y + tile > SSD_VER_RESOLUTION) ? (uint16_t)(SSD_VER_RESOLUTION - y) : tile;
            const uint16_t c = (((x / tile) ^ (y / tile)) & 1) ? RGB565(30,30,30) : RGB565(220,220,220);
            SSD1963_FillRect(x, y, w, h, c);
        }
    }
}

static void ssd_set_madctl_landscape(void)
{
    // Vendor example maps "USE_HORIZONTAL=1" to MADCTL value 0x00 (with their setup).
    // We add optional BGR bit (bit3).
    uint8_t madctl = 0x00;
    if (SSD1963_BGR) madctl |= (1u << 3);
    ssd_write_reg8(0x36, madctl);
}

void SSD1963_Init(void)
{
    // ---- Hardware reset pin (PC5 in your design) ----
    // Make sure PC5 is labeled LCD_RESET in CubeMX so these macros exist.
    HAL_GPIO_WritePin(LCD_RESET_GPIO_Port, LCD_RESET_Pin, GPIO_PIN_RESET);
    HAL_Delay(20);
    HAL_GPIO_WritePin(LCD_RESET_GPIO_Port, LCD_RESET_Pin, GPIO_PIN_SET);
    HAL_Delay(50);

    // ---- SSD1963 init (ported from vendor example) ----

    // Set PLL (vendor comment assumes SSD1963 OSC=10MHz)
    // 0xE2: PLL configuration
    {
        const uint8_t pll[] = {
            0x1D, // N
            0x02, // M
            0x04  // validate
        };
        ssd_write_reg(0xE2, pll, sizeof(pll));
    }
    HAL_Delay(1);

    // 0xE0: start PLL
    ssd_write_reg8(0xE0, 0x01);
    HAL_Delay(10);
    ssd_write_reg8(0xE0, 0x03);
    HAL_Delay(12);

    ssd_soft_reset();

    // 0xE6: pixel clock (vendor sets 0x03,0xFF,0xFF)
    {
        const uint8_t pclk[] = { 0x03, 0xFF, 0xFF };
        ssd_write_reg(0xE6, pclk, sizeof(pclk));
    }

    // 0xB0: LCD mode
    // vendor uses: 0x20, 0x00, (H-1), (V-1), 0x00
    {
        uint8_t b0[] = {
            0x20, // 24-bit mode (internal), still can use 565 interface via 0xF0
            0x00, // TFT mode
            (uint8_t)((SSD_HOR_RESOLUTION - 1) >> 8),
            (uint8_t)((SSD_HOR_RESOLUTION - 1) & 0xFF),
            (uint8_t)((SSD_VER_RESOLUTION - 1) >> 8),
            (uint8_t)((SSD_VER_RESOLUTION - 1) & 0xFF),
            0x00  // RGB sequence
        };
        ssd_write_reg(0xB0, b0, sizeof(b0));
    }

    // 0xB4: horizontal period
    {
        uint8_t b4[] = {
            (uint8_t)((SSD_HT - 1) >> 8),
            (uint8_t)((SSD_HT - 1) & 0xFF),
            (uint8_t)(SSD_HPS >> 8),
            (uint8_t)(SSD_HPS & 0xFF),
            (uint8_t)(SSD_HOR_PULSE_WIDTH - 1),
            0x00, 0x00, 0x00
        };
        ssd_write_reg(0xB4, b4, sizeof(b4));
    }

    // 0xB6: vertical period
    {
        uint8_t b6[] = {
            (uint8_t)((SSD_VT - 1) >> 8),
            (uint8_t)((SSD_VT - 1) & 0xFF),
            (uint8_t)(SSD_VPS >> 8),
            (uint8_t)(SSD_VPS & 0xFF),
            (uint8_t)(SSD_VER_FRONT_PORCH - 1),
            0x00, 0x00
        };
        ssd_write_reg(0xB6, b6, sizeof(b6));
    }

    // 0xF0: set CPU interface to 16-bit (565 for 16bpp)
    ssd_write_reg8(0xF0, 0x03);

    // Display ON
    lcd_cmd(0x29);

    // Disable DBC (vendor: 0xD0 0x00)
    ssd_write_reg8(0xD0, 0x00);

    // Set GPIO config (vendor: 0xB8 0x03 0x01)
    {
        uint8_t b8[] = { 0x03, 0x01 };
        ssd_write_reg(0xB8, b8, sizeof(b8));
    }

    // Direction control via GPIO? vendor sets 0xBA 0x01
    ssd_write_reg8(0xBA, 0x01);

    // MADCTL orientation
    ssd_set_madctl_landscape();
}
