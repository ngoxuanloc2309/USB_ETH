#ifndef USB_CDC_H
#define USB_CDC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* Depth of the RX and TX byte queues (cqueue), in bytes.
 * RX: bytes received from host, waiting to be consumed by the shell.
 * TX: bytes queued by the shell, waiting to be sent to host. */
#define USB_CDC_RX_QUEUE_LEN  256
#define USB_CDC_TX_QUEUE_LEN  256

/**
 * @brief Initialize the USB CDC transport layer.
 *
 * Must be called once before TinyUSB starts processing CDC events
 * (before tud_task() runs). Allocates and initializes the RX/TX
 * cqueue and their protecting mutex (see usb_cdc.c, Option A:
 * cqueue itself has no locking, thread-safety is added here).
 */
void usb_cdc_transport_init(void);

/**
 * @brief Read bytes received from the host (non-blocking).
 *
 * Called by the consumer (services/shell) from its own task context.
 * Pulls up to max_len bytes out of the RX queue.
 *
 * @param buf      destination buffer
 * @param max_len  capacity of buf
 * @return number of bytes actually read (0 if RX queue is empty)
 */
uint32_t usb_cdc_transport_read(uint8_t *buf, uint32_t max_len);

/**
 * @brief Queue bytes to be sent to the host (non-blocking).
 *
 * Called by the consumer (services/shell) to send a response.
 * Bytes are pushed into the TX queue; call usb_cdc_transport_flush()
 * to push them out over USB.
 *
 * @param buf  source buffer
 * @param len  number of bytes to write
 * @return number of bytes actually queued (less than len if TX queue
 *         is full)
 */
uint32_t usb_cdc_transport_write(const uint8_t *buf, uint32_t len);

/**
 * @brief Check if there is data available to read from the host.
 *
 * @return true if the RX queue has at least one byte pending.
 */
bool usb_cdc_transport_available(void);

/**
 * @brief Force the TX queue content to be sent over USB now.
 *
 * TinyUSB CDC buffers writes internally (tud_cdc_write), this pushes
 * them out immediately instead of waiting for the internal buffer to
 * fill up.
 */
void usb_cdc_transport_flush(void);

#ifdef __cplusplus
}
#endif

#endif /* USB_CDC_H */