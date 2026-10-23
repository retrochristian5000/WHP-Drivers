/* Pure host simulator: DMA descriptors/OWN modeled in memory, NOT real NIC. */
#include "dma_ring.h"
#include <stdio.h>
#include <stdalign.h>

#define CHECK(x) do { ++checks; if (!(x)) { \
    fprintf(stderr, "DMA test failed line %d: %s\n", __LINE__, #x); \
    return 1; } } while (0)

static int before_ok = 1, before_calls, prepared, released, acquired;
static int afterown, cpucalls;
static int before(void *ctx, uint64_t addr, uint16_t size, int rx)
{
    (void)ctx; (void)addr; (void)size; (void)rx;
    ++before_calls;
    return before_ok;
}
static void prep(void *ctx, struct whp_dma_desc *d)
{
    (void)ctx;
    if (whp_dma_desc_word(d, 0) & WHP_DMA_OWN)
        prepared = -1000;
    else
        ++prepared;
}
static void release(void *ctx, struct whp_dma_desc *d)
{
    (void)ctx;
    if (!(whp_dma_desc_word(d, 0) & WHP_DMA_OWN))
        released = -1000;
    else
        ++released;
}
static void acquire(void *ctx, struct whp_dma_desc *d)
{ (void)ctx; (void)d; ++acquired; }
static void after_own(void *ctx) { (void)ctx; ++afterown; }
static void cpu(void *ctx, uint64_t a, uint16_t len)
{ (void)ctx; (void)a; (void)len; ++cpucalls; }
static const struct whp_dma_ops ops = {
    before, prep, release, acquire, after_own, cpu
};

int main(void)
{
    alignas(256) struct whp_dma_desc tx[WHP_DMA_SLOTS];
    alignas(256) struct whp_dma_desc rx[WHP_DMA_SLOTS];
    struct whp_dma_ring t = {0}, r = {0}, bad = {0};
    struct whp_rtl8111e_mp mp;
    struct whp_rtl8111e_pci_id pci = {
        WHP_RTL8111E_PCI_VENDOR, WHP_RTL8111E_PCI_DEVICE,
        WHP_RTL8111E_PCI_CLASS, WHP_RTL8111E_PCI_SUBCLASS
    };
    const uint8_t mac[6] = {2,1,2,3,4,5};
    unsigned int i, idx;
    uint16_t length;
    uint32_t raw;
    int checks = 0;

    whp_rtl8111e_mp_construct(&mp);
    CHECK(whp_dma_ring_bind(&t, tx, 0x2000, &mp, &ops, 0, 0) ==
          WHP_DMA_BAD_STATE);
    CHECK(whp_rtl8111e_mp_begin_init(&mp, &pci, 0x2c100000u, mac) == WHP_MP_OK);
    CHECK(whp_rtl8111e_mp_finish_init(&mp, 1) == WHP_MP_OK);
    CHECK(whp_dma_ring_bind(&bad, tx, 0x2001, &mp, &ops, 0, 0) ==
          WHP_DMA_BAD_ARG);
    CHECK(whp_dma_ring_bind(&bad, tx, UINT64_C(0x100000000),
                            &mp, &ops, 0, 0) == WHP_DMA_BAD_ARG);
    CHECK(whp_dma_ring_bind(&t, tx, 0x2000, &mp, &ops, 0, 0) == WHP_DMA_OK);
    CHECK(whp_dma_ring_bind(&r, rx, 0x3000, &mp, &ops, 0, 1) == WHP_DMA_OK);
    CHECK(sizeof(tx[0]) == 16);
    CHECK(whp_dma_desc_word(&tx[7], 0) == WHP_DMA_EOR);
    CHECK(whp_dma_submit(&t, 0, 60, &idx) == WHP_DMA_BAD_ARG);
    CHECK(whp_dma_submit(&t, 0xffffffe0u, 64, &idx) == WHP_DMA_BAD_ARG);
    CHECK(whp_dma_submit(&t, 0x4000, 0, &idx) == WHP_DMA_BAD_ARG);
    CHECK(whp_dma_submit(&t, 0x4000, 59, &idx) == WHP_DMA_BAD_ARG);
    CHECK(whp_dma_submit(&r, 0xc000, 63, &idx) == WHP_DMA_BAD_ARG);
    CHECK(whp_dma_submit(&t, 0x4000, 2049, &idx) == WHP_DMA_BAD_ARG);
    before_ok = 0;
    CHECK(whp_dma_submit(&t, 0x4000, 100, &idx) == WHP_DMA_SYNC_FAILED);
    CHECK(mp.tx_pending == 0 && t.pending == 0 && released == 0);
    before_ok = 1;
    for (i = 0; i < WHP_DMA_SLOTS; ++i) {
        CHECK(whp_dma_submit(&t, 0x4000 + i * 2048, 100, &idx) ==
              WHP_DMA_OK);
        CHECK(idx == i);
        CHECK((whp_dma_desc_word(&tx[i], 0) & WHP_DMA_OWN) != 0);
        CHECK(whp_dma_desc_word(&tx[i], 2) == 0x4000u + i * 2048u);
        CHECK(whp_dma_desc_word(&tx[i], 3) == 0);
    }
    CHECK((whp_dma_desc_word(&tx[7], 0) & WHP_DMA_EOR) != 0);
    CHECK(whp_dma_submit(&t, 0x6000, 100, &idx) == WHP_DMA_BUSY);
    CHECK(whp_dma_unbind(&t) == WHP_DMA_BAD_STATE);
    CHECK(whp_dma_complete(&t, &idx, &length, &raw) == WHP_DMA_BUSY);
    for (i = 0; i < WHP_DMA_SLOTS; ++i) {
        whp_dma_desc_write_word(&tx[i], 0,
             whp_dma_desc_word(&tx[i], 0) & ~WHP_DMA_OWN);
        CHECK(whp_dma_complete(&t, &idx, &length, &raw) == WHP_DMA_OK);
        CHECK(idx == i && length == 100);
        CHECK(whp_dma_complete(&t, &idx, &length, &raw) == WHP_DMA_BAD_STATE);
        CHECK(whp_dma_ack(&t, i) == WHP_DMA_OK);
        CHECK(whp_dma_ack(&t, i) == WHP_DMA_BAD_STATE);
    }
    CHECK(t.pending == 0 && mp.tx_pending == 0);
    CHECK(prepared == released && prepared == 8 &&
          acquired >= 8 && afterown == 8);
    CHECK(whp_dma_submit(&t, 0xa000, 60, &idx) == WHP_DMA_OK && idx == 0);
    whp_dma_desc_write_word(&tx[0], 2, 0xb000);
    whp_dma_desc_write_word(&tx[0], 0,
        whp_dma_desc_word(&tx[0], 0) & ~WHP_DMA_OWN);
    CHECK(whp_dma_complete(&t, &idx, &length, &raw) == WHP_DMA_BAD_STATE);
    CHECK(t.pending == 1);
    whp_dma_desc_write_word(&tx[0], 2, 0xa000);
    CHECK(whp_dma_complete(&t, &idx, &length, &raw) == WHP_DMA_OK);
    CHECK(whp_dma_ack(&t, 0) == WHP_DMA_OK);

    CHECK(whp_dma_submit(&r, 0xc000, 2048, &idx) == WHP_DMA_OK);
    CHECK((whp_dma_desc_word(&rx[0], 0) & WHP_DMA_FS) == 0);
    CHECK(whp_dma_complete(&r, &idx, &length, &raw) == WHP_DMA_BUSY);
    whp_dma_desc_write_word(&rx[0], 0, WHP_DMA_FS | WHP_DMA_LS | 104u);
    CHECK(whp_dma_complete(&r, &idx, &length, &raw) == WHP_DMA_OK);
    CHECK(length == 100 && cpucalls == 1 && mp.rx_pending == 1);
    CHECK(whp_dma_unbind(&r) == WHP_DMA_BAD_STATE);
    CHECK(whp_dma_ack(&r, 0) == WHP_DMA_OK);
    CHECK(mp.rx_pending == 0);
    CHECK(whp_dma_submit(&r, 0xd000, 2048, &idx) == WHP_DMA_OK);
    whp_dma_desc_write_word(&rx[1], 0,
        WHP_DMA_RX_ERROR | WHP_DMA_FS | WHP_DMA_LS | 104u);
    CHECK(whp_dma_complete(&r, &idx, &length, &raw) ==
          WHP_DMA_DROPPED && length == 0);
    CHECK(cpucalls == 1);
    CHECK(whp_dma_ack(&r, 1) == WHP_DMA_OK);
    CHECK(whp_dma_submit(&r, 0xe000, 2048, &idx) == WHP_DMA_OK);
    whp_dma_desc_write_word(&rx[2], 0, WHP_DMA_FS | 1u);
    CHECK(whp_dma_complete(&r, &idx, &length, &raw) == WHP_DMA_DROPPED);
    CHECK(whp_dma_ack(&r, 2) == WHP_DMA_OK);
    CHECK(whp_dma_submit(&t, 0x12000, 64, &idx) == WHP_DMA_OK);
    CHECK(whp_rtl8111e_mp_begin_reset(&mp) == WHP_MP_OK);
    CHECK(whp_dma_submit(&t, 0x6000, 100, &idx) == WHP_DMA_BAD_STATE);
    CHECK(whp_rtl8111e_mp_finish_reset(&mp, 1) == WHP_MP_BUSY);
    CHECK(whp_dma_unbind(&t) == WHP_DMA_BUSY);
    CHECK(whp_dma_complete(&t, &idx, &length, &raw) == WHP_DMA_BUSY);
    whp_dma_desc_write_word(&tx[1], 0,
        whp_dma_desc_word(&tx[1], 0) & ~WHP_DMA_OWN);
    CHECK(whp_dma_complete(&t, &idx, &length, &raw) == WHP_DMA_OK);
    CHECK(whp_rtl8111e_mp_finish_reset(&mp, 1) == WHP_MP_BUSY);
    CHECK(whp_dma_ack(&t, 1) == WHP_DMA_OK);
    CHECK(mp.tx_pending == 0 && t.pending == 0);
    CHECK(whp_rtl8111e_mp_finish_reset(&mp, 1) == WHP_MP_OK);
    CHECK(whp_rtl8111e_mp_begin_halt(&mp) == WHP_MP_OK);
    CHECK(whp_dma_unbind(&t) == WHP_DMA_OK);
    CHECK(whp_dma_unbind(&r) == WHP_DMA_OK);
    CHECK(whp_rtl8111e_mp_finish_halt(&mp) == WHP_MP_OK);
    printf("RTL8111E DMA descriptor tests: %d checks passed\n", checks);
    return 0;
}
