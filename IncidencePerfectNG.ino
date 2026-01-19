#include "ui_lvgl.h"
#include "inclinometer_shared.h"

void setup()
{
  setup_display();
  setup_inclinometer();
}

void loop()
{
  loop_inclinometer();   // updates roll & pitch
  loop_display();        // updates LVGL
}