#include "ui_lvgl.h"
#include "inclinometer_shared.h"
#include "remote_control.h"

void setup_inclinometer();
void loop_inclinometer();

void setup()
{
  setup_display();
  setup_inclinometer();
  setup_remote_control();
}

void loop()
{
  loop_inclinometer();   // updates roll & pitch
  loop_remote_control(); // handles phone web UI + API
  loop_display();        // updates LVGL
}
