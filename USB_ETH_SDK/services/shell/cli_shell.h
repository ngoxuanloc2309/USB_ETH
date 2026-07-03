#ifndef CLI_SHELL_H
#define CLI_SHELL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

/* -------------------------------------------------------------------- */
/* Command table                                                         */
/* -------------------------------------------------------------------- */
/* The command table itself lives at the app layer
 * (app/user/shell_app/shell_command_lists.c) - cli_shell.c only
 * knows this type, it never includes anything from app/. The table
 * is handed in at cli_shell_init() time. */

typedef void (*cli_shell_cmd_fn)(int argc, char **argv);

typedef struct {
    const char *name;      /* command name as typed on the CLI */
    cli_shell_cmd_fn fn;    /* handler, called with argv[0] = name */
    const char *help;       /* one-line help text */
} cli_shell_cmd_t;

/* -------------------------------------------------------------------- */
/* Shell service API                                                     */
/* -------------------------------------------------------------------- */
/* This service does not create or own any FreeRTOS task - the caller
 * (shell_app, at the app layer) owns the task and drives the shell
 * forward by calling cli_shell_process() from its own loop. */

/**
 * @brief Initialize the shell (line buffer, command table).
 *
 * Must be called before cli_shell_process(). Does not touch the
 * transport - usb_cdc_transport_init() must already have run before
 * this, per the composition order in app_init().
 *
 * @param cmds Command table, owned by the caller (must outlive the
 *             shell, typically a static const array).
 * @param cmd_count Number of entries in cmds.
 */
void cli_shell_init(const cli_shell_cmd_t *cmds, size_t cmd_count);

/**
 * @brief Run one iteration of the shell.
 *
 * Non-blocking: reads whatever bytes are currently available from
 * the USB CDC transport, feeds them into the line buffer, and
 * dispatches a command once a newline is seen. Meant to be called
 * repeatedly from the shell_app task loop.
 */
void cli_shell_process(void);

/* -------------------------------------------------------------------- */
/* Output API - for command handlers to print back to the user          */
/* -------------------------------------------------------------------- */
/* Command handlers (cli_shell_cmd_fn, implemented at the app layer in
 * shell_command_lists.c) cannot include usb_cdc.h themselves - they
 * go through these instead. cli_shell.c implements them on top of
 * usb_cdc_transport_write(). */

/**
 * @brief Send one character back to the shell user.
 */
void cli_shell_send_char(char c);

/**
 * @brief Send a buffer back to the shell user.
 * @param str Buffer to send (not required to be null-terminated).
 * @param len Number of bytes to send.
 */
void cli_shell_send_str(const char *str, size_t len);

/**
 * @brief Try to read one raw character from the shell input.
 *
 * Normally cli_shell_process() consumes input itself to build up
 * command lines - this is exposed for command handlers that need
 * interactive raw input (e.g. a "y/n" confirmation prompt) while a
 * command is running.
 *
 * @param c Output: received character, valid only if this returns true.
 * @return true if a character was available and *c was set, false otherwise.
 */
bool cli_shell_receive_char(char *c);

#ifdef __cplusplus
}
#endif

#endif