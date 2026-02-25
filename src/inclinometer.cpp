/*
  QMI8658 – Inclinometer v4.2 (LOCKED)
  RC Plane Construction Tool

  BASELINE:
  - v4.1-LOCKED (hardware verified, angle behavior frozen)

  NEW IN v4.2:
  - True 6-orientation mechanical alignment
  - Alignment performed strictly in ANGLE DOMAIN
  - Alignment corrects constant roll & pitch bias only
  - No vector math, no gain correction, no axis remapping changes

  IMPORTANT DESIGN RULES:
  - Offset calibration corrects sensor bias (accelerometer + gyro)
  - Alignment corrects mechanical mounting error (angle bias)
  - Semantic roll sign flip is DISPLAY-ONLY and applied last
*/

/*
  DATA FLOW SUMMARY:
  Raw IMU → Remap → Bias removal → Accel angles
          → Gyro integration → Complementary filter
          → Alignment (angle bias)
          → User zero
          → Display semantics
*/

// ============================================================
// INCLUDES
// ============================================================

#include <QMI8658.h>
#include <Wire.h>
#include <EEPROM.h>
#include <math.h>
#include <lvgl.h>
#include "inclinometer_shared.h"

// ============================================================
// CONFIGURATION
// ============================================================

#define SDA_PIN 40
#define SCL_PIN 39

#define EEPROM_SIZE 128
#define EEPROM_ADDR_MODE       0
#define EEPROM_ADDR_INCIDENCE  1
#define EEPROM_ADDR_ALIGN      16   // stores: align_roll, align_pitch (floats)

// Complementary filter coefficient
// Lower alpha = more accel trust, higher alpha = more gyro trust
const float alpha = 0.90;

// Reference gravity magnitude (used only for accel Z offset)
const float g_ref = 9.81;

// Shared variable with UI
volatile float ui_roll = 0.0f;
volatile float ui_pitch = 0.0f;

// ============================================================
// GLOBAL STATE
// ============================================================

QMI8658 imu;

OrientationMode orientationMode;
bool incidenceMode = false;

// Sensor bias offsets (tool frame, SI units)
float ax_off=0, ay_off=0, az_off=0;
float gx_off=0, gy_off=0; // gyro bias (deg/s)

// Mechanical alignment corrections (ANGLE DOMAIN, degrees)
// These compensate for mounting / enclosure errors
float align_roll=0, align_pitch=0;

// Fused roll & pitch angles (degrees, physical frame)
float roll_phys=0, pitch_phys=0;

// User-defined zero reference (degrees)
float roll_zero=0, pitch_zero=0;

// Timekeeping for gyro integration
unsigned long lastTime=0; // ms
bool serialWasAttached = false;

// Forward declarations (required now that this file is compiled as C++)
void calibrateOffsets();
void initializeAngles();
void printMode();
void handleSerial();
void toggleIncidence();
void runAlignmentCalibration();
void captureAngles(float& r, float& p);
bool waitForC(unsigned long timeout_ms);

// ============================================================
// AXIS REMAPPING  (FROZEN v4.0 BEHAVIOR)
// ============================================================
//
// Tool coordinate system:
//   +X = forward
//   +Y = right
//   +Z = down
//
// This mapping MUST NOT CHANGE without redefining the instrument.
//

void remapAccel(const QMI8658_Data& d, float& ax, float& ay, float& az) {
  if (orientationMode == MODE_SCREEN_VERTICAL) {
    ax =  d.accelZ;
    ay = -d.accelY;
    az = -d.accelX;
  } else {
    ax =  d.accelX;
    ay =  d.accelY;
    az =  d.accelZ;
  }
}

void remapGyro(const QMI8658_Data& d, float& gx, float& gy) {
  if (orientationMode == MODE_SCREEN_VERTICAL) {
    gx = -d.gyroZ;
    gy =  d.gyroY;
  } else {
    gx =  d.gyroX;
    gy =  d.gyroY;
  }
}

// ============================================================
// SETUP
// ============================================================

void setup_inclinometer() {
  Serial.begin(115200);

  unsigned long t0 = millis();
  while (!Serial && millis() - t0 < 200) {
    delay(5);
  }

  Wire.begin(SDA_PIN, SCL_PIN);
  EEPROM.begin(EEPROM_SIZE);

  // Restore persisted state
  orientationMode = (OrientationMode)EEPROM.read(EEPROM_ADDR_MODE);
  if (orientationMode > MODE_SCREEN_VERTICAL)
    orientationMode = MODE_SCREEN_UP;
  incidenceMode   = EEPROM.read(EEPROM_ADDR_INCIDENCE);
  EEPROM.get(EEPROM_ADDR_ALIGN,     align_roll);
  EEPROM.get(EEPROM_ADDR_ALIGN + 4, align_pitch);

  Serial.println("\nQMI8658 Inclinometer v4.2 (LOCKED)");

  // IMU configuration (kept intentionally conservative)
  imu.begin(Wire, QMI8658_ADDRESS_HIGH);
  imu.setAccelRange(QMI8658_ACCEL_RANGE_2G);
  imu.setAccelODR(QMI8658_ACCEL_ODR_125HZ);
  imu.setAccelUnit_mps2(true);

  imu.setGyroRange(QMI8658_GYRO_RANGE_512DPS);
  imu.setGyroODR(QMI8658_GYRO_ODR_125HZ);
  imu.setGyroUnit_dps(true);

  imu.enableAccel();
  imu.enableGyro();

  // Initial calibration and zeroing
  calibrateOffsets();
  initializeAngles();

  printMode();
  serialWasAttached = (bool)Serial;

  lastTime = millis();
}

// ============================================================
// MAIN LOOP
// ============================================================

void loop_inclinometer() {
  // Print mode once whenever USB serial transitions from detached to attached.
  bool serialNow = (bool)Serial;
  if (serialNow && !serialWasAttached) {
    printMode();
  }
  serialWasAttached = serialNow;

  QMI8658_Data d;
  if (!imu.readSensorData(d)) return;

  // Time delta for gyro integration
  unsigned long now = millis();
  float dt = (now - lastTime) / 1000.0f;
  if (dt <= 0) return;
  lastTime = now;

  // Remap raw sensor data into tool frame
  float ax, ay, az, gx, gy;
  remapAccel(d, ax, ay, az);
  remapGyro(d, gx, gy);

  // Remove sensor bias offsets
  ax -= ax_off; ay -= ay_off; az -= az_off;
  gx -= gx_off; gy -= gy_off;

  // Accelerometer-derived angles (tool frame)
  float roll_acc  = atan2(ay, az) * RAD_TO_DEG;
  float pitch_acc = atan2(-ax, sqrt(ay*ay + az*az)) * RAD_TO_DEG;

  // Complementary filter fusion
  roll_phys  = alpha * (roll_phys  + gx * dt) + (1 - alpha) * roll_acc;
  pitch_phys = alpha * (pitch_phys + gy * dt) + (1 - alpha) * pitch_acc;

  // Roll becomes undefined near ±90° pitch (gimbal geometry)
  // Gradually attenuate roll contribution instead of hard-clamping
  if (fabs(pitch_phys) > 80.0)
    roll_phys *= cos(fabs(pitch_phys) * DEG_TO_RAD);

  // Apply mechanical alignment and user zero reference
  float r = roll_phys  + align_roll  - roll_zero;
  float p = pitch_phys + align_pitch - pitch_zero;

  // DISPLAY-ONLY semantic roll correction
  // (kept separate from alignment and physics)
  if (orientationMode == MODE_SCREEN_UP)
    r = -r;

  // Output
  if (incidenceMode) {
    Serial.print("INCIDENCE: ");
    Serial.println(p, 2);
  } else {
    Serial.print("Roll: ");
    Serial.print(r, 2);
    Serial.print("  Pitch: ");
    Serial.println(p, 2);
  }

  ui_roll  = r;
  ui_pitch = p;

  handleSerial();
  delay(20); // ~50 Hz output rate
}

// ============================================================
// SERIAL COMMAND HANDLING
// ============================================================

void handleSerial() {
  if (!Serial.available()) return;
  char c = Serial.read();

  switch (c) {
    case 'z': setZeroReference(); break;
    case 'c': calibrateOffsets(); initializeAngles(); break;
    case 'u': setOrientation(MODE_SCREEN_UP); break;
    case 'v': setOrientation(MODE_SCREEN_VERTICAL); break;
    case 'i': toggleIncidence(); break;
    case 'A': runAlignmentCalibration(); break;
  }
  // if (c=='z') setZeroReference();
  // if (c=='c') { calibrateOffsets(); initializeAngles(); }
  // if (c=='u') setOrientation(MODE_SCREEN_UP);
  // if (c=='v') setOrientation(MODE_SCREEN_VERTICAL);
  // if (c=='i') toggleIncidence();
  // if (c=='A') runAlignmentCalibration();
}

// ============================================================
// ALIGNMENT v4.2 — 6 ORIENTATIONS (ANGLE DOMAIN ONLY)
// ============================================================
//
// Uses redundant measurements to estimate CONSTANT angle bias.
// Does NOT:
//   - rotate vectors
//   - estimate gain
//   - affect remapping or filter behavior
//

void captureAngles(float& r, float& p) {
  const int N = 50;        // number of samples
  const int dt_ms = 10;   // sample interval (~500 ms total)

  float sr = 0;
  float sp = 0;

  // Allow system to settle
  delay(200);

  for (int i = 0; i < N; i++) {
    sr += roll_phys;
    sp += pitch_phys;
    delay(dt_ms);
  }

  r = sr / N;
  p = sp / N;
}

void alignmentCapture(void)
{
  // TEMPORARY bridge — will be expanded later
  static float dummy_r, dummy_p;
  captureAngles(dummy_r, dummy_p);
}

bool waitForC(unsigned long timeout_ms = 30000) {
  unsigned long t0 = millis();
  while (millis() - t0 < timeout_ms) {
    if (Serial.available() && Serial.read() == 'c') return true;
  }
  return false;
}

// ASSUMPTIONS:
// - Device is placed against reasonably square surfaces
// - Edge placements are within ±2° of nominal
// - Alignment estimates constant angular bias only
void runAlignmentCalibration() {
  Serial.println("\nALIGNMENT v4.2 (6 ORIENTATIONS)");

  float r_up, p_up;
  float r_down, p_down;
  float r_rd;   // right edge down
  float r_ru;   // right edge up (from underneath)
  float p_bd;   // bottom edge down
  float p_td;   // top edge down

  Serial.println("SCREEN UP → press c");
  waitForC(); captureAngles(r_up, p_up);

  Serial.println("SCREEN DOWN → press c");
  waitForC(); captureAngles(r_down, p_down);

  Serial.println("RIGHT EDGE DOWN → press c");
  waitForC(); captureAngles(r_rd, p_up);   // pitch unused

  Serial.println("RIGHT EDGE UP (from underneath) → press c");
  waitForC(); captureAngles(r_ru, p_up);   // pitch unused

  Serial.println("BOTTOM EDGE DOWN → press c");
  waitForC(); captureAngles(r_up, p_bd);   // roll unused

  Serial.println("TOP EDGE DOWN → press c");
  waitForC(); captureAngles(r_up, p_td);   // roll unused

  // Estimate constant angle bias using redundancy & symmetry
  float roll_bias =
    ( r_up
    - r_down
    + (r_rd - 90.0f)
    + (r_ru + 90.0f)
    ) / 4.0f;

  float pitch_bias =
    ( p_up
    - p_down
    + (p_bd - 90.0f)
    + (p_td + 90.0f)
    ) / 4.0f;

  // Store alignment correction
  align_roll  = -roll_bias;
  align_pitch = -pitch_bias;

  EEPROM.put(EEPROM_ADDR_ALIGN,     align_roll);
  EEPROM.put(EEPROM_ADDR_ALIGN + 4, align_pitch);
  EEPROM.commit();

  calibrateOffsets();
  initializeAngles();

  Serial.println("Alignment complete");
}

// ============================================================
// SUPPORT FUNCTIONS
// ============================================================

void calibrateOffsets() {
  float sax=0,say=0,saz=0,sgx=0,sgy=0;
  int n=0;

  for(int i=0;i<500;i++){
    QMI8658_Data d;
    if(!imu.readSensorData(d))continue;
    float ax,ay,az,gx,gy;
    remapAccel(d,ax,ay,az);
    remapGyro(d,gx,gy);
    sax+=ax; say+=ay; saz+=az;
    sgx+=gx; sgy+=gy;
    n++;
    delay(5);
  }

  ax_off=sax/n;
  ay_off=say/n;
  az_off=saz/n-g_ref;
  gx_off=sgx/n;
  gy_off=sgy/n;
}

void initializeAngles() {
  QMI8658_Data d;
  imu.readSensorData(d);
  float ax,ay,az;
  remapAccel(d,ax,ay,az);
  ax-=ax_off; ay-=ay_off; az-=az_off;
  roll_phys=atan2(ay,az)*RAD_TO_DEG;
  pitch_phys=atan2(-ax,sqrt(ay*ay+az*az))*RAD_TO_DEG;
}

void setZeroReference() {
  roll_zero  = roll_phys  + align_roll;
  pitch_zero = pitch_phys + align_pitch;
}

void setOrientation(OrientationMode m) {
  orientationMode = m;
  EEPROM.write(EEPROM_ADDR_MODE, m);
  EEPROM.commit();

  printMode();

  calibrateOffsets();
  initializeAngles();
}

void toggleIncidence() {
  incidenceMode=!incidenceMode;
  EEPROM.write(EEPROM_ADDR_INCIDENCE,incidenceMode);
  EEPROM.commit();
}

void printMode() {
  Serial.println("\n=== MODE ===");
  Serial.print("Orientation: ");
  Serial.println(orientationMode == MODE_SCREEN_VERTICAL ? "SCREEN VERTICAL" : "SCREEN UP");
  Serial.println("Roll:  + = right edge down");
  Serial.println("Pitch: + = screen tilts toward you");
  Serial.println("===================");
  delay(1000);
}
