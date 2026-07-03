/* SPDX-License-Identifier: MIT */

/*
 * CLI command table for shell_app. cli_shell.c never includes this
 * file directly - shell_app.c pulls it in via the extern declarations
 * of g_shell_commands / g_shell_commands_count (see cli_shell.h for
 * the cli_shell_cmd_t contract each handler must follow).
 */

#include <string.h>
#include <stdio.h>

#include "cli_shell.h"
#include "board.h"
#include "logger.h"

static const char *TAG = "shell-cmds";

/* Forward declarations: handlers reference g_shell_commands (for
 * "help") before it is defined further down in this file. */
static void cmd_help(int argc, char **argv);
static void cmd_info(int argc, char **argv);
static void cmd_config(int argc, char **argv);

extern const cli_shell_cmd_t g_shell_commands[];
extern const size_t g_shell_commands_count;

static void shell_print(const char *str)
{
    cli_shell_send_str(str, strlen(str));
}

/* help: list every registered command with its one-line help text. */
static void cmd_help(int argc, char **argv)
{
    size_t i;

    (void)argc;
    (void)argv;

    LOGD(TAG, "help invoked");

    shell_print("Available commands:\r\n");
    for (i = 0; i < g_shell_commands_count; i++) {
        char line[80];
        int n = snprintf(line, sizeof(line), "  %-8s %s\r\n",
                          g_shell_commands[i].name, g_shell_commands[i].help);
        if (n < 0) {
            continue;
        }
        if ((size_t)n >= sizeof(line)) {
            n = (int)sizeof(line) - 1; /* snprintf truncated, clamp length */
        }
        cli_shell_send_str(line, (size_t)n);
    }
}

/* info: static device/network info. All values come from compile-time
 * defines in board.h - no runtime netif status API exists yet, so
 * this only reports the configured (not necessarily "up") state. */
static void cmd_info(int argc, char **argv)
{
    char line[64];
    int n;

    (void)argc;
    (void)argv;

    LOGD(TAG, "info invoked");

    shell_print("USB_ETH device info:\r\n");

    n = snprintf(line, sizeof(line), "  MAC:     %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                 MAC_BYTE1, MAC_BYTE2, MAC_BYTE3, MAC_BYTE4, MAC_BYTE5, MAC_BYTE6);
    if (n > 0) {
        cli_shell_send_str(line, (size_t)n);
    }

    n = snprintf(line, sizeof(line), "  IP:      %d.%d.%d.%d\r\n",
                 IP_BYTE1, IP_BYTE2, IP_BYTE3, IP_BYTE4);
    if (n > 0) {
        cli_shell_send_str(line, (size_t)n);
    }

    n = snprintf(line, sizeof(line), "  Netmask: %d.%d.%d.%d\r\n",
                 NETMASK_BYTE1, NETMASK_BYTE2, NETMASK_BYTE3, NETMASK_BYTE4);
    if (n > 0) {
        cli_shell_send_str(line, (size_t)n);
    }

    n = snprintf(line, sizeof(line), "  Gateway: %d.%d.%d.%d\r\n",
                 GW_BYTE1, GW_BYTE2, GW_BYTE3, GW_BYTE4);
    if (n > 0) {
        cli_shell_send_str(line, (size_t)n);
    }
}

/* config: fixed credit line, as requested. */
static void cmd_config(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    LOGD(TAG, "config invoked");

    shell_print("Dev by Logan+Claude\r\n");
}

const cli_shell_cmd_t g_shell_commands[] = {
    { "help",   cmd_help,   "Show this help message" },
    { "info",   cmd_info,   "Show device/network info" },
    { "config", cmd_config, "Show build/config credit" },
};

const size_t g_shell_commands_count =
    sizeof(g_shell_commands) / sizeof(g_shell_commands[0]);