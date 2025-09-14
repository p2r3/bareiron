#ifndef H_TOOLS
#define H_TOOLS

#include <unistd.h>

#include "globals.h"

/* how do we / can we do inline */
#if defined(_MSC_VER) && _MSC_VER < 1200
/* available as a MS extension '__inline' */
#  define inline __inline
#elif defined(__GNUC__)
#  if defined(__STDC_VERSION__) && __STDC_VERSION__ > 199901L
/* available as 'inline' */
#  elif !defined(__STRICT_ANSI__)
/* available as GNU extension 'inline' */
#  else
/*#    warning "No known way to do inline functions in this compiler - doing nothing"*/
#    define inline /* nothing */
#  endif
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ < 199901L
/*#  warning "No known way to do inline functions in this compiler - doing nothing"*/
#  define inline /* nothing */
#endif


static inline int mod_abs (int a, int b) {
  return ((a % b) + b) % b;
}
static inline int div_floor (int a, int b) {
  return a % b < 0 ? (a - b) / b : a / b;
}

extern uint64_t total_bytes_received;
ssize_t recv_all (int client_fd, void *buf, size_t n, uint8_t require_first);
ssize_t send_all (int client_fd, const void *buf, ssize_t len);

ssize_t writeByte (int client_fd, uint8_t byte);
ssize_t writeUint16 (int client_fd, uint16_t num);
ssize_t writeUint32 (int client_fd, uint32_t num);
ssize_t writeUint64 (int client_fd, uint64_t num);
ssize_t writeFloat (int client_fd, float num);
ssize_t writeDouble (int client_fd, double num);

uint8_t readByte (int client_fd);
uint16_t readUint16 (int client_fd);
int16_t readInt16 (int client_fd);
uint32_t readUint32 (int client_fd);
uint64_t readUint64 (int client_fd);
int64_t readInt64 (int client_fd);
float readFloat (int client_fd);
double readDouble (int client_fd);

void readString (int client_fd);

uint32_t fast_rand ();
uint64_t splitmix64 (uint64_t state);

#ifdef ESP_PLATFORM
  #include "esp_timer.h"
  #define get_program_time esp_timer_get_time
#else
  int64_t get_program_time ();
#endif

#endif
