#include <libubus.h>
#include <libserialport.h>
enum {
        S_GET_PORT,
        S_OPEN_PORT,
        S_WRITE,
        S_READ,
};



int try_handle_err(int err, struct sp_port *port, int fn, struct blob_buf *b, struct ubus_context *ctx, struct ubus_request_data *req);