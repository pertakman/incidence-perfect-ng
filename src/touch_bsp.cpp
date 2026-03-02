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
static const uint8_t FT3168_REG_POINTS = 0x02;
static const uint8_t FT3168_REG_POINT1 = 0x03;
static const uint8_t FT3168_REG_DEVICE_MODE = 0x00;
static const uint8_t FT3168_REG_POWER_MODE = 0xA5;
static const uint8_t FT3168_POWER_ACTIVE = 0x00;
static const uint8_t FT3168_POWER_HIBERNATE = 0x03;
static const uint8_t FT3168_POWER_MONITOR = 0x01;
// Reliability-first default: monitor mode wakes consistently on this board.
// Hibernate can reduce current further but has shown wake regressions.
static const bool touchSleepPreferHibernate = false;

static bool touchReady = false;
static unsigned long touchLastRecoverMs = 0;
static const unsigned long touchRecoverIntervalMs = 250;

// ============================================================
// I2C HELPERS (Arduino Wire)
// ============================================================

static uint8_t I2C_write_buff(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len)
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
  // Use STOP between write/read for better robustness on this board.
  // Repeated-start can trigger intermittent i2cWriteReadNonStop failures.
  Wire.beginTransmission(addr);
  Wire.write(reg);
  uint8_t err = Wire.endTransmission(true);
  if (err != 0) {
    return err;
  }

  size_t n = Wire.requestFrom((int)addr, (int)len);
  if (n < len) {
    while (Wire.available()) {
      (void)Wire.read();
    }
    return 0xFE;
  }

  uint8_t i = 0;
  while (Wire.available() && i < len) {
    buf[i++] = Wire.read();
  }
  if (i < len) {
    return 0xFD;
  }
  return 0;
}

static bool touchWakeAndConfigure(void)
{
  // Best-effort wake from hibernate for variants that implement power mode.
  // Ignore errors so variants without this register still proceed.
  uint8_t power = FT3168_POWER_ACTIVE;
  (void)I2C_write_buff(I2C_ADDR_FT3168, FT3168_REG_POWER_MODE, &power, 1);
  delay(2);

  // Keep bring-up compatible across controller variants. Consider either
  // configure-write success or probe success as "reachable".
  uint8_t data = 0x00;
  const bool cfg_ok = (I2C_write_buff(I2C_ADDR_FT3168, FT3168_REG_DEVICE_MODE, &data, 1) == 0);
  delay(3);

  uint8_t points = 0;
  if (I2C_read_buff(I2C_ADDR_FT3168, FT3168_REG_POINTS, &points, 1) == 0 || cfg_ok) {
    return true;
  }

  // One extra settle-and-probe pass after sleep wake.
  delay(10);
  if (I2C_read_buff(I2C_ADDR_FT3168, FT3168_REG_POINTS, &points, 1) == 0) {
    return true;
  }

  // Some parts need a longer wake settle after hibernate.
  delay(25);
  (void)I2C_write_buff(I2C_ADDR_FT3168, FT3168_REG_POWER_MODE, &power, 1);
  delay(4);
  return I2C_read_buff(I2C_ADDR_FT3168, FT3168_REG_POINTS, &points, 1) == 0;
}

static bool touchRecover(void)
{
  // First try without tearing down the shared bus; this avoids disturbing
  // the IMU path unless recovery really needs a full re-init.
  Wire.begin(EXAMPLE_PIN_NUM_TOUCH_SDA, EXAMPLE_PIN_NUM_TOUCH_SCL, 300000);
  delay(2);
  if (touchWakeAndConfigure()) {
    return true;
  }

  Wire.end();
  delay(2);
  Wire.begin(EXAMPLE_PIN_NUM_TOUCH_SDA, EXAMPLE_PIN_NUM_TOUCH_SCL, 300000);
  delay(3);
  return touchWakeAndConfigure();
}

// ============================================================
// PUBLIC API
// ============================================================

void Touch_Init(void)
{
  touchReady = touchRecover();
  touchLastRecoverMs = millis();
}

uint8_t getTouch(uint16_t *x, uint16_t *y)
{
  if (!x || !y) return 0;

  const unsigned long now = millis();
  if (!touchReady) {
    if ((now - touchLastRecoverMs) >= touchRecoverIntervalMs) {
      touchLastRecoverMs = now;
      touchReady = touchRecover();
    }
    return 0;
  }

  uint8_t points = 0;
  uint8_t buf[4];

  // Read number of touch points
  if (I2C_read_buff(I2C_ADDR_FT3168, FT3168_REG_POINTS, &points, 1) != 0) {
    touchReady = false;
    touchLastRecoverMs = now;
    return 0;
  }

  if (points == 0) {
    return 0;
  }

  // Read first touch point coordinates
  if (I2C_read_buff(I2C_ADDR_FT3168, FT3168_REG_POINT1, buf, 4) != 0) {
    touchReady = false;
    touchLastRecoverMs = now;
    return 0;
  }

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

bool Touch_Sleep(void)
{
  // Prefer hibernate for lower sleep current; fall back to monitor if needed.
  uint8_t data = touchSleepPreferHibernate ? FT3168_POWER_HIBERNATE : FT3168_POWER_MONITOR;
  bool ok = I2C_write_buff(I2C_ADDR_FT3168, FT3168_REG_POWER_MODE, &data, 1) == 0;
  if (!ok && touchSleepPreferHibernate) {
    data = FT3168_POWER_MONITOR;
    ok = I2C_write_buff(I2C_ADDR_FT3168, FT3168_REG_POWER_MODE, &data, 1) == 0;
  }
  if (ok) {
    touchReady = false;
  }
  return ok;
}
