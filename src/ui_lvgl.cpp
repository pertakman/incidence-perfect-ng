// ============================================================
// ui_lvgl.ino — CLEAN REWRITE (FINAL, STABLE)
// LVGL v8.x.y — UI STRUCTURE LOCKED
// ============================================================

#include "ui_lvgl.h"
#include <Arduino_GFX_Library.h>
#include "inclinometer_shared.h"
#include "fw_version.h"
#include "splash_image_536x240_rgb565.h"
#include <math.h>
#include <string.h>
#include "touch_bsp.h"

// ============================================================
// DISPLAY CONFIG
// ============================================================

#define LCD_WIDTH   536
#define LCD_HEIGHT  240
#define DEG_SYM "\xC2\xB0"

// ============================================================
// QSPI PINS
// ============================================================

#define LCD_CS   6
#define LCD_RST  17
#define LCD_CLK  47
#define LCD_D0   18
#define LCD_D1   7
#define LCD_D2   48
#define LCD_D3   5

// ============================================================
// DISPLAY DRIVER
// ============================================================

Arduino_DataBus *bus = new Arduino_ESP32QSPI(
  LCD_CS, LCD_CLK, LCD_D0, LCD_D1, LCD_D2, LCD_D3
);

Arduino_GFX *gfx = new Arduino_RM67162(
  bus,
  LCD_RST,
  3,
  false
);

// ============================================================
// TOUCH SCREEN
// ============================================================
extern void touch_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data);

// ============================================================
// LVGL DRAW BUFFER
// ============================================================

static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf1;
static lv_color_t *buf2;
static lv_disp_t *disp_handle = nullptr;

static void show_startup_splash()
{
  gfx->draw16bitRGBBitmap(0, 0, (uint16_t *)SPLASH_IMAGE_536x240_RGB565, LCD_WIDTH, LCD_HEIGHT);

  const int text_size = 2;
  const int char_w = 6 * text_size;
  const int char_h = 8 * text_size;
  const int margin_r = 12;
  const int margin_b = 10;
  const int text_w = (int)strlen(FW_VERSION) * char_w;
  const int text_h = char_h;
  const int text_x = LCD_WIDTH - margin_r - text_w;
  const int text_y = LCD_HEIGHT - margin_b - text_h;

  gfx->fillRect(text_x - 6, text_y - 4, text_w + 12, text_h + 8, 0x0000);
  gfx->setTextSize(text_size);
  gfx->setTextColor(0xFFFF);
  gfx->setCursor(text_x, text_y);
  gfx->print(FW_VERSION);

  delay(1500);
}

// ============================================================
// UI STATE MACHINE
// ============================================================

typedef enum {
  UI_STATE_NORMAL,
  UI_STATE_ALIGN,
  UI_STATE_MODE
} ui_state_t;

static ui_state_t ui_state = UI_STATE_NORMAL;
static void ui_set_state(ui_state_t new_state);

volatile AxisDisplayMode ui_axis_mode = AXIS_BOTH;

// ============================================================
// UI OBJECTS
// ============================================================

// Top status
static lv_obj_t *label_mode;
static lv_obj_t *label_header;
static lv_obj_t *boot_hint_box;
static lv_obj_t *label_boot_hint;
static lv_obj_t *boot_hint_progress;

// Instruction group
static lv_obj_t *instr_grp;
static lv_obj_t *label_instruction;

// Roll / Pitch
static lv_obj_t *roll_grp;
static lv_obj_t *pitch_grp;
static lv_obj_t *label_roll;
static lv_obj_t *label_pitch;
static lv_obj_t *label_roll_value;
static lv_obj_t *label_pitch_value;

constexpr int READOUT_Y = 18;   // moved up to free room for hints above controls

// Buttons
static lv_obj_t *btn_zero;
static lv_obj_t *btn_axis;
static lv_obj_t *btn_mode;
static lv_obj_t *btn_align;
static lv_obj_t *btn_rotate;

static lv_obj_t *label_btn_zero;
static lv_obj_t *label_btn_axis;
static lv_obj_t *label_btn_mode;
static lv_obj_t *label_btn_align;
static lv_obj_t *label_btn_rotate;
static uint32_t zero_feedback_until_ms = 0;
static uint32_t zero_feedback_start_ms = 0;

static const char *axis_mode_text()
{
  switch (ui_axis_mode) {
    case AXIS_ROLL:  return "ROLL";
    case AXIS_PITCH: return "PITCH";
    default:         return "BOTH";
  }
}

static const char *orientation_text_for(OrientationMode m)
{
  return (m == MODE_SCREEN_VERTICAL) ? "SCREEN VERTICAL" : "SCREEN UP";
}

static const char *orientation_instruction_text_for(OrientationMode m)
{
  return (m == MODE_SCREEN_VERTICAL) ? "screen vertically" : "screen facing up";
}

static void process_mode_pending()
{
  // MODE workflow is owned by inclinometer.cpp and mirrored here.
}

static void update_status_label()
{
  static char last[64] = "";
  char buf[64];
  const char *orientation_text =
    (orientationMode == MODE_SCREEN_VERTICAL) ? "SCREEN VERTICAL" : "SCREEN UP";
  snprintf(buf, sizeof(buf), "%s | %s | ROT %d | %s",
           orientation_text,
           axis_mode_text(),
           displayRotated ? 180 : 0,
           measurementIsFrozen() ? "FROZEN" : "LIVE");

  if (strcmp(buf, last) != 0) {
    lv_label_set_text(label_mode, buf);
    strncpy(last, buf, sizeof(last) - 1);
    last[sizeof(last) - 1] = '\0';
  }
}

static void update_alignment_instruction()
{
  char buf[128];
  alignmentGetInstruction(buf, sizeof(buf));
  lv_label_set_text(label_instruction, buf);
}

static float countdown_display_seconds(float seconds_remaining)
{
  if (seconds_remaining <= 0.0f) return 0.0f;
  // Bias slightly upward so display cadence/loop jitter never shows 0.0s
  // before the underlying action has actually transitioned.
  seconds_remaining += 0.02f;
  float rounded_up = ceilf(seconds_remaining * 10.0f) / 10.0f;
  if (rounded_up < 0.1f) rounded_up = 0.1f;
  return rounded_up;
}

static void update_boot_hint_label()
{
  static char last[96] = "";
  char buf[96] = "";
  const uint32_t now_ms = millis();
  int progress_pct = -1;

  if (ui_state == UI_STATE_ALIGN) {
    lv_obj_add_flag(boot_hint_box, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(boot_hint_progress, LV_OBJ_FLAG_HIDDEN);
    last[0] = '\0';
    return;
  }

  if (ui_state == UI_STATE_MODE) {
    const char *target_text = orientation_instruction_text_for(modeWorkflowTarget());
    if (!modeWorkflowIsConfirmed()) {
      if (bootHoldIsActive()) {
        const unsigned long hold_ms = bootHoldDurationMs();
        if (hold_ms < 1200) {
          const float to_cancel = countdown_display_seconds((1200 - hold_ms) / 1000.0f);
          snprintf(buf, sizeof(buf),
                   "ACTION: release = CONFIRM | CANCEL in %.1f s",
                   to_cancel);
          progress_pct = (int)((hold_ms * 100UL) / 1200UL);
        } else {
          snprintf(buf, sizeof(buf), "ACTION: release = CANCEL");
          progress_pct = 100;
        }
      } else {
        snprintf(buf, sizeof(buf), "Reposition with %s", target_text);
      }
    } else {
      const float rem = modeWorkflowRemainingSeconds();
      // Flip to "Applying..." a tick before zero to avoid a stale final
      // countdown frame when the action completion is in-flight.
      if (rem > 0.10f) {
        const float rem_show = countdown_display_seconds(rem);
        snprintf(buf, sizeof(buf), "Hold still %.1f s", rem_show);
        progress_pct = (int)modeWorkflowProgressPercent();
      } else {
        snprintf(buf, sizeof(buf), "Applying...");
        progress_pct = 100;
      }
    }
  } else if (now_ms < zero_feedback_until_ms) {
    snprintf(buf, sizeof(buf), "Zero applied - hold still briefly");
    const uint32_t duration_ms = zero_feedback_until_ms - zero_feedback_start_ms;
    const uint32_t elapsed_ms = now_ms - zero_feedback_start_ms;
    if (duration_ms > 0) {
      progress_pct = (int)((elapsed_ms * 100UL) / duration_ms);
    } else {
      progress_pct = 100;
    }
  } else if (!bootHoldIsActive()) {
    lv_obj_add_flag(boot_hint_box, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(boot_hint_progress, LV_OBJ_FLAG_HIDDEN);
    last[0] = '\0';
    return;
  }

  if (ui_state == UI_STATE_NORMAL && now_ms >= zero_feedback_until_ms) {
    const unsigned long hold_ms = bootHoldDurationMs();
    const float to_axis = (hold_ms < 1200) ? countdown_display_seconds((1200 - hold_ms) / 1000.0f) : 0.0f;
    const float to_mode = (hold_ms < 2200) ? countdown_display_seconds((2200 - hold_ms) / 1000.0f) : 0.0f;

    if (hold_ms < 1200) {
      snprintf(buf, sizeof(buf),
               "Release: FREEZE | AXIS in %.1f s",
               to_axis);
      progress_pct = (int)((hold_ms * 100UL) / 2200UL);
    } else if (hold_ms < 2200) {
      snprintf(buf, sizeof(buf),
               "Release: AXIS | MODE in %.1f s",
               to_mode);
      progress_pct = (int)((hold_ms * 100UL) / 2200UL);
    } else {
      snprintf(buf, sizeof(buf), "Release: MODE");
      progress_pct = 100;
    }
  }

  if (strcmp(last, buf) != 0) {
    lv_label_set_text(label_boot_hint, buf);
    strncpy(last, buf, sizeof(last) - 1);
    last[sizeof(last) - 1] = '\0';
  }

  if (progress_pct >= 0) {
    if (progress_pct > 100) progress_pct = 100;
    lv_bar_set_value(boot_hint_progress, progress_pct, LV_ANIM_OFF);
    lv_obj_clear_flag(boot_hint_progress, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(boot_hint_progress, LV_OBJ_FLAG_HIDDEN);
  }
  lv_obj_clear_flag(boot_hint_box, LV_OBJ_FLAG_HIDDEN);
}

// ============================================================
// FILTERING / COLORS
// ============================================================

static float ui_roll_smooth  = 0.0f;
static float ui_pitch_smooth = 0.0f;

constexpr float EMA_ALPHA = 0.15f;
constexpr float DEAD_BAND = 0.05f;

constexpr float WARN_LIMIT = 30.0f;
constexpr float CRIT_LIMIT = 45.0f;

static float smooth_value(float prev, float raw)
{
  float delta = raw - prev;
  if (fabsf(delta) < DEAD_BAND) return prev;
  return prev + EMA_ALPHA * delta;
}

static lv_color_t angle_color(float v)
{
  float a = fabsf(v);
  if (a >= CRIT_LIMIT) return lv_color_hex(0xDC143C);
  if (a >= WARN_LIMIT) return lv_color_hex(0xFFBF00);
  return lv_color_white();
}

// ============================================================
// LVGL FLUSH CALLBACK
// ============================================================

void my_disp_flush(lv_disp_drv_t *disp_drv,
                   const lv_area_t *area,
                   lv_color_t *color_p)
{
  gfx->draw16bitRGBBitmap(
    area->x1,
    area->y1,
    (uint16_t *)color_p,
    area->x2 - area->x1 + 1,
    area->y2 - area->y1 + 1
  );
  lv_disp_flush_ready(disp_drv);
}

// ============================================================
// UI POLICY LAYER
// ============================================================

static void apply_axis_layout()
{
  switch (ui_axis_mode) {

    case AXIS_BOTH:
      lv_obj_clear_flag(roll_grp,  LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(pitch_grp, LV_OBJ_FLAG_HIDDEN);
      lv_obj_align(roll_grp,  LV_ALIGN_CENTER, -120, READOUT_Y);
      lv_obj_align(pitch_grp, LV_ALIGN_CENTER,  120, READOUT_Y);
      break;

    case AXIS_ROLL:
      lv_obj_clear_flag(roll_grp, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag  (pitch_grp, LV_OBJ_FLAG_HIDDEN);
      lv_obj_align(roll_grp, LV_ALIGN_CENTER, 0, READOUT_Y);
      break;

    case AXIS_PITCH:
      lv_obj_add_flag  (roll_grp, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(pitch_grp, LV_OBJ_FLAG_HIDDEN);
      lv_obj_align(pitch_grp, LV_ALIGN_CENTER, 0, READOUT_Y);
      break;
  }

  update_status_label();
}

static void apply_ui_state()
{
  switch (ui_state) {

    case UI_STATE_NORMAL:
      lv_obj_clear_flag(label_mode, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(boot_hint_box, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(label_header, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(instr_grp,    LV_OBJ_FLAG_HIDDEN);

      lv_label_set_text(label_btn_zero,  "ZERO");
      lv_label_set_text(label_btn_mode,  "MODE");
      lv_label_set_text(label_btn_align, "ALIGN");

      lv_obj_clear_state(btn_mode,   LV_STATE_DISABLED);
      lv_obj_clear_state(btn_rotate, LV_STATE_DISABLED);
      lv_obj_clear_state(btn_axis,   LV_STATE_DISABLED);
      lv_obj_clear_state(btn_align,  LV_STATE_DISABLED);

      apply_axis_layout();
      break;

    case UI_STATE_ALIGN:
      modeWorkflowCancel();
      lv_obj_add_flag(label_mode, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(boot_hint_box, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(label_header, LV_OBJ_FLAG_HIDDEN);

      update_alignment_instruction();
      lv_obj_clear_flag(instr_grp, LV_OBJ_FLAG_HIDDEN);

      lv_obj_add_flag(roll_grp,  LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(pitch_grp, LV_OBJ_FLAG_HIDDEN);

      lv_label_set_text(label_btn_zero,  "CANCEL");
      lv_label_set_text(label_btn_align, "CAPTURE");

      lv_obj_add_state(btn_mode,   LV_STATE_DISABLED);
      lv_obj_add_state(btn_rotate, LV_STATE_DISABLED);
      lv_obj_add_state(btn_axis,   LV_STATE_DISABLED);
      lv_obj_clear_state(btn_align, LV_STATE_DISABLED);
      break;

    case UI_STATE_MODE:
      lv_obj_clear_flag(label_mode, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(label_header, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(instr_grp, LV_OBJ_FLAG_HIDDEN);

      lv_label_set_text(label_btn_zero, "CANCEL");
      lv_label_set_text(label_btn_mode, "CONFIRM");
      lv_label_set_text(label_btn_align, "ALIGN");

      lv_obj_add_state(btn_axis,   LV_STATE_DISABLED);
      lv_obj_add_state(btn_align,  LV_STATE_DISABLED);
      lv_obj_add_state(btn_rotate, LV_STATE_DISABLED);
      lv_obj_clear_state(btn_mode, LV_STATE_DISABLED);

      apply_axis_layout();
      break;
  }
}

// ============================================================
// UI STATE API
// ============================================================

static void ui_set_state(ui_state_t new_state)
{
  ui_state = new_state;
  apply_ui_state();
}

// ============================================================
// APPLICATION HOOKS
// ============================================================

void cycleMode(void)
{
  setOrientation(
    orientationMode == MODE_SCREEN_VERTICAL ? MODE_SCREEN_UP : MODE_SCREEN_VERTICAL
  );
}

void cycleAxisMode(void)
{
  ui_axis_mode = (AxisDisplayMode)((ui_axis_mode + 1) % 3);
  if (ui_state == UI_STATE_NORMAL) {
    apply_axis_layout();
  }
}

// ============================================================
// BUTTON CALLBACKS
// ============================================================

static void on_zero_pressed(lv_event_t *)
{
  if (ui_state == UI_STATE_MODE) {
    modeWorkflowCancel();
    ui_set_state(UI_STATE_NORMAL);
    return;
  }

  if (ui_state == UI_STATE_NORMAL) {
    setZeroReference();
    zero_feedback_start_ms = millis();
    zero_feedback_until_ms = millis() + 1400;
  }
  else {
    alignmentCancel();
    ui_set_state(UI_STATE_NORMAL);
  }
}

static void on_axis_pressed(lv_event_t *)
{
  if (ui_state != UI_STATE_NORMAL) return;
  modeWorkflowCancel();
  cycleAxisMode();
}

static void on_mode_pressed(lv_event_t *)
{
  if (ui_state == UI_STATE_NORMAL) {
    modeWorkflowStartToggle();
    ui_set_state(UI_STATE_MODE);
    return;
  }

  if (ui_state != UI_STATE_MODE) return;

  if (!modeWorkflowIsConfirmed()) {
    modeWorkflowConfirm();
    return;
  }
}

static void on_readout_pressed(lv_event_t *)
{
  if (ui_state != UI_STATE_NORMAL) return;
  modeWorkflowCancel();
  toggleMeasurementFreeze();
}

static void on_align_pressed(lv_event_t *)
{
  if (ui_state == UI_STATE_NORMAL) {
    modeWorkflowCancel();
    alignmentStart();
    if (alignmentIsActive()) ui_set_state(UI_STATE_ALIGN);
  } else if (ui_state == UI_STATE_ALIGN) {
    alignmentCapture();
    if (alignmentIsActive()) {
      update_alignment_instruction();
    } else {
      ui_set_state(UI_STATE_NORMAL);
    }
  }
}

static void on_rotate_pressed(lv_event_t *)
{
  if (ui_state == UI_STATE_NORMAL) {
    modeWorkflowCancel();
    toggleRotation();
  }
}

// ============================================================
// UI CREATION
// ============================================================

static void create_ui()
{
  lv_obj_t *scr = lv_scr_act();

  // ----------------------------------------------------------
  // Screen hard reset (kills borders, scrollbars, ghost lines)
  // ----------------------------------------------------------
  lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
  lv_obj_set_style_border_width(scr, 0, LV_PART_ANY | LV_STATE_ANY);
  lv_obj_set_style_outline_width(scr, 0, LV_PART_ANY | LV_STATE_ANY);
  lv_obj_set_style_shadow_width(scr, 0, LV_PART_ANY | LV_STATE_ANY);
  lv_obj_set_style_pad_all(scr, 0, LV_PART_ANY | LV_STATE_ANY);

  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);

  // ==========================================================
  // TOP STATUS AREA
  // ==========================================================

  // Persistent context (Orientation | Axis)
  label_mode = lv_label_create(scr);
  lv_label_set_text(label_mode, "");
  lv_obj_set_style_text_font(label_mode, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_color(label_mode, lv_color_hex(0xC0C0C0), 0);
  lv_obj_align(label_mode, LV_ALIGN_TOP_LEFT, 2, 2); // Changed to 2 from 6

  // Modal header (hidden normally)
  label_header = lv_label_create(scr);
  lv_label_set_text(label_header, "");
  lv_obj_set_style_text_font(label_header, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_color(label_header, lv_color_white(), 0);
  lv_obj_align(label_header, LV_ALIGN_TOP_MID, 0, 6);
  lv_obj_add_flag(label_header, LV_OBJ_FLAG_HIDDEN);

  boot_hint_box = lv_obj_create(scr);
  lv_obj_set_size(boot_hint_box, lv_pct(96), 34);
  lv_obj_align(boot_hint_box, LV_ALIGN_BOTTOM_MID, 0, -68);
  lv_obj_set_style_bg_color(boot_hint_box, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(boot_hint_box, LV_OPA_80, 0);
  lv_obj_set_style_border_width(boot_hint_box, 0, 0);
  lv_obj_set_style_radius(boot_hint_box, 8, 0);
  lv_obj_set_style_pad_all(boot_hint_box, 2, 0);
  lv_obj_clear_flag(boot_hint_box, LV_OBJ_FLAG_SCROLLABLE);

  label_boot_hint = lv_label_create(boot_hint_box);
  lv_label_set_text(label_boot_hint, "");
  lv_obj_set_style_text_font(label_boot_hint, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(label_boot_hint, lv_color_hex(0xE6F5FF), 0);
  lv_obj_set_style_text_align(label_boot_hint, LV_TEXT_ALIGN_CENTER, 0);
  lv_label_set_long_mode(label_boot_hint, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(label_boot_hint, lv_pct(98));
  lv_obj_align(label_boot_hint, LV_ALIGN_TOP_MID, 0, 0);

  boot_hint_progress = lv_bar_create(boot_hint_box);
  lv_obj_set_size(boot_hint_progress, lv_pct(72), 5);
  lv_obj_align(boot_hint_progress, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_bar_set_range(boot_hint_progress, 0, 100);
  lv_bar_set_value(boot_hint_progress, 0, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(boot_hint_progress, lv_color_hex(0x3A3A3A), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(boot_hint_progress, LV_OPA_70, LV_PART_MAIN);
  lv_obj_set_style_bg_color(boot_hint_progress, lv_color_hex(0x6FD3FF), LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(boot_hint_progress, LV_OPA_90, LV_PART_INDICATOR);
  lv_obj_set_style_radius(boot_hint_progress, 3, LV_PART_MAIN);
  lv_obj_set_style_radius(boot_hint_progress, 3, LV_PART_INDICATOR);
  lv_obj_add_flag(boot_hint_progress, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(boot_hint_box, LV_OBJ_FLAG_HIDDEN);

  // ==========================================================
  // INSTRUCTION GROUP (modal, centered)
  // ==========================================================

  instr_grp = lv_obj_create(scr);
  lv_obj_set_width(instr_grp, lv_pct(94));
  lv_obj_align(instr_grp, LV_ALIGN_CENTER, 0, -34);
  lv_obj_set_flex_flow(instr_grp, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(instr_grp,
                        LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);

  lv_obj_set_style_bg_color(instr_grp, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(instr_grp, LV_OPA_80, 0);
  lv_obj_set_style_border_width(instr_grp, 0, 0);
  lv_obj_set_style_pad_all(instr_grp, 8, 0);

  lv_obj_clear_flag(instr_grp, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(instr_grp, LV_SCROLLBAR_MODE_OFF);

  label_instruction = lv_label_create(instr_grp);
  lv_obj_set_width(label_instruction, lv_pct(100));
  lv_label_set_text(label_instruction, "");
  lv_label_set_long_mode(label_instruction, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_font(label_instruction, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_color(label_instruction, lv_color_hex(0xE0E0E0), 0);
  lv_obj_set_style_text_align(label_instruction, LV_TEXT_ALIGN_CENTER, 0);

  lv_obj_add_flag(instr_grp, LV_OBJ_FLAG_HIDDEN);

  // ==========================================================
  // MAIN INSTRUMENT READOUT AREA
  // ==========================================================

  // --- Roll ---
  roll_grp = lv_obj_create(scr);
  lv_obj_add_flag(roll_grp, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_bg_opa(roll_grp, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(roll_grp, 0, 0);
  lv_obj_set_flex_flow(roll_grp, LV_FLEX_FLOW_COLUMN);
  lv_obj_align(roll_grp, LV_ALIGN_CENTER, -120, READOUT_Y);   // vertical spacing (Changed to 60 from 36)

  label_roll = lv_label_create(roll_grp);
  lv_label_set_text(label_roll, "ROLL");
  lv_obj_set_style_text_font(label_roll, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(label_roll, lv_color_hex(0xD0D0D0), 0);

  label_roll_value = lv_label_create(roll_grp);
  lv_obj_set_style_text_font(label_roll_value, &lv_font_montserrat_48, 0);

  // --- Pitch ---
  pitch_grp = lv_obj_create(scr);
  lv_obj_add_flag(pitch_grp, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_bg_opa(pitch_grp, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(pitch_grp, 0, 0);
  lv_obj_set_flex_flow(pitch_grp, LV_FLEX_FLOW_COLUMN);
  lv_obj_align(pitch_grp, LV_ALIGN_CENTER, 120, READOUT_Y);   // vertical spacing (Changed to 60 from 36)

  label_pitch = lv_label_create(pitch_grp);
  lv_label_set_text(label_pitch, "PITCH");
  lv_obj_set_style_text_font(label_pitch, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(label_pitch, lv_color_hex(0xD0D0D0), 0);

  label_pitch_value = lv_label_create(pitch_grp);
  lv_obj_set_style_text_font(label_pitch_value, &lv_font_montserrat_48, 0);

  // ==========================================================
  // BOTTOM CONTROL AREA (fixed, no reflow)
  // ==========================================================

  constexpr int DIVIDER_H = 2;
  constexpr int BTN_BAR_H = 70;

  lv_obj_t *bottom = lv_obj_create(scr);
  lv_obj_set_width(bottom, lv_pct(100));
  lv_obj_set_height(bottom, DIVIDER_H + BTN_BAR_H);
  lv_obj_align(bottom, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_flex_flow(bottom, LV_FLEX_FLOW_COLUMN);

  lv_obj_set_style_bg_opa(bottom, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(bottom, 0, 0);
  lv_obj_set_style_shadow_width(bottom, 0, 0);
  lv_obj_set_style_pad_all(bottom, 0, 0);
  lv_obj_set_style_pad_row(bottom, 6, 0);

  lv_obj_clear_flag(bottom, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(bottom, LV_SCROLLBAR_MODE_OFF);

  // --- Divider ---
  lv_obj_t *divider = lv_obj_create(bottom);
  lv_obj_set_height(divider, DIVIDER_H);
  lv_obj_set_width(divider, lv_pct(100));
  lv_obj_set_style_bg_color(divider, lv_color_hex(0x303030), 0);
  lv_obj_set_style_border_width(divider, 0, 0);
  lv_obj_set_style_outline_width(divider, 0, 0);
  lv_obj_set_style_shadow_width(divider, 0, 0);
  lv_obj_set_style_pad_all(divider, 0, 0);

  // --- Button bar ---
  lv_obj_t *bar = lv_obj_create(bottom);
  lv_obj_set_height(bar, BTN_BAR_H);
  lv_obj_set_width(bar, lv_pct(100));

  // 🔒 CRITICAL: no layout, no repaint flicker
  lv_obj_add_flag(bar, LV_OBJ_FLAG_IGNORE_LAYOUT);

  // Invisible but non-redrawing background
  lv_obj_set_style_bg_opa(bar, LV_OPA_0, 0);
  lv_obj_set_style_bg_color(bar, lv_color_black(), 0);

  lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(bar, LV_SCROLLBAR_MODE_OFF);

  lv_obj_set_style_border_width(bar, 0, 0);
  lv_obj_set_style_outline_width(bar, 0, 0);
  lv_obj_set_style_shadow_width(bar, 0, 0);
  lv_obj_set_style_pad_all(bar, 0, 0);
  lv_obj_set_style_pad_column(bar, 6, 0);
  lv_obj_set_style_pad_top(bar, 4, 0);
  lv_obj_set_style_pad_bottom(bar, 4, 0);


  lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(bar,
                        LV_FLEX_ALIGN_SPACE_EVENLY,
                        LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);

  // --- Buttons ---
  constexpr int BTN_W = 96;
  constexpr int BTN_H = 50;

  btn_zero   = lv_btn_create(bar);
  btn_axis   = lv_btn_create(bar);
  btn_mode   = lv_btn_create(bar);
  btn_align  = lv_btn_create(bar);
  btn_rotate = lv_btn_create(bar);

  lv_obj_set_size(btn_zero,   BTN_W, BTN_H);
  lv_obj_set_size(btn_axis,   BTN_W, BTN_H);
  lv_obj_set_size(btn_mode,   BTN_W, BTN_H);
  lv_obj_set_size(btn_align,  BTN_W, BTN_H);
  lv_obj_set_size(btn_rotate, BTN_W, BTN_H);

  // Make touch targets more forgiving for larger fingers.
  lv_obj_set_ext_click_area(btn_zero,   10);
  lv_obj_set_ext_click_area(btn_axis,   10);
  lv_obj_set_ext_click_area(btn_mode,   10);
  lv_obj_set_ext_click_area(btn_align,  10);
  lv_obj_set_ext_click_area(btn_rotate, 10);

  // Preserve shadow for press animation
  lv_obj_set_style_shadow_width(btn_zero,   8, LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(btn_axis,   8, LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(btn_mode,   8, LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(btn_align,  8, LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(btn_rotate, 8, LV_STATE_DEFAULT);

  // Snappier button interaction: faster transitions + tighter pressed feedback.
  lv_obj_set_style_anim_time(btn_zero,   70, LV_PART_MAIN);
  lv_obj_set_style_anim_time(btn_axis,   70, LV_PART_MAIN);
  lv_obj_set_style_anim_time(btn_mode,   70, LV_PART_MAIN);
  lv_obj_set_style_anim_time(btn_align,  70, LV_PART_MAIN);
  lv_obj_set_style_anim_time(btn_rotate, 70, LV_PART_MAIN);

  lv_obj_set_style_transform_width(btn_zero,   -2, LV_STATE_PRESSED);
  lv_obj_set_style_transform_width(btn_axis,   -2, LV_STATE_PRESSED);
  lv_obj_set_style_transform_width(btn_mode,   -2, LV_STATE_PRESSED);
  lv_obj_set_style_transform_width(btn_align,  -2, LV_STATE_PRESSED);
  lv_obj_set_style_transform_width(btn_rotate, -2, LV_STATE_PRESSED);

  lv_obj_set_style_transform_height(btn_zero,   -2, LV_STATE_PRESSED);
  lv_obj_set_style_transform_height(btn_axis,   -2, LV_STATE_PRESSED);
  lv_obj_set_style_transform_height(btn_mode,   -2, LV_STATE_PRESSED);
  lv_obj_set_style_transform_height(btn_align,  -2, LV_STATE_PRESSED);
  lv_obj_set_style_transform_height(btn_rotate, -2, LV_STATE_PRESSED);

  lv_obj_set_style_shadow_width(btn_zero,   3, LV_STATE_PRESSED);
  lv_obj_set_style_shadow_width(btn_axis,   3, LV_STATE_PRESSED);
  lv_obj_set_style_shadow_width(btn_mode,   3, LV_STATE_PRESSED);
  lv_obj_set_style_shadow_width(btn_align,  3, LV_STATE_PRESSED);
  lv_obj_set_style_shadow_width(btn_rotate, 3, LV_STATE_PRESSED);

  label_btn_zero   = lv_label_create(btn_zero);
  label_btn_axis   = lv_label_create(btn_axis);
  label_btn_mode   = lv_label_create(btn_mode);
  label_btn_align  = lv_label_create(btn_align);
  label_btn_rotate = lv_label_create(btn_rotate);

  lv_label_set_text(label_btn_zero,   "ZERO");
  lv_label_set_text(label_btn_axis,   "AXIS");
  lv_label_set_text(label_btn_mode,   "MODE");
  lv_label_set_text(label_btn_align,  "ALIGN");
  lv_label_set_text(label_btn_rotate, "ROTATE");

  lv_obj_center(label_btn_zero);
  lv_obj_center(label_btn_axis);
  lv_obj_center(label_btn_mode);
  lv_obj_center(label_btn_align);
  lv_obj_center(label_btn_rotate);

  lv_obj_add_event_cb(btn_zero,   on_zero_pressed,   LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(btn_axis,   on_axis_pressed,   LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(btn_mode,   on_mode_pressed,   LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(btn_align,  on_align_pressed,  LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(btn_rotate, on_rotate_pressed, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(roll_grp,   on_readout_pressed, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(pitch_grp,  on_readout_pressed, LV_EVENT_CLICKED, NULL);
}

// ============================================================
// UI UPDATE (TEXT & COLOR ONLY)
// ============================================================

static void update_ui()
{
  static char last_r[16] = "";
  static char last_p[16] = "";
  static bool last_align_active = false;
  static bool last_frozen = false;
  static float frozen_display_roll = 0.0f;
  static float frozen_display_pitch = 0.0f;

  bool align_active = alignmentIsActive();
  if (align_active != last_align_active) {
    ui_set_state(align_active ? UI_STATE_ALIGN : UI_STATE_NORMAL);
    last_align_active = align_active;
  }

  if (!align_active) {
    const bool mode_active = modeWorkflowIsActive();
    if (mode_active && ui_state != UI_STATE_MODE) {
      ui_set_state(UI_STATE_MODE);
    } else if (!mode_active && ui_state == UI_STATE_MODE) {
      ui_set_state(UI_STATE_NORMAL);
    }
  }

  const bool frozen = measurementIsFrozen();
  if (frozen && !last_frozen) {
    // Freeze what the user currently sees, not the next filtered sample.
    frozen_display_roll = ui_roll_smooth;
    frozen_display_pitch = ui_pitch_smooth;
  }

  if (frozen) {
    ui_roll_smooth = frozen_display_roll;
    ui_pitch_smooth = frozen_display_pitch;
  } else {
    ui_roll_smooth  = smooth_value(ui_roll_smooth,  ui_roll);
    ui_pitch_smooth = smooth_value(ui_pitch_smooth, ui_pitch);
  }
  last_frozen = frozen;

  process_mode_pending();
  update_status_label();

  if (ui_state == UI_STATE_ALIGN) {
    update_alignment_instruction();
  }
  update_boot_hint_label();

  char buf[16];

  snprintf(buf, sizeof(buf), "%+.2f%s", ui_roll_smooth, DEG_SYM);
  if (strcmp(buf, last_r)) {
    lv_label_set_text(label_roll_value, buf);
    lv_obj_set_style_text_color(label_roll_value,
      angle_color(ui_roll_smooth), 0);
    strcpy(last_r, buf);
  }

  snprintf(buf, sizeof(buf), "%+.2f%s", ui_pitch_smooth, DEG_SYM);
  if (strcmp(buf, last_p)) {
    lv_label_set_text(label_pitch_value, buf);
    lv_obj_set_style_text_color(label_pitch_value,
      angle_color(ui_pitch_smooth), 0);
    strcpy(last_p, buf);
  }
}

// ============================================================
// DISPLAY SETUP / LOOP
// ============================================================

void setup_display()
{
  gfx->begin();
  show_startup_splash();
  lv_init();

  Touch_Init();

  buf1 = (lv_color_t *)heap_caps_malloc(
    LCD_WIDTH * 60 * sizeof(lv_color_t),
    MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

  buf2 = (lv_color_t *)heap_caps_malloc(
    LCD_WIDTH * 60 * sizeof(lv_color_t),
    MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

  lv_disp_draw_buf_init(&draw_buf, buf1, buf2, LCD_WIDTH * 60);

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = LCD_WIDTH;
  disp_drv.ver_res = LCD_HEIGHT;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  disp_drv.sw_rotate = 1;
  disp_handle = lv_disp_drv_register(&disp_drv);
  lv_disp_set_rotation(
    disp_handle,
    displayRotated ? LV_DISP_ROT_180 : LV_DISP_ROT_NONE
  );

  // ==========================================================
  // TOUCH INPUT DEVICE (LVGL)
  // ==========================================================
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = touch_read_cb;
  lv_indev_drv_register(&indev_drv);

  create_ui();
  apply_ui_state();
  update_status_label();
}

void loop_display()
{
  static uint32_t last_tick = 0;
  static uint32_t last_ui   = 0;
  static int last_rotation = -1;

  uint32_t now = millis();
  int desired_rotation = displayRotated ? 1 : 0;

  if (last_rotation != desired_rotation) {
    if (disp_handle) {
      lv_disp_set_rotation(
        disp_handle,
        displayRotated ? LV_DISP_ROT_180 : LV_DISP_ROT_NONE
      );
    }
    last_rotation = desired_rotation;
  }

  if (now - last_tick >= 5) {
    lv_tick_inc(5);
    last_tick += 5;
  }

  lv_timer_handler();

  if (now - last_ui >= 50) {
    update_ui();
    last_ui = now;
  }

  delay(5);
}
