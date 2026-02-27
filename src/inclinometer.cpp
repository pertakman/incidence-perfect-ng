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
#define EEPROM_ADDR_BIAS_UP_MAGIC    24
#define EEPROM_ADDR_BIAS_UP          28   // stores: ax,ay,az,gx,gy (floats)
#define EEPROM_ADDR_BIAS_VERT_MAGIC  52
#define EEPROM_ADDR_BIAS_VERT        56   // stores: ax,ay,az,gx,gy (floats)
#define EEPROM_ADDR_ZERO_UP_MAGIC    80
#define EEPROM_ADDR_ZERO_UP          84   // stores: roll_zero, pitch_zero (floats)
#define EEPROM_ADDR_ZERO_VERT_MAGIC  92
#define EEPROM_ADDR_ZERO_VERT        96   // stores: roll_zero, pitch_zero (floats)
static const uint32_t EEPROM_BIAS_MAGIC = 0x42534131UL; // "BSA1"
static const uint32_t EEPROM_ZERO_MAGIC = 0x5A455231UL; // "ZER1"

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
static const unsigned long bootBtnUltraLongPressMs = 3200;
static bool modePending = false;
static bool modeConfirmed = false;
static OrientationMode modeTargetState = MODE_SCREEN_UP;
static unsigned long modeStartMs = 0;
static float modeRefRoll = 0.0f;
static float modeRefPitch = 0.0f;
static const unsigned long modeStillMs = 1500;
static const float modeMotionThreshDeg = 0.8f;
static bool recalPending = false;
static bool recalConfirmed = false;
static unsigned long recalStartMs = 0;
static float recalRefRoll = 0.0f;
static float recalRefPitch = 0.0f;
static bool recalSampling = false;
static int recalSampleCount = 0;
static const int recalSampleTarget = 125; // ~2.5 s at 20 ms loop
static float recalSumAx = 0.0f;
static float recalSumAy = 0.0f;
static float recalSumAz = 0.0f;
static float recalSumGx = 0.0f;
static float recalSumGy = 0.0f;
static bool zeroPending = false;
static bool zeroConfirmed = false;
static bool zeroSampling = false;
static unsigned long zeroStartMs = 0;
static float zeroRefRoll = 0.0f;
static float zeroRefPitch = 0.0f;
static int zeroSampleCount = 0;
static const int zeroSampleTarget = 40; // ~0.8 s at 20 ms loop
static float zeroSumRoll = 0.0f;
static float zeroSumPitch = 0.0f;
static const unsigned long zeroStillMs = 1000;
static const float zeroMotionThreshDeg = 0.4f;
static float lastRawAx = 0.0f;
static float lastRawAy = 0.0f;
static float lastRawAz = 0.0f;
static float lastRawGx = 0.0f;
static float lastRawGy = 0.0f;
static bool rawStreamEnabled = false;
static unsigned long rawStreamLastMs = 0;

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

struct AlignmentCaptureState {
  bool active;
  AlignmentStep step;
  int settleTicksRemaining;
  int sampleTarget;
  int sampleCount;
  float sumRoll;
  float sumPitch;
};

static AlignmentCaptureState alignCaptureState = {
  false,
  ALIGN_SCREEN_UP,
  0,
  0,
  0,
  0.0f,
  0.0f
};

static const int alignCaptureSettleTicks = 10;  // ~200 ms @ 20 ms loop
static const int alignCaptureSampleTarget = 25; // ~500 ms @ 20 ms loop

// Forward declarations (required now that this file is compiled as C++)
void calibrateOffsets();
void initializeAngles();
bool loadBiasOffsetsFromEeprom(OrientationMode mode);
void saveBiasOffsetsToEeprom(OrientationMode mode);
bool loadZeroReferenceFromEeprom(OrientationMode mode);
void saveZeroReferenceToEeprom(OrientationMode mode);
void printMode();
void printRawImuSample(const QMI8658_Data &d, float ax, float ay, float az, float gx, float gy);
void handleSerial();
const char *alignmentStepText(AlignmentStep step);
const char *alignmentStepHintText(AlignmentStep step);
void printAlignmentInstruction();
void finalizeAlignment();
void handleBootButton();
void processModeWorkflow();
void processAlignmentCapture();
void processRecalibrationWorkflow();
void processZeroWorkflow();
void recalibrationWorkflowStart();
void recalibrationWorkflowConfirm();
void recalibrationWorkflowCancel();

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
  Serial.println("ACTION button mapped to GPIO0 (BOOT, active-low)");

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
  if (!loadBiasOffsetsFromEeprom(orientationMode)) {
    calibrateOffsets();
    saveBiasOffsetsToEeprom(orientationMode);
  } else {
    Serial.println("Loaded bias offsets from EEPROM");
  }
  if (!loadZeroReferenceFromEeprom(orientationMode)) {
    roll_zero = 0.0f;
    pitch_zero = 0.0f;
  } else {
    Serial.println("Loaded zero reference from EEPROM");
  }
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
  lastRawAx = ax;
  lastRawAy = ay;
  lastRawAz = az;
  lastRawGx = gx;
  lastRawGy = gy;

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
  processZeroWorkflow();
  processRecalibrationWorkflow();
  processAlignmentCapture();

  // Output
  if (rawStreamEnabled) {
    if ((now - rawStreamLastMs) >= 200) { // 5 Hz debug stream
      printRawImuSample(d, ax, ay, az, gx, gy);
      rawStreamLastMs = now;
    }
  } else if (!alignmentIsActive() && !modePending && !zeroPending && !recalPending) {
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
    case 'z':
      Serial.println("Serial 'z': start guided zero");
      zeroWorkflowStart();
      break;
    case 'c':
      if (alignmentIsActive()) {
        Serial.println("Serial 'c': CAPTURE");
        alignmentCapture();
      } else if (modePending && !modeConfirmed) {
        Serial.println("Serial 'c': CONFIRM mode workflow");
        modeWorkflowConfirm();
      } else if (zeroPending && !zeroConfirmed) {
        Serial.println("Serial 'c': CONFIRM zero workflow");
        zeroWorkflowConfirm();
      } else if (recalPending && !recalConfirmed) {
        Serial.println("Serial 'c': CONFIRM recalibration workflow");
        recalibrationWorkflowConfirm();
      } else {
        Serial.println("Serial 'c': guided recalibration start+confirm");
        recalibrationWorkflowStart();
        recalibrationWorkflowConfirm();
      }
      break;
    case 'C': alignmentStart(); break;
    case 'u': modeWorkflowStart(MODE_SCREEN_UP); break;
    case 'v': modeWorkflowStart(MODE_SCREEN_VERTICAL); break;
    case 'm': modeWorkflowStartToggle(); break;
    case 'x':
      modeWorkflowCancel();
      zeroWorkflowCancel();
      recalibrationWorkflowCancel();
      alignmentCancel();
      break;
    case 'k': recalibrationWorkflowStart(); break;
    case 'a': cycleAxisMode(); break;
    case 'r': toggleRotation(); break;
    case 'd':
      rawStreamEnabled = !rawStreamEnabled;
      rawStreamLastMs = 0;
      Serial.print("Raw IMU stream: ");
      Serial.println(rawStreamEnabled ? "ON (5 Hz)" : "OFF");
      break;

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

void alignmentCapture(void)
{
  if (!alignState.active) {
    return;
  }
  if (alignCaptureState.active) {
    return;
  }
  alignCaptureState.active = true;
  alignCaptureState.step = alignState.step;
  alignCaptureState.settleTicksRemaining = alignCaptureSettleTicks;
  alignCaptureState.sampleTarget = alignCaptureSampleTarget;
  alignCaptureState.sampleCount = 0;
  alignCaptureState.sumRoll = 0.0f;
  alignCaptureState.sumPitch = 0.0f;
  Serial.println("Capturing...");
}

void processAlignmentCapture() {
  if (!alignCaptureState.active) return;

  if (alignCaptureState.settleTicksRemaining > 0) {
    alignCaptureState.settleTicksRemaining--;
    return;
  }

  alignCaptureState.sumRoll += roll_phys;
  alignCaptureState.sumPitch += pitch_phys;
  alignCaptureState.sampleCount++;

  if (alignCaptureState.sampleCount < alignCaptureState.sampleTarget) return;

  const float r = alignCaptureState.sumRoll / (float)alignCaptureState.sampleTarget;
  const float p = alignCaptureState.sumPitch / (float)alignCaptureState.sampleTarget;
  const AlignmentStep step = alignCaptureState.step;
  alignCaptureState.active = false;

  switch (step) {
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
  Serial.println(alignmentStepText(step));
  Serial.print("  roll_phys=");
  Serial.print(r, 2);
  Serial.print("  pitch_phys=");
  Serial.println(p, 2);

  if (step == ALIGN_TOP_EDGE_DOWN) {
    finalizeAlignment();
    return;
  }

  alignState.step = (AlignmentStep)((int)step + 1);
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

bool loadBiasOffsetsFromEeprom(OrientationMode mode) {
  const int magic_addr = (mode == MODE_SCREEN_VERTICAL)
    ? EEPROM_ADDR_BIAS_VERT_MAGIC
    : EEPROM_ADDR_BIAS_UP_MAGIC;
  const int bias_addr = (mode == MODE_SCREEN_VERTICAL)
    ? EEPROM_ADDR_BIAS_VERT
    : EEPROM_ADDR_BIAS_UP;

  uint32_t magic = 0;
  EEPROM.get(magic_addr, magic);
  if (magic != EEPROM_BIAS_MAGIC) return false;

  EEPROM.get(bias_addr + 0,  ax_off);
  EEPROM.get(bias_addr + 4,  ay_off);
  EEPROM.get(bias_addr + 8,  az_off);
  EEPROM.get(bias_addr + 12, gx_off);
  EEPROM.get(bias_addr + 16, gy_off);
  return true;
}

void saveBiasOffsetsToEeprom(OrientationMode mode) {
  const int magic_addr = (mode == MODE_SCREEN_VERTICAL)
    ? EEPROM_ADDR_BIAS_VERT_MAGIC
    : EEPROM_ADDR_BIAS_UP_MAGIC;
  const int bias_addr = (mode == MODE_SCREEN_VERTICAL)
    ? EEPROM_ADDR_BIAS_VERT
    : EEPROM_ADDR_BIAS_UP;

  EEPROM.put(bias_addr + 0,  ax_off);
  EEPROM.put(bias_addr + 4,  ay_off);
  EEPROM.put(bias_addr + 8,  az_off);
  EEPROM.put(bias_addr + 12, gx_off);
  EEPROM.put(bias_addr + 16, gy_off);
  EEPROM.put(magic_addr, EEPROM_BIAS_MAGIC);
  EEPROM.commit();
}

bool loadZeroReferenceFromEeprom(OrientationMode mode) {
  const int magic_addr = (mode == MODE_SCREEN_VERTICAL)
    ? EEPROM_ADDR_ZERO_VERT_MAGIC
    : EEPROM_ADDR_ZERO_UP_MAGIC;
  const int zero_addr = (mode == MODE_SCREEN_VERTICAL)
    ? EEPROM_ADDR_ZERO_VERT
    : EEPROM_ADDR_ZERO_UP;

  uint32_t magic = 0;
  EEPROM.get(magic_addr, magic);
  if (magic != EEPROM_ZERO_MAGIC) return false;

  EEPROM.get(zero_addr + 0, roll_zero);
  EEPROM.get(zero_addr + 4, pitch_zero);
  return true;
}

void saveZeroReferenceToEeprom(OrientationMode mode) {
  const int magic_addr = (mode == MODE_SCREEN_VERTICAL)
    ? EEPROM_ADDR_ZERO_VERT_MAGIC
    : EEPROM_ADDR_ZERO_UP_MAGIC;
  const int zero_addr = (mode == MODE_SCREEN_VERTICAL)
    ? EEPROM_ADDR_ZERO_VERT
    : EEPROM_ADDR_ZERO_UP;

  EEPROM.put(zero_addr + 0, roll_zero);
  EEPROM.put(zero_addr + 4, pitch_zero);
  EEPROM.put(magic_addr, EEPROM_ZERO_MAGIC);
  EEPROM.commit();
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
  saveZeroReferenceToEeprom(orientationMode);
}

void zeroWorkflowStart(void) {
  if (alignmentIsActive()) {
    Serial.println("Zero blocked: ALIGN active");
    return;
  }
  modeWorkflowCancel();
  recalibrationWorkflowCancel();
  zeroPending = true;
  zeroConfirmed = false;
  zeroSampling = false;
  zeroSampleCount = 0;
  zeroSumRoll = 0.0f;
  zeroSumPitch = 0.0f;

  Serial.println();
  Serial.println("Zero workflow");
  Serial.println("Press CONFIRM and hold still");
  Serial.println("Send 'c' to CONFIRM, 'x' to cancel");
  Serial.println("Tip: ACTION short=CONFIRM, long=CANCEL");
}

void zeroWorkflowConfirm(void) {
  if (!zeroPending || zeroConfirmed) return;
  zeroConfirmed = true;
  zeroSampling = false;
  zeroSampleCount = 0;
  zeroStartMs = millis();
  zeroRefRoll = roll_phys + align_roll;
  zeroRefPitch = pitch_phys + align_pitch;
  Serial.println("Zero confirmed. Hold still...");
}

void zeroWorkflowCancel(void) {
  if (!zeroPending) return;
  zeroPending = false;
  zeroConfirmed = false;
  zeroSampling = false;
  zeroSampleCount = 0;
  Serial.println("Zero canceled");
}

void processZeroWorkflow() {
  if (!zeroPending || !zeroConfirmed) return;

  const unsigned long now = millis();
  const float curRoll = roll_phys + align_roll;
  const float curPitch = pitch_phys + align_pitch;

  if (!zeroSampling) {
    const float dr = fabsf(curRoll - zeroRefRoll);
    const float dp = fabsf(curPitch - zeroRefPitch);
    if (dr > zeroMotionThreshDeg || dp > zeroMotionThreshDeg) {
      zeroStartMs = now;
      zeroRefRoll = curRoll;
      zeroRefPitch = curPitch;
      return;
    }
    if ((now - zeroStartMs) >= zeroStillMs) {
      zeroSampling = true;
      zeroSampleCount = 0;
      zeroSumRoll = 0.0f;
      zeroSumPitch = 0.0f;
      Serial.println("Zero sampling...");
    }
    return;
  }

  zeroSumRoll += curRoll;
  zeroSumPitch += curPitch;
  zeroSampleCount++;

  if (zeroSampleCount < zeroSampleTarget) return;

  roll_zero = zeroSumRoll / (float)zeroSampleTarget;
  pitch_zero = zeroSumPitch / (float)zeroSampleTarget;
  saveZeroReferenceToEeprom(orientationMode);

  zeroPending = false;
  zeroConfirmed = false;
  zeroSampling = false;
  zeroSampleCount = 0;
  Serial.println("Zero complete");
}

bool zeroWorkflowIsActive(void) {
  return zeroPending;
}

bool zeroWorkflowIsConfirmed(void) {
  return zeroConfirmed;
}

float zeroWorkflowRemainingSeconds(void) {
  if (!zeroPending || !zeroConfirmed) return 0.0f;
  if (!zeroSampling) {
    const unsigned long now = millis();
    const unsigned long elapsed = now - zeroStartMs;
    if (elapsed >= zeroStillMs) return ((float)zeroSampleTarget * 0.02f);
    return ((float)(zeroStillMs - elapsed) / 1000.0f) + ((float)zeroSampleTarget * 0.02f);
  }
  const int rem = zeroSampleTarget - zeroSampleCount;
  if (rem <= 0) return 0.0f;
  return (float)rem * 0.02f;
}

float zeroWorkflowProgressPercent(void) {
  if (!zeroPending || !zeroConfirmed) return 0.0f;
  if (!zeroSampling) {
    const unsigned long now = millis();
    const unsigned long elapsed = now - zeroStartMs;
    if (elapsed >= zeroStillMs) return 40.0f;
    return 40.0f * ((float)elapsed / (float)zeroStillMs);
  }
  float frac = (float)zeroSampleCount / (float)zeroSampleTarget;
  if (frac > 1.0f) frac = 1.0f;
  return 40.0f + 60.0f * frac;
}

void setOrientation(OrientationMode m) {
  orientationMode = m;
  EEPROM.write(EEPROM_ADDR_MODE, m);
  EEPROM.commit();

  printMode();

  if (!loadBiasOffsetsFromEeprom(orientationMode)) {
    calibrateOffsets();
    saveBiasOffsetsToEeprom(orientationMode);
  }
  if (!loadZeroReferenceFromEeprom(orientationMode)) {
    roll_zero = 0.0f;
    pitch_zero = 0.0f;
  }
  initializeAngles();
}

void runQuickRecalibration(void) {
  calibrateOffsets();
  saveBiasOffsetsToEeprom(orientationMode);
  initializeAngles();
  Serial.println("Quick recalibration complete");
}

bool alignmentIsActive(void) {
  return alignState.active;
}

bool alignmentCaptureInProgress(void) {
  return alignCaptureState.active;
}

float alignmentCaptureProgressPercent(void) {
  if (!alignCaptureState.active) return 0.0f;
  const int totalTicks = alignCaptureSettleTicks + alignCaptureSampleTarget;
  const int doneTicks =
    (alignCaptureSettleTicks - alignCaptureState.settleTicksRemaining) +
    alignCaptureState.sampleCount;
  if (doneTicks <= 0) return 0.0f;
  if (doneTicks >= totalTicks) return 100.0f;
  return ((float)doneTicks * 100.0f) / (float)totalTicks;
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
  Serial.println("Tip: ACTION button = CAPTURE");
}

void alignmentGetInstruction(char *buf, unsigned int buf_size) {
  if (!buf || buf_size == 0) return;
  if (!alignState.active) {
    buf[0] = '\0';
    return;
  }
  if (alignCaptureState.active) {
    const int pct = (int)alignmentCaptureProgressPercent();
    snprintf(buf, buf_size, "Capturing... %d%%", pct);
    return;
  }
  const char *boot_hint = "Tip: ACTION button = CAPTURE";
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
  modeWorkflowCancel();
  zeroWorkflowCancel();
  recalibrationWorkflowCancel();
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
  alignCaptureState.active = false;

  Serial.print("\nAlignment FW ");
  Serial.print(FW_VERSION);
  Serial.println(" (6 orientations)");
  printAlignmentInstruction();
}

void alignmentCancel(void) {
  if (!alignState.active) return;
  alignCaptureState.active = false;
  alignState.active = false;
  Serial.println("Alignment canceled");
}

void modeWorkflowStart(OrientationMode target) {
  zeroWorkflowCancel();
  recalibrationWorkflowCancel();
  modePending = false;
  modeConfirmed = false;
  modeTargetState = target;
  setOrientation(target);
  Serial.println("Mode change complete");
}

void modeWorkflowStartToggle(void) {
  modeWorkflowStart(
    orientationMode == MODE_SCREEN_VERTICAL ? MODE_SCREEN_UP : MODE_SCREEN_VERTICAL
  );
}

void modeWorkflowConfirm() {
  // MODE is immediate in current UX; confirm kept as a no-op for compatibility.
  (void)modePending;
  (void)modeConfirmed;
}

void modeWorkflowCancel() {
  if (!modePending) return;
  modePending = false;
  modeConfirmed = false;
  Serial.println("Mode change canceled");
}

void processModeWorkflow() {
  // MODE is immediate; no async workflow processing required.
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
  return 0.0f;
}

float modeWorkflowProgressPercent(void) {
  return 0.0f;
}

void recalibrationWorkflowStart(void) {
  modeWorkflowCancel();
  zeroWorkflowCancel();
  recalPending = true;
  recalConfirmed = false;
  recalSampling = false;
  recalSampleCount = 0;
  recalSumAx = recalSumAy = recalSumAz = 0.0f;
  recalSumGx = recalSumGy = 0.0f;

  Serial.println();
  Serial.println("Recalibration workflow");
  Serial.print("Reposition with ");
  Serial.println(orientationMode == MODE_SCREEN_VERTICAL ? "SCREEN VERTICAL" : "SCREEN UP");
  Serial.println("Send 'c' to CONFIRM and hold still");
  Serial.println("Send 'x' to cancel");
}

void recalibrationWorkflowConfirm(void) {
  if (!recalPending || recalConfirmed) return;
  recalConfirmed = true;
  recalSampling = false;
  recalSampleCount = 0;
  recalStartMs = millis();
  recalRefRoll = roll_phys;
  recalRefPitch = pitch_phys;
  Serial.println("Recalibration confirmed. Hold still...");
}

void recalibrationWorkflowCancel(void) {
  if (!recalPending) return;
  recalPending = false;
  recalConfirmed = false;
  recalSampling = false;
  recalSampleCount = 0;
  Serial.println("Recalibration canceled");
}

void processRecalibrationWorkflow() {
  if (!recalPending || !recalConfirmed) return;

  const unsigned long now = millis();
  if (!recalSampling) {
    const float dr = fabsf(roll_phys - recalRefRoll);
    const float dp = fabsf(pitch_phys - recalRefPitch);
    if (dr > modeMotionThreshDeg || dp > modeMotionThreshDeg) {
      recalStartMs = now;
      recalRefRoll = roll_phys;
      recalRefPitch = pitch_phys;
      return;
    }
    if ((now - recalStartMs) >= modeStillMs) {
      recalSampling = true;
      recalSampleCount = 0;
      recalSumAx = recalSumAy = recalSumAz = 0.0f;
      recalSumGx = recalSumGy = 0.0f;
      Serial.println("Recalibration sampling...");
    }
    return;
  }

  recalSumAx += lastRawAx;
  recalSumAy += lastRawAy;
  recalSumAz += lastRawAz;
  recalSumGx += lastRawGx;
  recalSumGy += lastRawGy;
  recalSampleCount++;

  if (recalSampleCount < recalSampleTarget) return;

  ax_off = recalSumAx / (float)recalSampleTarget;
  ay_off = recalSumAy / (float)recalSampleTarget;
  az_off = (recalSumAz / (float)recalSampleTarget) - g_ref;
  gx_off = recalSumGx / (float)recalSampleTarget;
  gy_off = recalSumGy / (float)recalSampleTarget;
  saveBiasOffsetsToEeprom(orientationMode);
  initializeAngles();

  recalPending = false;
  recalConfirmed = false;
  recalSampling = false;
  recalSampleCount = 0;
  Serial.println("Recalibration complete");
}

bool recalibrationWorkflowIsActive(void) {
  return recalPending;
}

bool recalibrationWorkflowIsConfirmed(void) {
  return recalConfirmed;
}

float recalibrationWorkflowRemainingSeconds(void) {
  if (!recalPending || !recalConfirmed) return 0.0f;
  if (!recalSampling) {
    const unsigned long now = millis();
    const unsigned long elapsed = now - recalStartMs;
    if (elapsed >= modeStillMs) return ((float)recalSampleTarget * 0.02f);
    return ((float)(modeStillMs - elapsed) / 1000.0f) + ((float)recalSampleTarget * 0.02f);
  }
  const int rem = recalSampleTarget - recalSampleCount;
  if (rem <= 0) return 0.0f;
  return (float)rem * 0.02f;
}

float recalibrationWorkflowProgressPercent(void) {
  if (!recalPending || !recalConfirmed) return 0.0f;
  if (!recalSampling) {
    const unsigned long now = millis();
    const unsigned long elapsed = now - recalStartMs;
    if (elapsed >= modeStillMs) return 40.0f;
    return 40.0f * ((float)elapsed / (float)modeStillMs);
  }
  float frac = (float)recalSampleCount / (float)recalSampleTarget;
  if (frac > 1.0f) frac = 1.0f;
  return 40.0f + 60.0f * frac;
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

  alignState.active = false;
  Serial.println("Alignment complete (offsets saved)");
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

void printRawImuSample(const QMI8658_Data &d, float ax, float ay, float az, float gx, float gy) {
  // d.* are sensor values after library scaling, before project remap/offset removal.
  // ax..gy args here are remapped + offset-corrected values used by the filter.
  float rax, ray, raz, rgx, rgy;
  remapAccel(d, rax, ray, raz);
  remapGyro(d, rgx, rgy);

  Serial.print("RAW ");
  Serial.print(orientationMode == MODE_SCREEN_VERTICAL ? "VERT" : "UP");
  Serial.print(" | sens a=(");
  Serial.print(d.accelX, 3); Serial.print(",");
  Serial.print(d.accelY, 3); Serial.print(",");
  Serial.print(d.accelZ, 3); Serial.print(") g=(");
  Serial.print(d.gyroX, 3); Serial.print(",");
  Serial.print(d.gyroY, 3); Serial.print(",");
  Serial.print(d.gyroZ, 3); Serial.print(")");
  Serial.print(" | map a=(");
  Serial.print(rax, 3); Serial.print(",");
  Serial.print(ray, 3); Serial.print(",");
  Serial.print(raz, 3); Serial.print(") g=(");
  Serial.print(rgx, 3); Serial.print(",");
  Serial.print(rgy, 3); Serial.print(")");
  Serial.print(" | corr a=(");
  Serial.print(ax, 3); Serial.print(",");
  Serial.print(ay, 3); Serial.print(",");
  Serial.print(az, 3); Serial.print(") g=(");
  Serial.print(gx, 3); Serial.print(",");
  Serial.print(gy, 3); Serial.print(")");
  Serial.print(" | angle=(");
  Serial.print(roll_phys, 2); Serial.print(",");
  Serial.print(pitch_phys, 2); Serial.println(")");
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
      } else if (zeroWorkflowIsActive()) {
        if (pressMs >= bootBtnLongPressMs) {
          zeroWorkflowCancel();
        } else {
          zeroWorkflowConfirm();
        }
      } else if (recalibrationWorkflowIsActive()) {
        if (pressMs >= bootBtnLongPressMs) {
          recalibrationWorkflowCancel();
        } else {
          recalibrationWorkflowConfirm();
        }
      } else {
        if (pressMs >= bootBtnUltraLongPressMs) {
          recalibrationWorkflowStart();
        } else if (pressMs >= bootBtnVeryLongPressMs) {
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
