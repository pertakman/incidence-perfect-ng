#pragma once

#include <WebServer.h>

bool ota_is_upload_in_progress();
bool ota_is_upload_ok();
const char *ota_last_error();

void reset_ota_upload_session_state();
void handle_ota_upload_data(WebServer &server, const char *current_version);
