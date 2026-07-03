/* SPDX-License-Identifier: MIT */

/*
 * CLI shell service. Reads bytes from the USB CDC transport, builds
 * up command lines, and dispatches them against a command table
 * supplied by the caller (app/user/shell_app). Does not own any
 * FreeRTOS task - cli_shell_process() is meant to be called
 * repeatedly from the shell_app task loop.
 */

#include "cli_shell.h"

#include <string.h>

#include "usb_cdc.h"
#include "logger.h"

static const char *TAG = "cli-shell";

#define CLI_SHELL_LINE_MAX   128
#define CLI_SHELL_MAX_ARGS   8

static char line_buf[CLI_SHELL_LINE_MAX];
static size_t line_len;

static const cli_shell_cmd_t *cmd_table;
static size_t cmd_table_count;

/* -------------------------------------------------------------------- */
/* Output API                                                            */
/* -------------------------------------------------------------------- */

void cli_shell_send_char(char c)
{
    usb_cdc_transport_write((const uint8_t *)&c, 1);
}

void cli_shell_send_str(const char *str, size_t len)
{
    usb_cdc_transport_write((const uint8_t *)str, (uint32_t)len);
}

bool cli_shell_receive_char(char *c)
{
    return usb_cdc_transport_read((uint8_t *)c, 1) == 1;
}

/* -------------------------------------------------------------------- */
/* Command dispatch                                                      */
/* -------------------------------------------------------------------- */

/* Splits line in place on whitespace (like strtok, but self-contained
 * so we do not depend on strtok's hidden static state). Only called
 * from cli_shell_process()'s task context, one line at a time -
 * reusing static storage is safe. */
static int cli_shell_tokenize(char *line, char *argv[], int max_args)
{
    int argc = 0;
    char *p = line;

    while (*p != '\0' && argc < max_args) {
        while (*p == ' ') {
            p++;
        }
        if (*p == '\0') {
            break;
        }

        argv[argc++] = p;

        while (*p != '\0' && *p != ' ') {
            p++;
        }
        if (*p == ' ') {
            *p = '\0';
            p++;
        }
    }

    return argc;
}

static void cli_shell_dispatch(char *line)
{
    char *argv[CLI_SHELL_MAX_ARGS];
    int argc = cli_shell_tokenize(line, argv, CLI_SHELL_MAX_ARGS);
    size_t i;

    if (argc == 0) {
        return;
    }

    for (i = 0; i < cmd_table_count; i++) {
        if (strcmp(argv[0], cmd_table[i].name) == 0) {
            LOGD(TAG, "dispatch: %s (argc=%d)", argv[0], argc);
            cmd_table[i].fn(argc, argv);
            return;
        }
    }

    LOGW(TAG, "unknown command: %s", argv[0]);
    cli_shell_send_str(argv[0], strlen(argv[0]));
    cli_shell_send_str(": command not found\r\n", 22);
}

/* -------------------------------------------------------------------- */
/* Line editing                                                          */
/* -------------------------------------------------------------------- */

static void cli_shell_prompt(void)
{
    cli_shell_send_str("> ", 2);
}

static void cli_shell_handle_byte(uint8_t byte)
{
    if (byte == '\r' || byte == '\n') {
        cli_shell_send_str("\r\n", 2);

        if (line_len > 0) {
            line_buf[line_len] = '\0';
            cli_shell_dispatch(line_buf);
            line_len = 0;
        }

        cli_shell_prompt();
        return;
    }

    if (byte == 0x08 || byte == 0x7F) {
        /* backspace / delete: erase last char, both in our buffer and
         * visually on the terminal (back, space, back). */
        if (line_len > 0) {
            line_len--;
            cli_shell_send_str("\b \b", 3);
        }
        return;
    }

    if (line_len >= CLI_SHELL_LINE_MAX - 1) {
        LOGW(TAG, "line buffer full (%u), char dropped", (unsigned)CLI_SHELL_LINE_MAX);
        return;
    }

    line_buf[line_len++] = (char)byte;
    cli_shell_send_char((char)byte); /* local echo */
}

/* -------------------------------------------------------------------- */
/* Public API                                                            */
/* -------------------------------------------------------------------- */

void cli_shell_init(const cli_shell_cmd_t *cmds, size_t cmd_count)
{
    cmd_table = cmds;
    cmd_table_count = cmd_count;
    line_len = 0;

    LOGI(TAG, "shell initialized, %u commands registered", (unsigned)cmd_count);
}

void cli_shell_process(void)
{
    uint8_t byte;
    bool got_data = false;

    while (usb_cdc_transport_read(&byte, 1) == 1) {
        cli_shell_handle_byte(byte);
        got_data = true;
    }

    if (got_data) {
        usb_cdc_transport_flush();
    }
}