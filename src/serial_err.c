#include "serial_err.h"
#include "log.h"
#include "ubus_wrapper.h"
#include <libubus.h>

const char *errors[] = {
        [S_GET_PORT] = "Failed to get port",
        [S_OPEN_PORT] = "Failed to open port",
        [S_WRITE] = "Failed to write in port",
        [S_READ] = "Failed to read from port",
};

int try_handle_err(int err, struct sp_port *port, int fn, struct blob_buf *b, struct ubus_context *ctx, struct ubus_request_data *req)
{
        if(err < 0) {
                char *errmsg = sp_last_error_message();
                write_log(LOG_ERR, "%s: %s", errors[fn], errmsg);
        
                blobmsg_add_u32(b, "rc", ERR_GET_FAILED);
                blobmsg_add_string(b, "msg", errmsg);
                ubus_send_reply(ctx, req, b->head);
                blob_buf_free(b);
        
                sp_free_error_message(errmsg);
                if(port != NULL) {
                        sp_close(port);
                        sp_free_port(port);
                }
                return err;
        }
        return 0;
}