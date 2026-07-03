/* SPDX-License-Identifier: MIT */

/*
 * App-layer wiring for the CLI shell. Owns the FreeRTOS task (the
 * service, cli_shell, does not create any task itself - see
 * services/shell/cli_shell.h). Supplies the command table from
 * shell_command_lists.c to the service.
 */

#include "shell_app.h"

#include "cli_shell.h"
#include "logger.h"

static const char *TAG = "shell-app";

static TaskHandle_t shell_app_task_handle;

/* Command table, defined at app/user/shell_app/shell_command_lists.c.
 * Declared extern here rather than via its own header for now - if a
 * shell_command_lists.h turns out to be needed (e.g. other app code
 * wants to reference it too), that is a separate step. */
extern const cli_shell_cmd_t g_shell_commands[];
extern const size_t g_shell_commands_count;

/* Polling period for cli_shell_process(). Shell is the lowest
 * priority task in the system (see priority table, muc 5.12) - this
 * value only bounds keystroke-echo latency, it does not affect
 * USB/lwip timing. */
#define SHELL_APP_POLL_PERIOD_MS   10

static void shell_app_task(void *arg)
{
    (void)arg;

    LOGI(TAG, "shell_app task running");

    for (;;) {
        cli_shell_process();
        vTaskDelay(pdMS_TO_TICKS(SHELL_APP_POLL_PERIOD_MS));
    }
}

void shell_app_init(void)
{
    cli_shell_init(g_shell_commands, g_shell_commands_count);
    LOGI(TAG, "shell_app initialized, %u commands", (unsigned)g_shell_commands_count);
}

void shell_app_start(void)
{
    BaseType_t ok = xTaskCreate(shell_app_task,
                                 "shell_app",
                                 SHELL_APP_TASK_STACK_SIZE,
                                 NULL,
                                 SHELL_APP_TASK_PRIORITY,
                                 &shell_app_task_handle);

    if (ok != pdPASS) {
        LOGE(TAG, "failed to create shell_app task");
        return;
    }

    LOGI(TAG, "shell_app task started");
}