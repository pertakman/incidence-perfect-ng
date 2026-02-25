#include "touch_bsp.h"
#include <Wire.h>

// ============================================================
// TOUCH CONFIG (FT3168)
// ============================================================

#define EXAMPLE_PIN_NUM_TOUCH_SCL   (GPIO_NUM_39)
#define EXAMPLE_PIN_NUM_TOUCH_SDA   (GPIO_NUM_40)

#define EXAMPLE_LCD_H_RES  536
#define EXAMPLE_LCD_V_RES  240

#define I2C_ADDR_FT3168   0x38

// ============================================================
// I2C HELPERS (Arduino Wire)
// ============================================================

static uint8_t I2C_writr_buff(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len)
{
  Wire.beginTransmission(addr);
  Wire.write(reg);
  for (uint8_t i = 0; i < len; i++) {
    Wire.write(buf[i]);
  }
  return Wire.endTransmission();
}

static uint8_t I2C_read_buff(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len)
{
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.endTransmission(false);   // repeated start
  Wire.requestFrom(addr, len);

  uint8_t i = 0;
  while (Wire.available() && i < len) {
    buf[i++] = Wire.read();
  }
  return 0;
}

// ============================================================
// PUBLIC API
// ============================================================

void Touch_Init(void)
{
  Wire.begin(EXAMPLE_PIN_NUM_TOUCH_SDA,
             EXAMPLE_PIN_NUM_TOUCH_SCL,
             300000);   // 300 kHz, matches original BSP

  // Put FT3168 into normal operating mode
  uint8_t data = 0x00;
  I2C_writr_buff(I2C_ADDR_FT3168, 0x00, &data, 1);
}

uint8_t getTouch(uint16_t *x, uint16_t *y)
{
  uint8_t points = 0;
  uint8_t buf[4];

  // Read number of touch points
  I2C_read_buff(I2C_ADDR_FT3168, 0x02, &points, 1);

  if (points == 0) {
    return 0;
  }

  // Read first touch point coordinates
  I2C_read_buff(I2C_ADDR_FT3168, 0x03, buf, 4);

  uint16_t ty = (((uint16_t)buf[0] & 0x0F) << 8) | buf[1];
  uint16_t tx = (((uint16_t)buf[2] & 0x0F) << 8) | buf[3];

  // Clamp to screen bounds
  if (tx > EXAMPLE_LCD_H_RES) tx = EXAMPLE_LCD_H_RES;
  if (ty > EXAMPLE_LCD_V_RES) ty = EXAMPLE_LCD_V_RES;

  // Match existing orientation behavior
  *x = EXAMPLE_LCD_H_RES - tx;
  *y = ty;

  return 1;
}
