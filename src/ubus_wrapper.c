#include "ubus_wrapper.h"
#include "log.h"
#include "serial.h"
#include <libubox/blobmsg_json.h>
#include <libubus.h>
#include <ubusmsg.h>
#include <libserialport.h>

/*
 * The policy for the esp 'on' and 'off' methods
 */

enum { ESP_PIN_PORT, ESP_PIN_PIN, __ESP_PIN_POL_MAX };

static const struct blobmsg_policy esp_pin_policy[] = {
	[ESP_PIN_PORT] = { .name = "port", .type = BLOBMSG_TYPE_STRING },
	[ESP_PIN_PIN]  = { .name = "pin", .type = BLOBMSG_TYPE_INT32 }
};

/*
 * The policy for the esp 'get' method
 */

enum { ESP_GET_PORT, ESP_GET_PIN, ESP_GET_SENSOR, ESP_GET_MODEL, __ESP_GET_POL_MAX };

static const struct blobmsg_policy esp_get_policy[] = {
	[ESP_GET_PORT]	 = { .name = "port", .type = BLOBMSG_TYPE_STRING },
	[ESP_GET_PIN]	 = { .name = "pin", .type = BLOBMSG_TYPE_INT32 },
	[ESP_GET_SENSOR] = { .name = "sensor", .type = BLOBMSG_TYPE_STRING },
	[ESP_GET_MODEL]	 = { .name = "model", .type = BLOBMSG_TYPE_STRING }
};

/*
 * The method to get all connected ESP devices
 */

int esp_devices(struct ubus_context *ctx, struct ubus_object *obj, struct ubus_request_data *req,
		const char *method, struct blob_attr *msg)
{
	(void)method;
	(void)msg;
	(void)req;
	(void)obj;
	(void)ctx;
	struct blob_buf b = {};
	blob_buf_init(&b, 0);

        int ret = populate_devices_blob(&b);

	ubus_send_reply(ctx, req, b.head);
	blob_buf_free(&b);

	return ret;
};

/*
 * The method to order an ESP device to toggle a pin
 */
int esp_pin(struct ubus_context *ctx, struct ubus_object *obj, struct ubus_request_data *req,
	    const char *method, struct blob_attr *msg)
{
	(void)req;
	(void)ctx;
	(void)obj;
	struct blob_attr *tb[__ESP_PIN_POL_MAX];
	struct blob_buf b   = {};
	struct ubus_pkg pkg = { .b = &b, .ctx = ctx, .req = req };
	blob_buf_init(&b, 0);

	blobmsg_parse(esp_pin_policy, __ESP_PIN_POL_MAX, tb, blob_data(msg), blob_len(msg));
	if (!tb[ESP_PIN_PIN] && !tb[ESP_PIN_PORT])
		return UBUS_STATUS_INVALID_ARGUMENT;

	char *portname = blobmsg_get_string(tb[ESP_PIN_PORT]);
	int pin	       = blobmsg_get_u32(tb[ESP_PIN_PIN]);

	struct sp_port *port = NULL;

	int status = setup_port(portname, port, &pkg);
        if(status != 0)
                return status;

	char outmsg[256];
	sprintf(outmsg, PIN_FMT, method, pin);

	write_log(LOG_INFO, "Writing message to port %s...", portname);
	if (s_handle_err(sp_blocking_write(port, outmsg, strlen(outmsg), 0), port, S_WRITE, &pkg) < 0)
		return UBUS_STATUS_UNKNOWN_ERROR;
	write_log(LOG_INFO, "Writing to port %s finished", portname);

	char buf[256];
        memset(buf, '\0', sizeof(char)*256);

	write_log(LOG_ERR, "Reading bytes in port %s", portname);
	if (s_handle_err(sp_blocking_read(port, buf, 256, READ_TIMEOUT), port, S_WRITE, &pkg) < 0)
		return UBUS_STATUS_UNKNOWN_ERROR;
	write_log(LOG_ERR, "Reading from port %s finished", portname);

	blobmsg_add_json_from_string(&b, buf);

	ubus_send_reply(ctx, req, b.head);
	blob_buf_free(&b);

	sp_close(port);
	sp_free_port(port);

	return 0;
}

/*
 * The method to order an ESP device to get information about a sensor
 */
int esp_get(struct ubus_context *ctx, struct ubus_object *obj, struct ubus_request_data *req,
	    const char *method, struct blob_attr *msg)
{
	(void)method;
	(void)msg;
	(void)req;
	(void)obj;
	(void)ctx;
	struct blob_attr *tb[__ESP_GET_POL_MAX];
	struct blob_buf b   = {};
	struct ubus_pkg pkg = { .b = &b, .ctx = ctx, .req = req };
	blob_buf_init(&b, 0);

	blobmsg_parse(esp_get_policy, __ESP_GET_POL_MAX, tb, blob_data(msg), blob_len(msg));
	if (!tb[ESP_GET_PIN] && !tb[ESP_GET_PORT] && !tb[ESP_GET_MODEL] && !tb[ESP_GET_SENSOR])
		return UBUS_STATUS_INVALID_ARGUMENT;

	char *portname = blobmsg_get_string(tb[ESP_GET_PORT]);
	int pin	       = blobmsg_get_u32(tb[ESP_GET_PIN]);
	char *sensor   = blobmsg_get_string(tb[ESP_GET_SENSOR]);
	char *model    = blobmsg_get_string(tb[ESP_GET_MODEL]);

	struct sp_port *port = NULL;

	int status = setup_port(portname, port, &pkg);
        if(status != 0)
                return status;

	char outmsg[256];
	sprintf(outmsg, GET_FMT, sensor, pin, model);

	sp_flush(port, SP_BUF_INPUT);

	write_log(LOG_INFO, "Writing message to port %s...", portname);
	if (s_handle_err(sp_blocking_write(port, outmsg, strlen(outmsg) + 1, 0), port, S_WRITE, &pkg) < 0)
		return UBUS_STATUS_UNKNOWN_ERROR;
	write_log(LOG_INFO, "Writing to port %s finished", portname);

	char buf[256];
        memset(buf, '\0', sizeof(char)*256);

	write_log(LOG_ERR, "Reading bytes in port %s", portname);
	if (s_handle_err(sp_blocking_read(port, buf, 256, READ_TIMEOUT), port, S_READ, &pkg) < 0)
		return UBUS_STATUS_UNKNOWN_ERROR;
	blobmsg_add_json_from_string(&b, buf);

	ubus_send_reply(ctx, req, b.head);
	blob_buf_free(&b);

	sp_close(port);
	sp_free_port(port);
	return 0;
}

/*
 * This structure is used to define the methods of our ubus object
 */

static const struct ubus_method esp_methods[] = { UBUS_METHOD_NOARG("devices", esp_devices),
						  UBUS_METHOD("on", esp_pin, esp_pin_policy),
						  UBUS_METHOD("off", esp_pin, esp_pin_policy),
						  UBUS_METHOD("get", esp_get, esp_get_policy) };

/*
 * This structure is used to define the type of our object with methods.
 * */

static struct ubus_object_type esp_object_type = UBUS_OBJECT_TYPE("esp", esp_methods);

/*
 * This structure is used to register our program as an ubus object
 * with our methods and other neccessary data. 
 * */

static struct ubus_object esp_object = {
	.name	   = "esp",
	.type	   = &esp_object_type,
	.methods   = esp_methods,
	.n_methods = ARRAY_SIZE(esp_methods),
};

int initialise_ubus(struct ubus_context **ctx)
{
	write_log(LOG_INFO, "Initialising uloop...");
	int status = uloop_init();
	if (status != 0) {
		write_log(LOG_CRIT, "Failed to initialize uloop, error code %d", status);
		closelog();
		return status;
	}

	write_log(LOG_INFO, "Connecting to ubus...");
	*ctx = ubus_connect(NULL);
	if (!(*ctx)) {
		write_log(LOG_CRIT, "Failed to connect to ubus.");
		uloop_done();
		closelog();
		return -1;
	}

	ubus_add_uloop(*ctx);
	status = ubus_add_object(*ctx, &esp_object);

	if (status != 0) {
		write_log(LOG_CRIT, "Failed to initialize add esp object: %s", ubus_strerror(status));
		ubus_free(*ctx);
		uloop_done();
		closelog();
		return -2;
	}
	return 0;
}

void run_ubus()
{
	uloop_run();
}

void cleanup_ubus(struct ubus_context *ctx)
{
	ubus_free(ctx);
	uloop_done();
}

void stop_ubus()
{
	uloop_end();
}
