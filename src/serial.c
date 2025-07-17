#include "serial.h"
#include "log.h"
#include "ubus_wrapper.h"
#include <libubus.h>

struct err_msg {
	char *msg;
	int err;
};

const struct err_msg errors[] = {
	[S_UNKNOWN_FN] = { .msg = "Unknown function specified", .err = ERR_READ_FAILED },
	[S_GET_PORT]   = { .msg = "Failed to get port", .err = ERR_GET_FAILED },
	[S_OPEN_PORT]  = { .msg = "Failed to open port", .err = ERR_OPEN_FAILED },
	[S_WRITE]      = { .msg = "Failed to write in port", .err = ERR_WRITE_FAILED },
	[S_READ]       = { .msg = "Failed to read from port", .err = ERR_READ_FAILED },
	[S_ESP_CHECK]  = { .msg = "Failed to open port", .err = ERR_INVALID_DEVICE },
};

struct err_msg *get_error(int func)
{
	if (func < 0 || func >= __S_MAX_FN) {
		return &(errors[S_UNKNOWN_FN]);
	}
	return &(errors[func]);
}

int s_handle_err(int err, struct sp_port *port, int fn, struct ubus_pkg *pkg)
{
	if (err < 0) {
		char *errmsg	    = sp_last_error_message();
		struct err_msg *err = get_error(fn);
		write_log(LOG_ERR, "%s: %s", err->msg, errmsg);

		if (pkg != NULL) {
			blobmsg_add_u32(pkg->b, "rc", err->err);
			blobmsg_add_string(pkg->b, "msg", errmsg);
			ubus_send_reply(pkg->ctx, pkg->req, pkg->b->head);
			blob_buf_free(pkg->b);
		}

		sp_free_error_message(errmsg);
		if (port != NULL) {
			sp_close(port);
			sp_free_port(port);
		}
		return err;
	}
	return 0;
}

void configure_port(struct sp_port *port)
{
	sp_set_baudrate(port, 9600);
	sp_set_bits(port, 8);
	sp_set_parity(port, SP_PARITY_NONE);
	sp_set_stopbits(port, 1);
	sp_set_flowcontrol(port, SP_FLOWCONTROL_NONE);
}

int setup_port(char *portname, struct sp_port *port, struct ubus_pkg *pkg)
{
	write_log(LOG_INFO, "Getting port %s...", portname);
	if (try_handle_err(sp_get_port_by_name(portname, &port), port, S_GET_PORT, pkg) < 0)
		return UBUS_STATUS_INVALID_ARGUMENT;

	int vid = 0;
	int pid = 0;
	sp_get_port_usb_vid_pid(port, &vid, &pid);

	if (s_handle_err(vid != ESP_VENDOR && pid != ESP_PRODUCT, port, S_ESP_CHECK, pkg))
		return UBUS_STATUS_INVALID_ARGUMENT;

	write_log(LOG_INFO, "Opening port %s...", portname);
	if (try_handle_err(sp_open(port, SP_MODE_READ_WRITE), port, S_OPEN_PORT, pkg))
		return UBUS_STATUS_UNKNOWN_ERROR;

	write_log(LOG_INFO, "Setting up serial protocol for port %s...", portname);

	configure_port(port);

	return 0;
}

int populate_devices_blob(struct blob_buf *b)
{
        if(b == NULL)
                return UBUS_STATUS_UNKNOWN_ERROR;
	void *cookie = blobmsg_open_array(b, "devices");
	struct sp_port **port_list;

	enum sp_return result = sp_list_ports(&port_list);

	if (result != SP_OK) {
		write_log(LOG_ERR, "sp_list_ports() failed!\n");
		return UBUS_STATUS_UNKNOWN_ERROR;
	}
	for (int i = 0; port_list[i] != NULL; i++) {
		struct sp_port *port = port_list[i];
		/* Get the name of the port. */
		char *port_name = sp_get_port_name(port);
		int vid		= 0;
		int pid		= 0;
		sp_get_port_usb_vid_pid(port, &vid, &pid);
		if (vid != ESP_VENDOR && pid != ESP_PRODUCT)
			continue;
		void *device = blobmsg_open_table(&b, NULL);
		blobmsg_add_string(&b, "port", port_name);
		blobmsg_add_u32(&b, "vendor_id", vid);
		blobmsg_add_u32(&b, "product_id", pid);
		write_log(LOG_INFO, "Found port: %s\n", port_name);
		blobmsg_close_table(&b, device);
	}
	blobmsg_close_array(&b, cookie);
	sp_free_port_list(port_list);
        return 0;
}