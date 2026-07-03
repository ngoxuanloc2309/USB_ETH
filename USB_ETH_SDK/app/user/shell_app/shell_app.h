#ifndef SHELL_APP_H
#define SHELL_APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "task.h"

/* CLI is not real-time, keep it out of the way of the USB task and
 * tcpip_thread (see priority table, muc 5.12). */
#ifndef SHELL_APP_TASK_STACK_SIZE
#define SHELL_APP_TASK_STACK_SIZE   256
#endif

#ifndef SHELL_APP_TASK_PRIORITY
#define SHELL_APP_TASK_PRIORITY     (tskIDLE_PRIORITY + 2)
#endif

/**
 * @brief Wire up and initialize the shell service.
 *
 * Calls into services/shell (cli_shell) to set up its internal state
 * (command table, line buffer, ...). Does not start the task and
 * does not touch the transport yet - usb_cdc_transport_init() must
 * already have run before this, per the composition order in
 * app_init().
 */
void shell_app_init(void);

/**
 * @brief Start the shell task.
 *
 * Owns and creates the FreeRTOS task (this project's task-ownership
 * rule places task creation at the app layer for this module - see
 * architecture note, shell). The task loop repeatedly drives the
 * shell service (cli_shell) forward; the service itself does not
 * spawn any task of its own.
 */
void shell_app_start(void);

#ifdef __cplusplus
}
#endif

#endif