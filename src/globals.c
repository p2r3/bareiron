#include <stdio.h>
#include <stdint.h>
#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
#else
  #include <arpa/inet.h>
#endif
#include <unistd.h>

#include "globals.h"

#ifdef ESP_PLATFORM
  #include "esp_task_wdt.h"
  #include "esp_timer.h"

  // Time between vTaskDelay calls in microseconds
  #define TASK_YIELD_INTERVAL 1000 * 1000
  // How many ticks to delay for on each yield
  #define TASK_YIELD_TICKS 1

  int64_t last_yield = 0;
  void task_yield () {
    int64_t time_now = esp_timer_get_time();
    if (time_now - last_yield < TASK_YIELD_INTERVAL) return;
    vTaskDelay(TASK_YIELD_TICKS);
    last_yield = time_now;
  }
#endif

// Legacy global scalars have moved to ServerContext; removed here.

// All world/player/mob arrays have moved into ServerContext; no globals here.
