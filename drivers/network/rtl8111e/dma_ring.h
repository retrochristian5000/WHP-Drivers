/* RTL8168/RTL8111E 16-byte descriptor DMA core for WHP-Drivers.
 * Hardware-independent ring mechanics; NOT an NDIS memory allocator.
 * A verified platform supplies bus addresses, cache maintenance, ordering,
 * serialization, PCI resources and completion/interrupt wiring.
 */
#ifndef WHP_RTL8111E_DMA_RING_H
#define WHP_RTL8111E_DMA_RING_H
#include <stdint.h>
#include "miniport_core.h"

#define WHP_DMA_SLOTS 8u
#define WHP_DMA_DESC_BYTES 16u
#define WHP_DMA_RING_ALIGN 256u
#define WHP_DMA_FRAME_MAX 2048u
#define WHP_DMA_OWN 0x80000000u
#define WHP_DMA_EOR 0x40000000u
#define WHP_DMA_FS  0x20000000u
#define WHP_DMA_LS  0x10000000u
#define WHP_DMA_RX_ERROR 0x00200000u
#define WHP_DMA_SIZE_MASK 0x00003fffu

/* Byte arrays make little-endian wire layout independent of host endian
 * and forbid unaligned uint64_t accesses. Array size/offset are exact. */
struct whp_dma_desc { uint8_t data[WHP_DMA_DESC_BYTES]; };

enum whp_dma_result {
    WHP_DMA_OK = 0, WHP_DMA_BAD_ARG, WHP_DMA_BAD_STATE,
    WHP_DMA_BUSY, WHP_DMA_SYNC_FAILED, WHP_DMA_DROPPED
};

/* All hooks are MANDATORY. They must implement real platform DMA/cache
 * synchronization and ownership order, NOT merely compiler fences.
 * before_device: synchronize the packet buffer for DMA (may fail).
 * prepare_desc: synchronize address/options and release barrier BEFORE OWN.
 * release: publish the OWN-bit descriptor and write barrier AFTER OWN.
 * acquire: sync descriptor for CPU so OWN can be read safely.
 * after_own: DMA read barrier AFTER observing OWN cleared.
 * before_cpu: synchronize completed receive buffer for CPU consumption.
 * No function writes MMIO, enables bus mastering or allocates DMA memory.
 */
struct whp_dma_ops {
    int (*before_device)(void *cookie, uint64_t bus, uint16_t size, int rx);
    void (*prepare_desc)(void *cookie, struct whp_dma_desc *desc);
    void (*release)(void *cookie, struct whp_dma_desc *desc);
    void (*acquire)(void *cookie, struct whp_dma_desc *desc);
    void (*after_own)(void *cookie);
    void (*before_cpu)(void *cookie, uint64_t bus, uint16_t size);
};

struct whp_dma_ring {
    struct whp_dma_desc *descs;
    struct whp_rtl8111e_mp *mp;
    struct whp_dma_ops ops;
    void *cookie;
    uint64_t ring_bus;
    uint64_t buffer_bus[WHP_DMA_SLOTS];
    uint16_t buffer_size[WHP_DMA_SLOTS];
    struct whp_rtl8111e_mp_ticket ticket[WHP_DMA_SLOTS];
    uint8_t active[WHP_DMA_SLOTS];
    unsigned int producer, consumer, pending;
    int rx;
};

/* The ring structure must be zero initialized before its first bind.
 * Ring and buffer bus addresses are supplied, already mapped by a trusted
 * DMA allocator. Conservative 32-bit bus address ceiling until historical
 * NDIS 3.1 physical-address handling is validated. */
enum whp_dma_result whp_dma_ring_bind(struct whp_dma_ring *r,
    struct whp_dma_desc *descs, uint64_t ring_bus,
    struct whp_rtl8111e_mp *mp, const struct whp_dma_ops *ops,
    void *cookie, int rx);

enum whp_dma_result whp_dma_submit(struct whp_dma_ring *r,
    uint64_t buffer_bus, uint16_t length, unsigned int *index);

/* Polls oldest outstanding descriptor, strictly in ring order.
 * WHP_DMA_OK means completion observed; NOT acknowledged or packet success.
 * TX status is raw: only a verified chip driver can classify error bits.
 * RX length excludes FCS; fragmented/error frames are dropped.
 * Returns descriptor results but KEEPS the buffer tracked as CPU-held.
 * Caller MUST finish NDIS completion/copy before whp_dma_ack().
 * Even dropped RX descriptors require acknowledgment to release ownership.
 */
enum whp_dma_result whp_dma_complete(struct whp_dma_ring *r,
    unsigned int *index, uint16_t *length, uint32_t *status);

/* Acknowledge CPU-held descriptor only after NDIS send completion or RX
 * payload copy / drop; never free/overwrite the DMA buffer before ACK. */
enum whp_dma_result whp_dma_ack(struct whp_dma_ring *r, unsigned int index);

/* Requires HALTING/HALTED/RESETTING state, NIC DMA/IRQ independently
 * quiesced, and all descriptors returned. Does not disable the device.
 * Never free the backing memory on WHP_DMA_BUSY/BAD_STATE. */
enum whp_dma_result whp_dma_unbind(struct whp_dma_ring *r);

uint32_t whp_dma_desc_word(const struct whp_dma_desc *desc, unsigned int word);
void whp_dma_desc_write_word(struct whp_dma_desc *desc, unsigned int word,
                             uint32_t value);
#endif
