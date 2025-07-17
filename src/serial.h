#ifndef ESP_SERIAL
#define ESP_SERIAL

#include <libubus.h>
#include <libserialport.h>
#include <ubus_wrapper.h>

enum {
        S_UNKNOWN_FN,
        S_GET_PORT,
        S_OPEN_PORT,
        S_WRITE,
        S_READ,
        S_ESP_CHECK,
        __S_MAX_FN
};

static const int ESP_VENDOR = 0x10c4;
static const int ESP_PRODUCT = 0xea60;

static const char GET_FMT[] = "{\"action\": \"get\", \"sensor\": \"%s\", \"pin\": %d, \"model\": \"%s\"}";
static const char PIN_FMT[] = "{\"action\": \"%s\", \"pin\": %d}";

static const int READ_TIMEOUT = 2000;

int s_handle_err(int err, struct sp_port *port, int fn, struct ubus_pkg *pkg);
int setup_port(char *portname, struct sp_port **port, struct ubus_pkg *pkg);
int populate_devices_blob(struct blob_buf *b);

#endif