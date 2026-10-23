#include "dma_ring.h"
#include <stddef.h>

_Static_assert(sizeof(struct whp_dma_desc) == 16u, "RTL descriptor must be 16 bytes");

uint32_t whp_dma_desc_word(const struct whp_dma_desc *d, unsigned int word)
{
    unsigned int k;
    uint32_t value = 0;
    if (!d || word >= 4u)
        return 0;
    for (k = 0; k < 4u; ++k)
        value |= (uint32_t)d->data[4u * word + k] << (8u * k);
    return value;
}

void whp_dma_desc_write_word(struct whp_dma_desc *d, unsigned int word,
                             uint32_t value)
{
    unsigned int k;
    if (!d || word >= 4u)
        return;
    for (k = 0; k < 4u; ++k)
        d->data[4u * word + k] = (uint8_t)(value >> (8u * k));
}

static int range_ok(uint64_t bus, uint32_t length)
{
    /* Even on a 64-bit host, never invent NT 3.51 64-bit bus DMA support. */
    return bus != 0 && bus <= UINT32_MAX &&
           length != 0 && length <= WHP_DMA_FRAME_MAX &&
           bus + (uint64_t)length - 1u <= UINT32_MAX;
}

static uint32_t eor(unsigned int i)
{
    return i == WHP_DMA_SLOTS - 1u ? WHP_DMA_EOR : 0u;
}

enum whp_dma_result whp_dma_ring_bind(struct whp_dma_ring *r,
    struct whp_dma_desc *descs, uint64_t ring_bus,
    struct whp_rtl8111e_mp *mp, const struct whp_dma_ops *ops,
    void *cookie, int rx)
{
    unsigned int i, j;
    if (!r || !descs || !mp || !ops || !ops->before_device ||
        !ops->prepare_desc || !ops->release || !ops->acquire ||
        !ops->after_own || !ops->before_cpu ||
        (rx != 0 && rx != 1) ||
        ((uintptr_t)descs & (WHP_DMA_RING_ALIGN - 1u)) != 0 ||
        (ring_bus & (WHP_DMA_RING_ALIGN - 1u)) != 0 ||
        ring_bus == 0 ||
        ring_bus + (uint64_t)WHP_DMA_SLOTS * WHP_DMA_DESC_BYTES - 1u > UINT32_MAX)
        return WHP_DMA_BAD_ARG;
    if (mp->state != WHP_MP_RUNNING)
        return WHP_DMA_BAD_STATE;
    /* Caller must not rebind an active ring. Bind only fresh/cleared state.
     * No direct PCI/NDIS/physical-memory operations are performed here. */
    if (r->descs || r->pending)
        return WHP_DMA_BAD_STATE;
    r->descs = descs;
    r->mp = mp;
    r->ops = *ops;
    r->cookie = cookie;
    r->ring_bus = ring_bus;
    r->producer = 0;
    r->consumer = 0;
    r->pending = 0;
    r->rx = rx;
    for (i = 0; i < WHP_DMA_SLOTS; ++i) {
        r->active[i] = 0;
        r->buffer_bus[i] = 0;
        r->buffer_size[i] = 0;
        r->ticket[i].slot = 0;
        r->ticket[i].serial = 0;
        for (j = 0; j < WHP_DMA_DESC_BYTES; ++j)
            descs[i].data[j] = 0;
        whp_dma_desc_write_word(&descs[i], 0, eor(i));
    }
    return WHP_DMA_OK;
}

enum whp_dma_result whp_dma_submit(struct whp_dma_ring *r,
    uint64_t bus, uint16_t length, unsigned int *index)
{
    unsigned int i;
    struct whp_dma_desc *d;
    struct whp_rtl8111e_mp_ticket ticket;
    enum whp_rtl8111e_mp_result mp_result;
    if (!r || !r->descs || !r->mp || !index)
        return WHP_DMA_BAD_ARG;
    if (r->mp->state != WHP_MP_RUNNING)
        return WHP_DMA_BAD_STATE;
    if (!range_ok(bus, length) ||
        (!r->rx && length < 60u) || (r->rx && length < 64u))
        return WHP_DMA_BAD_ARG;
    if (r->pending >= WHP_DMA_SLOTS)
        return WHP_DMA_BUSY;
    i = r->producer;
    if (r->active[i])
        return WHP_DMA_BUSY;
    mp_result = r->rx ? whp_rtl8111e_mp_rx_begin(r->mp, &ticket) :
                        whp_rtl8111e_mp_tx_begin(r->mp, &ticket);
    if (mp_result != WHP_MP_OK)
        return WHP_DMA_BUSY;
    if (!r->ops.before_device(r->cookie, bus, length, r->rx)) {
        if (r->rx)
            (void)whp_rtl8111e_mp_rx_retire(r->mp, ticket);
        else
            (void)whp_rtl8111e_mp_tx_retire(r->mp, ticket);
        return WHP_DMA_SYNC_FAILED;
    }
    d = &r->descs[i];
    whp_dma_desc_write_word(d, 1, 0);
    whp_dma_desc_write_word(d, 2, (uint32_t)bus);
    whp_dma_desc_write_word(d, 3, 0);
    /* Synchronize descriptor address/options while device owns nothing.
     * It is CRITICAL not to expose OWN before this hook returns. */
    whp_dma_desc_write_word(d, 0, eor(i) |
        (r->rx ? 0u : WHP_DMA_FS | WHP_DMA_LS) | length);
    r->ops.prepare_desc(r->cookie, d);
    /* Publish OWN as the final field, followed by DMA descriptor flush and
     * a platform write barrier before the caller rings any doorbell. */
    whp_dma_desc_write_word(d, 0, WHP_DMA_OWN | eor(i) |
        (r->rx ? 0u : WHP_DMA_FS | WHP_DMA_LS) | length);
    r->ops.release(r->cookie, d);
    r->ticket[i] = ticket;
    r->active[i] = 1;
    r->buffer_bus[i] = bus;
    r->buffer_size[i] = length;
    ++r->pending;
    r->producer = (i + 1u) % WHP_DMA_SLOTS;
    *index = i;
    return WHP_DMA_OK;
}

enum whp_dma_result whp_dma_complete(struct whp_dma_ring *r,
    unsigned int *index, uint16_t *length, uint32_t *status)
{
    unsigned int i;
    uint32_t word, data_len;
    struct whp_dma_desc *d;
    int dropped;
    if (!r || !r->descs || !r->mp || !index || !length || !status)
        return WHP_DMA_BAD_ARG;
    if (r->mp->state != WHP_MP_RUNNING &&
        r->mp->state != WHP_MP_RESETTING &&
        r->mp->state != WHP_MP_HALTING)
        return WHP_DMA_BAD_STATE;
    if (!r->pending)
        return WHP_DMA_BUSY;
    i = r->consumer;
    if (r->active[i] != 1)
        return WHP_DMA_BAD_STATE;
    d = &r->descs[i];
    r->ops.acquire(r->cookie, d);
    word = whp_dma_desc_word(d, 0);
    if (word & WHP_DMA_OWN)
        return WHP_DMA_BUSY;
    r->ops.after_own(r->cookie);
    /* Reject corrupt EOR and changed DMA address; do not free hardware
     * mappings or drop miniport ownership on suspicious descriptor data. */
    if ((word & WHP_DMA_EOR) != eor(i) ||
        whp_dma_desc_word(d, 2) != (uint32_t)r->buffer_bus[i] ||
        whp_dma_desc_word(d, 3) != 0)
        return WHP_DMA_BAD_STATE;
    data_len = word & WHP_DMA_SIZE_MASK;
    dropped = 0;
    if (r->rx) {
        if ((word & (WHP_DMA_FS | WHP_DMA_LS)) !=
                 (WHP_DMA_FS | WHP_DMA_LS) ||
            (word & WHP_DMA_RX_ERROR) ||
            data_len < 4u || data_len > r->buffer_size[i])
            dropped = 1;
        else
            r->ops.before_cpu(r->cookie, r->buffer_bus[i], (uint16_t)data_len);
        *length = dropped ? 0u : (uint16_t)(data_len - 4u);
    } else {
        *length = r->buffer_size[i];
    }
    *status = word;
    *index = i;
    /* Device has surrendered OWN, but NDIS still may be using the buffer.
     * Retain miniport and ring ownership until explicit ACK. */
    r->active[i] = 2;
    return dropped ? WHP_DMA_DROPPED : WHP_DMA_OK;
}

enum whp_dma_result whp_dma_ack(struct whp_dma_ring *r, unsigned int index)
{
    enum whp_rtl8111e_mp_result mp_result;
    struct whp_dma_desc *d;
    if (!r || !r->descs || !r->mp || index >= WHP_DMA_SLOTS)
        return WHP_DMA_BAD_ARG;
    if (r->mp->state != WHP_MP_RUNNING &&
        r->mp->state != WHP_MP_RESETTING &&
        r->mp->state != WHP_MP_HALTING)
        return WHP_DMA_BAD_STATE;
    if (index != r->consumer || r->active[index] != 2)
        return WHP_DMA_BAD_STATE;
    mp_result = r->rx ? whp_rtl8111e_mp_rx_retire(r->mp, r->ticket[index]) :
                        whp_rtl8111e_mp_tx_retire(r->mp, r->ticket[index]);
    if (mp_result != WHP_MP_OK)
        return WHP_DMA_BAD_STATE;
    d = &r->descs[index];
    r->active[index] = 0;
    r->buffer_bus[index] = 0;
    r->buffer_size[index] = 0;
    r->ticket[index].serial = 0;
    r->ticket[index].slot = 0;
    whp_dma_desc_write_word(d, 0, eor(index));
    whp_dma_desc_write_word(d, 1, 0);
    whp_dma_desc_write_word(d, 2, 0);
    whp_dma_desc_write_word(d, 3, 0);
    --r->pending;
    r->consumer = (index + 1u) % WHP_DMA_SLOTS;
    return WHP_DMA_OK;
}

enum whp_dma_result whp_dma_unbind(struct whp_dma_ring *r)
{
    if (!r || !r->descs || !r->mp)
        return WHP_DMA_BAD_ARG;
    if (r->mp->state != WHP_MP_HALTING &&
        r->mp->state != WHP_MP_HALTED &&
        r->mp->state != WHP_MP_RESETTING)
        return WHP_DMA_BAD_STATE;
    if (r->pending || r->mp->tx_pending || r->mp->rx_pending)
        return WHP_DMA_BUSY;
    /* Releasing physical allocations remains the verified NDIS layer's
     * responsibility, and only after hardware bus mastering has stopped. */
    r->descs = 0;
    r->mp = 0;
    return WHP_DMA_OK;
}
