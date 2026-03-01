#pragma once

#include <QMI8658.h>

// Shared UI values
extern volatile float ui_roll;
extern volatile float ui_pitch;

// Orientation enum shared across files
enum OrientationMode {
  MODE_SCREEN_UP = 0,
  MODE_SCREEN_VERTICAL = 1
};

enum AxisDisplayMode {
  AXIS_BOTH = 0,
  AXIS_ROLL = 1,
  AXIS_PITCH = 2
};

extern OrientationMode orientationMode;
extern bool displayRotated;
extern volatile AxisDisplayMode ui_axis_mode;
AxisDisplayMode getAxisMode(void);

struct ImuDiagnosticsSample {
  bool valid;
  float sens_ax;
  float sens_ay;
  float sens_az;
  float sens_gx;
  float sens_gy;
  float sens_gz;
  float map_ax;
  float map_ay;
  float map_az;
  float map_gx;
  float map_gy;
  float corr_ax;
  float corr_ay;
  float corr_az;
  float corr_gx;
  float corr_gy;
  float angle_roll;
  float angle_pitch;
};

struct CalibrationStateSnapshot {
  float bias_ax;
  float bias_ay;
  float bias_az;
  float bias_gx;
  float bias_gy;
  float zero_roll;
  float zero_pitch;
  float align_roll;
  float align_pitch;
};

// Forward declarations used across files
void setOrientation(OrientationMode m);

void setZeroReference(void);
void zeroWorkflowStart(void);
void zeroWorkflowConfirm(void);
void zeroWorkflowCancel(void);
bool zeroWorkflowIsActive(void);
bool zeroWorkflowIsConfirmed(void);
float zeroWorkflowRemainingSeconds(void);
float zeroWorkflowProgressPercent(void);
void cycleMode(void);
void cycleAxisMode(void);
void toggleRotation(void);
void toggleMeasurementFreeze(void);
bool measurementIsFrozen(void);
float rollConditionPercent(void);
bool rollConditionIsLow(void);
bool bootHoldIsActive(void);
unsigned long bootHoldDurationMs(void);
bool modeWorkflowIsActive(void);
bool modeWorkflowIsConfirmed(void);
OrientationMode modeWorkflowTarget(void);
float modeWorkflowRemainingSeconds(void);
float modeWorkflowProgressPercent(void);
void modeWorkflowStart(OrientationMode target);
void modeWorkflowStartToggle(void);
void modeWorkflowConfirm(void);
void modeWorkflowCancel(void);

bool alignmentIsActive(void);
void alignmentStart(void);
void alignmentCancel(void);
void alignmentCapture(void);
void alignmentGetInstruction(char *buf, unsigned int buf_size);
bool alignmentCaptureInProgress(void);
float alignmentCaptureProgressPercent(void);
void runQuickOffsetCalibration(void);

void offsetCalibrationWorkflowStart(void);
void offsetCalibrationWorkflowConfirm(void);
void offsetCalibrationWorkflowCancel(void);
bool offsetCalibrationWorkflowIsActive(void);
bool offsetCalibrationWorkflowIsConfirmed(void);
float offsetCalibrationWorkflowRemainingSeconds(void);
float offsetCalibrationWorkflowProgressPercent(void);
void getImuDiagnosticsSample(ImuDiagnosticsSample *out_sample);
void getCalibrationStateSnapshot(CalibrationStateSnapshot *out_snapshot);
