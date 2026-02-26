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
#include "fw_version.h"

// ============================================================
// CONFIGURATION
// ============================================================

#define SDA_PIN 40
#define SCL_PIN 39
#define BOOT_BUTTON_PIN 0

#define EEPROM_SIZE 128
#define EEPROM_ADDR_MODE       0
#define EEPROM_ADDR_ROTATION   2
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
bool displayRotated = false;
static bool freezeActive = false;
static float freeze_roll = 0.0f;
static float freeze_pitch = 0.0f;

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
static bool bootBtnStablePressed = false;
static bool bootBtnRawPressed = false;
static unsigned long bootBtnLastChangeMs = 0;
static unsigned long bootBtnPressStartMs = 0;
static bool bootHoldActive = false;
static unsigned long bootHoldMs = 0;
static const unsigned long bootBtnDebounceMs = 30;
static const unsigned long bootBtnLongPressMs = 1200;
static const unsigned long bootBtnVeryLongPressMs = 2200;
static bool modePending = false;
static bool modeConfirmed = false;
static OrientationMode modeTargetState = MODE_SCREEN_UP;
static unsigned long modeStartMs = 0;
static float modeRefRoll = 0.0f;
static float modeRefPitch = 0.0f;
static const unsigned long modeStillMs = 1500;
static const float modeMotionThreshDeg = 0.8f;

enum AlignmentStep {
  ALIGN_SCREEN_UP = 0,
  ALIGN_SCREEN_DOWN,
  ALIGN_RIGHT_EDGE_DOWN,
  ALIGN_RIGHT_EDGE_UP,
  ALIGN_BOTTOM_EDGE_DOWN,
  ALIGN_TOP_EDGE_DOWN,
  ALIGN_STEP_COUNT
};

struct AlignmentWorkflowState {
  bool active;
  AlignmentStep step;
  float r_up, p_up;
  float r_down, p_down;
  float r_rd;
  float r_ru;
  float p_bd;
  float p_td;
};

static AlignmentWorkflowState alignState = {
  false,
  ALIGN_SCREEN_UP,
  0, 0, 0, 0, 0, 0, 0, 0
};

// Forward declarations (required now that this file is compiled as C++)
void calibrateOffsets();
void initializeAngles();
void printMode();
void handleSerial();
void captureAngles(float& r, float& p);
const char *alignmentStepText(AlignmentStep step);
const char *alignmentStepHintText(AlignmentStep step);
void printAlignmentInstruction();
void finalizeAlignment();
void handleBootButton();
void processModeWorkflow();

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
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
  EEPROM.begin(EEPROM_SIZE);

  // Restore persisted state
  orientationMode = (OrientationMode)EEPROM.read(EEPROM_ADDR_MODE);
  if (orientationMode > MODE_SCREEN_VERTICAL)
    orientationMode = MODE_SCREEN_UP;
  displayRotated = EEPROM.read(EEPROM_ADDR_ROTATION) ? true : false;
  EEPROM.get(EEPROM_ADDR_ALIGN,     align_roll);
  EEPROM.get(EEPROM_ADDR_ALIGN + 4, align_pitch);

  Serial.print("\nQMI8658 Inclinometer FW ");
  Serial.println(FW_VERSION);
  Serial.println("BOOT button mapped to GPIO0 (active-low)");

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

  if (freezeActive) {
    r = freeze_roll;
    p = freeze_pitch;
  }

  processModeWorkflow();

  // Output
  if (!alignmentIsActive() && !modePending) {
    switch (ui_axis_mode) {
      case AXIS_ROLL:
        Serial.print("Roll: ");
        Serial.println(r, 2);
        break;
      case AXIS_PITCH:
        Serial.print("Pitch: ");
        Serial.println(p, 2);
        break;
      default:
        Serial.print("Roll: ");
        Serial.print(r, 2);
        Serial.print("  Pitch: ");
        Serial.println(p, 2);
        break;
    }
  }

  ui_roll  = r;
  ui_pitch = p;

  handleSerial();
  handleBootButton();
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
    case 'c':
      if (alignmentIsActive()) alignmentCapture();
      else if (modePending && !modeConfirmed) modeWorkflowConfirm();
      else { calibrateOffsets(); initializeAngles(); }
      break;
    case 'C': alignmentStart(); break;
    case 'u': modeWorkflowStart(MODE_SCREEN_UP); break;
    case 'v': modeWorkflowStart(MODE_SCREEN_VERTICAL); break;
    case 'm': modeWorkflowStartToggle(); break;
    case 'x': modeWorkflowCancel(); break;
    case 'a': cycleAxisMode(); break;
    case 'r': toggleRotation(); break;

  }
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
  if (!alignState.active) {
    return;
  }

  float r = 0.0f;
  float p = 0.0f;
  captureAngles(r, p);

  switch (alignState.step) {
    case ALIGN_SCREEN_UP:
      alignState.r_up = r;
      alignState.p_up = p;
      break;
    case ALIGN_SCREEN_DOWN:
      alignState.r_down = r;
      alignState.p_down = p;
      break;
    case ALIGN_RIGHT_EDGE_DOWN:
      alignState.r_rd = r;
      break;
    case ALIGN_RIGHT_EDGE_UP:
      alignState.r_ru = r;
      break;
    case ALIGN_BOTTOM_EDGE_DOWN:
      alignState.p_bd = p;
      break;
    case ALIGN_TOP_EDGE_DOWN:
      alignState.p_td = p;
      break;
    default:
      break;
  }

  Serial.print("Captured: ");
  Serial.println(alignmentStepText(alignState.step));
  Serial.print("  roll_phys=");
  Serial.print(r, 2);
  Serial.print("  pitch_phys=");
  Serial.println(p, 2);

  if (alignState.step == ALIGN_TOP_EDGE_DOWN) {
    finalizeAlignment();
    return;
  }

  alignState.step = (AlignmentStep)((int)alignState.step + 1);
  printAlignmentInstruction();
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

bool alignmentIsActive(void) {
  return alignState.active;
}

const char *alignmentStepText(AlignmentStep step) {
  switch (step) {
    case ALIGN_SCREEN_UP:         return "SCREEN UP";
    case ALIGN_SCREEN_DOWN:       return "SCREEN DOWN";
    case ALIGN_RIGHT_EDGE_DOWN:   return "RIGHT EDGE DOWN";
    case ALIGN_RIGHT_EDGE_UP:     return "RIGHT EDGE UP";
    case ALIGN_BOTTOM_EDGE_DOWN:  return "BOTTOM EDGE DOWN";
    case ALIGN_TOP_EDGE_DOWN:     return "TOP EDGE DOWN";
    default:                      return "";
  }
}

const char *alignmentStepHintText(AlignmentStep step) {
  if (step == ALIGN_RIGHT_EDGE_UP) {
    return "Tip: use table underside";
  }
  return "";
}

void printAlignmentInstruction() {
  if (!alignState.active) return;
  Serial.println();
  Serial.print("ALIGN ");
  Serial.print((int)alignState.step + 1);
  Serial.print("/");
  Serial.println((int)ALIGN_STEP_COUNT);
  Serial.print("Place tool: ");
  Serial.println(alignmentStepText(alignState.step));
  const char *hint = alignmentStepHintText(alignState.step);
  if (hint[0] != '\0') {
    Serial.println(hint);
  }
  Serial.println("Press CAPTURE (touch) or send 'c' (serial)");
  Serial.println("Tip: BOOT = CAPTURE");
}

void alignmentGetInstruction(char *buf, unsigned int buf_size) {
  if (!buf || buf_size == 0) return;
  if (!alignState.active) {
    buf[0] = '\0';
    return;
  }
  const char *boot_hint = "Tip: BOOT = CAPTURE";
  const char *hint = alignmentStepHintText(alignState.step);
  if (hint[0] != '\0') {
    snprintf(
      buf,
      buf_size,
      "ALIGN %d/%d\nPlace tool: %s\n%s\n%s\nPress CAPTURE",
      (int)alignState.step + 1,
      (int)ALIGN_STEP_COUNT,
      alignmentStepText(alignState.step),
      hint,
      boot_hint
    );
    return;
  }
  snprintf(
    buf,
    buf_size,
    "ALIGN %d/%d\nPlace tool: %s\n%s\nPress CAPTURE",
    (int)alignState.step + 1,
    (int)ALIGN_STEP_COUNT,
    alignmentStepText(alignState.step),
    boot_hint
  );
}

void alignmentStart(void) {
  if (alignState.active) {
    printAlignmentInstruction();
    return;
  }

  alignState.active = true;
  alignState.step = ALIGN_SCREEN_UP;
  alignState.r_up = 0.0f;
  alignState.p_up = 0.0f;
  alignState.r_down = 0.0f;
  alignState.p_down = 0.0f;
  alignState.r_rd = 0.0f;
  alignState.r_ru = 0.0f;
  alignState.p_bd = 0.0f;
  alignState.p_td = 0.0f;

  Serial.print("\nALIGNMENT FW ");
  Serial.print(FW_VERSION);
  Serial.println(" (6 ORIENTATIONS)");
  printAlignmentInstruction();
}

void alignmentCancel(void) {
  if (!alignState.active) return;
  alignState.active = false;
  Serial.println("Alignment canceled");
}

void modeWorkflowStart(OrientationMode target) {
  modePending = true;
  modeConfirmed = false;
  modeTargetState = target;

  Serial.println();
  Serial.println("MODE WORKFLOW");
  Serial.print("Position with ");
  Serial.println(target == MODE_SCREEN_VERTICAL ? "SCREEN VERTICAL" : "SCREEN UP");
  Serial.println("Send 'c' to CONFIRM and hold still");
  Serial.println("Send 'x' to CANCEL");
}

void modeWorkflowStartToggle(void) {
  modeWorkflowStart(
    orientationMode == MODE_SCREEN_VERTICAL ? MODE_SCREEN_UP : MODE_SCREEN_VERTICAL
  );
}

void modeWorkflowConfirm() {
  if (!modePending) return;
  if (modeConfirmed) return;

  modeConfirmed = true;
  modeStartMs = millis();
  modeRefRoll = roll_phys;
  modeRefPitch = pitch_phys;
  Serial.println("MODE CONFIRMED. Hold still...");
}

void modeWorkflowCancel() {
  if (!modePending) return;
  modePending = false;
  modeConfirmed = false;
  Serial.println("Mode change canceled");
}

void processModeWorkflow() {
  if (!modePending || !modeConfirmed) return;

  const unsigned long now = millis();
  const float dr = fabsf(roll_phys - modeRefRoll);
  const float dp = fabsf(pitch_phys - modeRefPitch);
  if (dr > modeMotionThreshDeg || dp > modeMotionThreshDeg) {
    modeStartMs = now;
    modeRefRoll = roll_phys;
    modeRefPitch = pitch_phys;
    return;
  }

  if ((now - modeStartMs) >= modeStillMs) {
    modePending = false;
    modeConfirmed = false;
    setOrientation(modeTargetState);
    Serial.println("Mode change complete");
  }
}

bool modeWorkflowIsActive(void) {
  return modePending;
}

bool modeWorkflowIsConfirmed(void) {
  return modeConfirmed;
}

OrientationMode modeWorkflowTarget(void) {
  return modeTargetState;
}

float modeWorkflowRemainingSeconds(void) {
  if (!modePending || !modeConfirmed) return 0.0f;
  const unsigned long now = millis();
  const unsigned long elapsed = now - modeStartMs;
  if (elapsed >= modeStillMs) return 0.0f;
  return (float)(modeStillMs - elapsed) / 1000.0f;
}

void finalizeAlignment() {
  float roll_bias =
    ( alignState.r_up
    - alignState.r_down
    + (alignState.r_rd - 90.0f)
    + (alignState.r_ru + 90.0f)
    ) / 4.0f;

  float pitch_bias =
    ( alignState.p_up
    - alignState.p_down
    + (alignState.p_bd - 90.0f)
    + (alignState.p_td + 90.0f)
    ) / 4.0f;

  align_roll  = -roll_bias;
  align_pitch = -pitch_bias;

  EEPROM.put(EEPROM_ADDR_ALIGN,     align_roll);
  EEPROM.put(EEPROM_ADDR_ALIGN + 4, align_pitch);
  EEPROM.commit();

  calibrateOffsets();
  initializeAngles();

  alignState.active = false;
  Serial.println("Alignment complete");
  Serial.print("  align_roll=");
  Serial.print(align_roll, 3);
  Serial.print("  align_pitch=");
  Serial.println(align_pitch, 3);
}

void toggleRotation() {
  displayRotated = !displayRotated;
  EEPROM.write(EEPROM_ADDR_ROTATION, displayRotated ? 1 : 0);
  EEPROM.commit();
  Serial.print("Display rotation: ");
  Serial.println(displayRotated ? "180" : "0");
}

void toggleMeasurementFreeze() {
  freezeActive = !freezeActive;
  if (freezeActive) {
    float r = roll_phys  + align_roll  - roll_zero;
    float p = pitch_phys + align_pitch - pitch_zero;
    if (orientationMode == MODE_SCREEN_UP) {
      r = -r;
    }
    freeze_roll = r;
    freeze_pitch = p;
  }
}

bool measurementIsFrozen(void) {
  return freezeActive;
}

bool bootHoldIsActive(void) {
  return bootHoldActive;
}

unsigned long bootHoldDurationMs(void) {
  return bootHoldMs;
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

void handleBootButton() {
  const bool rawPressed = (digitalRead(BOOT_BUTTON_PIN) == LOW);
  const unsigned long now = millis();

  if (rawPressed != bootBtnRawPressed) {
    bootBtnRawPressed = rawPressed;
    bootBtnLastChangeMs = now;
  }

  if ((now - bootBtnLastChangeMs) < bootBtnDebounceMs) {
    return;
  }

  if (bootBtnStablePressed != bootBtnRawPressed) {
    bootBtnStablePressed = bootBtnRawPressed;

    if (bootBtnStablePressed) {
      bootBtnPressStartMs = now;
      bootHoldActive = true;
      bootHoldMs = 0;
    } else {
      // Trigger on release to avoid repeat while held.
      unsigned long pressMs = now - bootBtnPressStartMs;
      bootHoldActive = false;
      bootHoldMs = 0;
      if (alignmentIsActive()) {
        alignmentCapture();
      } else if (modeWorkflowIsActive()) {
        if (pressMs >= bootBtnLongPressMs) {
          modeWorkflowCancel();
        } else {
          modeWorkflowConfirm();
        }
      } else {
        if (pressMs >= bootBtnVeryLongPressMs) {
          modeWorkflowStartToggle();
        } else if (pressMs >= bootBtnLongPressMs) {
          cycleAxisMode();
        } else {
          toggleMeasurementFreeze();
        }
      }
    }
  }

  if (bootBtnStablePressed) {
    bootHoldActive = true;
    bootHoldMs = now - bootBtnPressStartMs;
  }
}
