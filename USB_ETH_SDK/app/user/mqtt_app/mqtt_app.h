#ifndef MQTT_APP_H
#define MQTT_APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "task.h"

/* MQTT is not real-time, same reasoning as shell_app - keep it out of
 * the way of the USB task and tcpip_thread (see priority table). */
#ifndef MQTT_APP_TASK_STACK_SIZE
#define MQTT_APP_TASK_STACK_SIZE   1024
#endif

#ifndef MQTT_APP_TASK_PRIORITY
#define MQTT_APP_TASK_PRIORITY     (tskIDLE_PRIORITY + 2)
#endif

/**
 * @brief Build the MQTT config (from app_config.h) and create the
 *        mqtt_service handle. Does not connect yet.
 *
 * Must be called before mqtt_app_start(), after usb_netif_init() (the
 * network interface must exist before DNS/TCP can do anything -
 * mqtt_app_start() itself blocks inside its own task until connected,
 * so the ordering only matters relative to netif existing at all, not
 * being fully up).
 */
void mqtt_app_init(void);

/**
 * @brief Start the MQTT task.
 *
 * Owns and creates the FreeRTOS task (task-ownership rule: this
 * project places task creation at the app layer for mqtt, same as
 * shell_app). The task connects (blocking, inside its own task - does
 * not stall app_init()), subscribes to MQTT_TEST_PUB_TOPIC, then
 * publishes a test message to that same topic every 10s so the
 * round-trip can be observed via mqtt_sv_data_incoming().
 *
 * Option A (see design discussion): if connect fails, the task logs
 * the error and exits - no auto-reconnect in this version.
 */
void mqtt_app_start(void);

#ifdef __cplusplus
}
#endif

#endif /* MQTT_APP_H */