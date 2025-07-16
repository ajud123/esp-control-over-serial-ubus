#include <syslog.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include "log.h"

const int BUFFER_SIZE = 256;

void open_log()
{
	openlog(NULL, LOG_PID | LOG_NDELAY, LOG_DAEMON);
}

void write_log(int priority, const char *fmt, ...)
{
        char buffer[BUFFER_SIZE];
	va_list args;
	va_start(args, fmt);
        vsprintf(buffer, fmt, args);
	va_end(args);
        printf("%s\n", buffer);
        syslog(priority, "%s", buffer);
}

void close_log()
{
	closelog();
}