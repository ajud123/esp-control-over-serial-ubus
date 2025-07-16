#include "log.h"
#include <signal.h>
#include <libubus.h>
#include <libserialport.h>
#include <string.h>
#include "ubus_wrapper.h"

static int state = 1;

void handle_exit(int signum)
{
	state = 0xFF;
	write_log(LOG_INFO, "Received signal %d, setting state to %d", signum, state);
        stop_ubus();
}

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	signal(SIGINT, handle_exit);
	signal(SIGTERM, handle_exit);
	signal(SIGQUIT, handle_exit);
	signal(SIGABRT, handle_exit);
	open_log();

	struct ubus_context *ctx;

	int status = initialise_ubus(&ctx);
        if(status != 0)
                return status;

	write_log(LOG_INFO, "Starting ubus...");
        run_ubus(ctx);
        cleanup_ubus(ctx);

	closelog();
}