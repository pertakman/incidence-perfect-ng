// ============================================================
// ui_lvgl.ino — CLEAN REWRITE (FINAL, STABLE)
// LVGL v8.x.y — UI STRUCTURE LOCKED
// ============================================================

#include "ui_lvgl.h"
#include <Arduino_GFX_Library.h>
#include "inclinometer_shared.h"
#include <math.h>
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

// ============================================================
// UI STATE MACHINE
// ============================================================

typedef enum {
  UI_STATE_NORMAL,
  UI_STATE_ALIGN_CAPTURE,
  UI_STATE_ALIGN_DONE
} ui_state_t;

static ui_state_t ui_state = UI_STATE_NORMAL;

// ============================================================
// AXIS DISPLAY MODE
// ============================================================

typedef enum {
  AXIS_BOTH,
  AXIS_ROLL,
  AXIS_PITCH
} axis_display_t;

static axis_display_t axis_mode = AXIS_BOTH;

// ============================================================
// UI OBJECTS
// ============================================================

// Top status
static lv_obj_t *label_mode;
static lv_obj_t *label_header;

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

constexpr int READOUT_Y = 30;   // <<<<<< adjust vertical balance here

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
  switch (axis_mode) {

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
}

static void apply_ui_state()
{
  switch (ui_state) {

    case UI_STATE_NORMAL:
      lv_obj_add_flag(label_header, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(instr_grp,    LV_OBJ_FLAG_HIDDEN);

      lv_label_set_text(label_btn_zero,  "ZERO");
      lv_label_set_text(label_btn_align, "ALIGN");

      lv_obj_clear_state(btn_mode,   LV_STATE_DISABLED);
      lv_obj_clear_state(btn_rotate, LV_STATE_DISABLED);
      lv_obj_clear_state(btn_axis,   LV_STATE_DISABLED);

      apply_axis_layout();
      break;

    case UI_STATE_ALIGN_CAPTURE:
      lv_label_set_text(label_header, "ALIGNMENT");
      lv_obj_clear_flag(label_header, LV_OBJ_FLAG_HIDDEN);

      lv_label_set_text(
        label_instruction,
        "Place tool on reference surface\nand press CAPTURE"
      );
      lv_obj_clear_flag(instr_grp, LV_OBJ_FLAG_HIDDEN);

      lv_obj_add_flag(roll_grp,  LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(pitch_grp, LV_OBJ_FLAG_HIDDEN);

      lv_label_set_text(label_btn_zero,  "CANCEL");
      lv_label_set_text(label_btn_align, "CAPTURE");

      lv_obj_add_state(btn_mode,   LV_STATE_DISABLED);
      lv_obj_add_state(btn_rotate, LV_STATE_DISABLED);
      lv_obj_add_state(btn_axis,   LV_STATE_DISABLED);
      break;

    case UI_STATE_ALIGN_DONE:
      lv_label_set_text(
        label_instruction,
        "Alignment captured\nPress DONE to continue"
      );
      lv_label_set_text(label_btn_align, "DONE");
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

void cycleMode(void)      {}
void toggleRotation(void){}

// ============================================================
// BUTTON CALLBACKS
// ============================================================

static void on_zero_pressed(lv_event_t *)
{
  if (ui_state == UI_STATE_NORMAL) setZeroReference();
  else ui_set_state(UI_STATE_NORMAL);
}

static void on_axis_pressed(lv_event_t *)
{
  if (ui_state != UI_STATE_NORMAL) return;
  axis_mode = (axis_display_t)((axis_mode + 1) % 3);
  apply_axis_layout();
}

static void on_mode_pressed(lv_event_t *)
{
  if (ui_state == UI_STATE_NORMAL) cycleMode();
}

static void on_align_pressed(lv_event_t *)
{
  if (ui_state == UI_STATE_NORMAL) ui_set_state(UI_STATE_ALIGN_CAPTURE);
  else if (ui_state == UI_STATE_ALIGN_CAPTURE) {
    alignmentCapture();
    ui_set_state(UI_STATE_ALIGN_DONE);
  } else ui_set_state(UI_STATE_NORMAL);
}

static void on_rotate_pressed(lv_event_t *)
{
  if (ui_state == UI_STATE_NORMAL) toggleRotation();
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
  lv_label_set_text(label_mode, "SCREEN UP | BOTH"); // placeholder
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

  // ==========================================================
  // INSTRUCTION GROUP (modal, centered)
  // ==========================================================

  instr_grp = lv_obj_create(scr);
  lv_obj_set_width(instr_grp, lv_pct(90));
  lv_obj_align(instr_grp, LV_ALIGN_CENTER, 0, -10);
  lv_obj_set_flex_flow(instr_grp, LV_FLEX_FLOW_COLUMN);

  lv_obj_set_style_bg_opa(instr_grp, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(instr_grp, 0, 0);
  lv_obj_set_style_pad_all(instr_grp, 0, 0);

  lv_obj_clear_flag(instr_grp, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(instr_grp, LV_SCROLLBAR_MODE_OFF);

  label_instruction = lv_label_create(instr_grp);
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

  // Preserve shadow for press animation
  lv_obj_set_style_shadow_width(btn_zero,   8, LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(btn_axis,   8, LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(btn_mode,   8, LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(btn_align,  8, LV_STATE_DEFAULT);
  lv_obj_set_style_shadow_width(btn_rotate, 8, LV_STATE_DEFAULT);

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
}

// ============================================================
// UI UPDATE (TEXT & COLOR ONLY)
// ============================================================

static void update_ui()
{
  static char last_r[16] = "";
  static char last_p[16] = "";

  ui_roll_smooth  = smooth_value(ui_roll_smooth,  ui_roll);
  ui_pitch_smooth = smooth_value(ui_pitch_smooth, ui_pitch);

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
  lv_disp_drv_register(&disp_drv);

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
}

void loop_display()
{
  static uint32_t last_tick = 0;
  static uint32_t last_ui   = 0;

  uint32_t now = millis();

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
