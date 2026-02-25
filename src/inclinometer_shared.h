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

// Forward declarations used across files
void setOrientation(OrientationMode m);

void setZeroReference(void);
void cycleMode(void);
void cycleAxisMode(void);
void toggleRotation(void);

void alignmentCapture(void);
