#include "ubus_wrapper.h"
#include "log.h"
#include "serial_err.h"
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

	struct sp_port **port_list;

	enum sp_return result = sp_list_ports(&port_list);

	if (result != SP_OK) {
		write_log(LOG_ERR, "sp_list_ports() failed!\n");
		return -1;
	}
	int i;
	void *cookie = blobmsg_open_array(&b, "devices");
	for (i = 0; port_list[i] != NULL; i++) {
		struct sp_port *port = port_list[i];
		/* Get the name of the port. */
		char *port_name = sp_get_port_name(port);
		int vid		= 0;
		int pid		= 0;
		sp_get_port_usb_vid_pid(port, &vid, &pid);
		if (vid != 0x10c4 && pid != 0xea60)
			continue;
		void *device = blobmsg_open_table(&b, NULL);
		blobmsg_add_string(&b, "port", port_name);
		blobmsg_add_u32(&b, "vendor_id", vid);
		blobmsg_add_u32(&b, "product_id", pid);
		write_log(LOG_INFO, "Found port: %s\n", port_name);
		blobmsg_close_table(&b, device);
	}
	blobmsg_close_array(&b, cookie);
	ubus_send_reply(ctx, req, b.head);
	blob_buf_free(&b);

	sp_free_port_list(port_list);
	return 0;
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
	struct blob_buf b = {};
	blob_buf_init(&b, 0);

	blobmsg_parse(esp_pin_policy, __ESP_PIN_POL_MAX, tb, blob_data(msg), blob_len(msg));
	if (!tb[ESP_PIN_PIN] && !tb[ESP_PIN_PORT])
		return UBUS_STATUS_INVALID_ARGUMENT;

	char *portname = blobmsg_get_string(tb[ESP_PIN_PORT]);
	int pin	       = blobmsg_get_u32(tb[ESP_PIN_PIN]);

	struct sp_port *port = NULL;

	if (try_handle_err(sp_get_port_by_name(portname, &port), port, S_GET_PORT, &b, ctx, req) < 0)
		return UBUS_STATUS_INVALID_ARGUMENT;

	if (try_handle_err(sp_open(port, SP_MODE_READ_WRITE), port, S_OPEN_PORT, &b, ctx, req) < 0)
		return UBUS_STATUS_UNKNOWN_ERROR;

	sp_set_baudrate(port, 9600);
	sp_set_bits(port, 8);
	sp_set_parity(port, SP_PARITY_NONE);
	sp_set_stopbits(port, 1);
	sp_set_flowcontrol(port, SP_FLOWCONTROL_NONE);

	char outmsg[256];
	sprintf(outmsg, "{\"action\": \"%s\", \"pin\": %d}", method, pin);
        if(try_handle_err(sp_blocking_write(port, outmsg, strlen(outmsg), 0), port, S_WRITE, &b, ctx, req) < 0) 
		return UBUS_STATUS_UNKNOWN_ERROR;

	int bytes = 256;
	char *buf = calloc(bytes, sizeof(char));

	write_log(LOG_ERR, "Reading bytes in port %s: %s", portname);

        if(try_handle_err(sp_blocking_read(port, buf, bytes, 2000), port, S_WRITE, &b, ctx, req) < 0) 
		return UBUS_STATUS_UNKNOWN_ERROR;

	blobmsg_add_json_from_string(&b, buf);
	free(buf);

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
	struct blob_buf b = {};
	blob_buf_init(&b, 0);

	blobmsg_parse(esp_get_policy, __ESP_GET_POL_MAX, tb, blob_data(msg), blob_len(msg));
	if (!tb[ESP_GET_PIN] && !tb[ESP_GET_PORT] && !tb[ESP_GET_MODEL] && !tb[ESP_GET_SENSOR])
		return UBUS_STATUS_INVALID_ARGUMENT;

	char *portname = blobmsg_get_string(tb[ESP_GET_PORT]);
	int pin	       = blobmsg_get_u32(tb[ESP_GET_PIN]);
	char *sensor   = blobmsg_get_string(tb[ESP_GET_SENSOR]);
	char *model    = blobmsg_get_string(tb[ESP_GET_MODEL]);

	struct sp_port *port = NULL;

	if (sp_get_port_by_name(portname, &port) < 0) {
		char *errmsg = sp_last_error_message();
		write_log(LOG_ERR, "Failed to get port %s: %s", portname, errmsg);
		sp_free_error_message(errmsg);
		return UBUS_STATUS_INVALID_ARGUMENT;
	}
	if (sp_open(port, SP_MODE_READ_WRITE) < 0) {
		char *errmsg = sp_last_error_message();
		write_log(LOG_ERR, "Failed to open port %s: %s", portname, errmsg);
		sp_free_error_message(errmsg);
		sp_close(port);
		sp_free_port(port);
		return UBUS_STATUS_UNKNOWN_ERROR;
	}

	sp_set_baudrate(port, 9600);
	sp_set_bits(port, 8);
	sp_set_parity(port, SP_PARITY_NONE);
	sp_set_stopbits(port, 1);
	sp_set_flowcontrol(port, SP_FLOWCONTROL_NONE);

	char outmsg[256];
	sprintf(outmsg, "{\"action\": \"get\", \"sensor\": \"%s\", \"pin\": %d, \"model\": \"%s\"}", sensor,
		pin, model);
	sp_flush(port, SP_BUF_INPUT);
	if (sp_blocking_write(port, outmsg, strlen(outmsg) + 1, 0) < 0) {
		char *errmsg = sp_last_error_message();
		write_log(LOG_ERR, "Failed to write to port %s: %s", portname, errmsg);
		sp_free_error_message(errmsg);
		sp_close(port);
		sp_free_port(port);
		return UBUS_STATUS_UNKNOWN_ERROR;
	}
	int bytes = 256;
	char *buf = calloc(bytes, sizeof(char));

	if (sp_blocking_read(port, buf, bytes, 2000) < 0) {
		char *errmsg = sp_last_error_message();
		write_log(LOG_ERR, "Failed to get read bytes in port %s: %s", portname, errmsg);
		sp_free_error_message(errmsg);
		sp_close(port);
		sp_free_port(port);
		return UBUS_STATUS_UNKNOWN_ERROR;
	}
	blobmsg_add_json_from_string(&b, buf);
	free(buf);

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
