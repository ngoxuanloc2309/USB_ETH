#include <string.h>
#include "usb_cdc.h"
#include "cqueue.h"
#include "tusb.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "logger.h"

static const char *TAG = "USB_CDC";

static CQueue_t rx_queue;
static CQueue_t tx_queue;

static uint8_t rx_buf[USB_CDC_RX_QUEUE_LEN];
static uint8_t tx_buf[USB_CDC_TX_QUEUE_LEN];

/* cqueue.c has no internal locking (kept RTOS-agnostic on purpose).
 * Thread-safety between the TinyUSB task (producer for RX, consumer
 * for TX) and the shell task (consumer for RX, producer for TX) is
 * added here with a mutex per queue (design Option A). */
static SemaphoreHandle_t rx_mutex;
static SemaphoreHandle_t tx_mutex;

void usb_cdc_transport_init(void)
{
    cqueue_init_static(&rx_queue, rx_buf, USB_CDC_RX_QUEUE_LEN, sizeof(uint8_t));
    cqueue_init_static(&tx_queue, tx_buf, USB_CDC_TX_QUEUE_LEN, sizeof(uint8_t));

    rx_mutex = xSemaphoreCreateMutex();
    tx_mutex = xSemaphoreCreateMutex();

    if (rx_mutex == NULL || tx_mutex == NULL) {
        LOGE(TAG, "failed to create rx/tx mutex");
        return;
    }

    LOGI(TAG, "transport initialized, rx_len=%u tx_len=%u",
         (unsigned)USB_CDC_RX_QUEUE_LEN, (unsigned)USB_CDC_TX_QUEUE_LEN);
}

uint32_t usb_cdc_transport_read(uint8_t *buf, uint32_t max_len)
{
    uint32_t count = 0;

    if (buf == NULL || max_len == 0) {
        return 0;
    }

    xSemaphoreTake(rx_mutex, portMAX_DELAY);
    while (count < max_len && cqueue_receive(&rx_queue, &buf[count])) {
        count++;
    }
    xSemaphoreGive(rx_mutex);

    if (count > 0) {
        LOGD(TAG, "read %u bytes from rx queue", (unsigned)count);
    }

    return count;
}

uint32_t usb_cdc_transport_write(const uint8_t *buf, uint32_t len)
{
    uint32_t count = 0;

    if (buf == NULL || len == 0) {
        return 0;
    }

    xSemaphoreTake(tx_mutex, portMAX_DELAY);
    while (count < len && cqueue_send(&tx_queue, (void *)&buf[count])) {
        count++;
    }
    xSemaphoreGive(tx_mutex);

    if (count < len) {
        LOGW(TAG, "tx queue full, dropped %u of %u bytes",
             (unsigned)(len - count), (unsigned)len);
    }

    return count;
}

bool usb_cdc_transport_available(void)
{
    bool has_data;

    xSemaphoreTake(rx_mutex, portMAX_DELAY);
    has_data = !cqueue_is_empty(&rx_queue);
    xSemaphoreGive(rx_mutex);

    return has_data;
}

void usb_cdc_transport_flush(void)
{
    uint8_t byte;
    uint32_t flushed = 0;

    xSemaphoreTake(tx_mutex, portMAX_DELAY);
    /* Check tud_cdc_write_available() BEFORE popping from tx_queue:
     * cqueue has no "push back" on failure, popping first and finding
     * USB busy would silently lose that byte. */
    while (!cqueue_is_empty(&tx_queue) && tud_cdc_write_available() > 0) {
        cqueue_receive(&tx_queue, &byte);
        tud_cdc_write(&byte, 1);
        flushed++;
    }
    xSemaphoreGive(tx_mutex);

    if (flushed > 0) {
        tud_cdc_write_flush();
        LOGD(TAG, "flushed %u bytes to host", (unsigned)flushed);
    }
}

/* -------------------------------------------------------------------- */
/* TinyUSB CDC callbacks (called from the task running tud_task())      */
/* -------------------------------------------------------------------- */

void tud_cdc_rx_cb(uint8_t itf)
{
    uint8_t byte;
    uint32_t count = 0;

    (void)itf;

    xSemaphoreTake(rx_mutex, portMAX_DELAY);
    /* Same pattern as flush: check room in rx_queue BEFORE pulling the
     * byte out of TinyUSB's own CDC buffer. If rx_queue is full we
     * simply stop draining; TinyUSB keeps the remaining bytes in its
     * internal buffer (flow control), nothing is lost. */
    while (tud_cdc_available() > 0 && !cqueue_is_full(&rx_queue)) {
        if (tud_cdc_read(&byte, 1) != 1) {
            break;
        }
        cqueue_send(&rx_queue, &byte);
        count++;
    }
    xSemaphoreGive(rx_mutex);

    if (count > 0) {
        LOGD(TAG, "rx_cb: queued %u bytes", (unsigned)count);
    } else {
        LOGW(TAG, "rx_cb: rx queue full, bytes left buffered in TinyUSB");
    }
}

void tud_cdc_tx_complete_cb(uint8_t itf)
{
    (void)itf;
    LOGD(TAG, "tx_complete_cb");
}