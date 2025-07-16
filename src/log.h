#ifndef ESP_LOG
#define ESP_LOG

#include <syslog.h>
void open_log();
void write_log(int priority, const char *fmt, ...);
void close_log();

#endif