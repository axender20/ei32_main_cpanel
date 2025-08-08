#include "Arduino.h"
#include "main_config.h"
#include "handler_wifi.h"

void setup() {
  delay(1000);
  load_config();
  init_thr_wifi();

  xTaskCreate(thread_wifi, "tsk_w", 8192, NULL, 1, NULL);
}

void loop() {
}

