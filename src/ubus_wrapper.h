#include <libubus.h>
enum { ERR_GET_FAILED = 50, ERR_OPEN_FAILED, ERR_READ_FAILED, ERR_WRITE_FAILED, ERR_INVALID_DEVICE, ERR_UNKNOWN_FN };

struct ubus_pkg {
	struct blob_buf *b;
	struct ubus_context *ctx;
	struct ubus_request_data *req;
};

/*
 * Initialises the ubus and uloop systems.
 * On failure, automatically cleans up the necessary systems that were created.
 * 
 * @returns
 * `0` on success, otherwise an internal error code on failure.
 */
int initialise_ubus(struct ubus_context **ctx);

void run_ubus();

void cleanup_ubus(struct ubus_context *ctx);

void stop_ubus();