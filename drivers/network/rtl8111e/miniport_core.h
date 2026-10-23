/* RTL8111E miniport lifecycle and outstanding-I/O bookkeeping.
 * Portable policy core, NOT an NDIS miniport, DMA engine or Win95 .VXD.
 * All callers MUST serialize access using the verified platform's lock.
 * A send or receive ticket must be retired after its upper-layer completion;
 * reset and halt are blocked until all outstanding tickets are retired.
 */
#ifndef WHP_RTL8111E_MINIPORT_CORE_H
#define WHP_RTL8111E_MINIPORT_CORE_H

#include <stdint.h>
#include "rtl8111e.h"

#define WHP_RTL8111E_MP_SLOTS 8u

enum whp_rtl8111e_mp_state {
    WHP_MP_HALTED = 0,
    WHP_MP_INITIALIZING,
    WHP_MP_RUNNING,
    WHP_MP_RESETTING,
    WHP_MP_HALTING
};

enum whp_rtl8111e_mp_result {
    WHP_MP_OK = 0,
    WHP_MP_BAD_STATE,
    WHP_MP_UNSUPPORTED,
    WHP_MP_BAD_TOKEN,
    WHP_MP_BUSY,
    WHP_MP_EXHAUSTED
};

struct whp_rtl8111e_mp_ticket {
    uint32_t slot;
    uint32_t serial;
};

struct whp_rtl8111e_mp {
    enum whp_rtl8111e_mp_state state;
    enum whp_rtl8111e_identity revision;
    uint32_t last_serial;
    uint32_t tx[WHP_RTL8111E_MP_SLOTS];
    uint32_t rx[WHP_RTL8111E_MP_SLOTS];
    uint32_t tx_pending;
    uint32_t rx_pending;
};

/* Construct ONCE, before any callback can access this context.
 * Never reconstruct a live instance to bypass the outstanding-I/O gate.
 */
void whp_rtl8111e_mp_construct(struct whp_rtl8111e_mp *mp);

enum whp_rtl8111e_mp_result whp_rtl8111e_mp_begin_init(
    struct whp_rtl8111e_mp *mp, const struct whp_rtl8111e_pci_id *pci,
    uint32_t txconfig, const uint8_t mac[6]);
enum whp_rtl8111e_mp_result whp_rtl8111e_mp_finish_init(
    struct whp_rtl8111e_mp *mp, int succeeded);
enum whp_rtl8111e_mp_result whp_rtl8111e_mp_begin_reset(
    struct whp_rtl8111e_mp *mp);
enum whp_rtl8111e_mp_result whp_rtl8111e_mp_finish_reset(
    struct whp_rtl8111e_mp *mp, int succeeded);
enum whp_rtl8111e_mp_result whp_rtl8111e_mp_begin_halt(
    struct whp_rtl8111e_mp *mp);
enum whp_rtl8111e_mp_result whp_rtl8111e_mp_finish_halt(
    struct whp_rtl8111e_mp *mp);

/* These are *bookkeeping tickets*, not device descriptors or NDIS packets.
 * The port layer owns actual DMA memory, NDIS status completion, interrupts,
 * and packet buffers. This core does not release any external resources.
 */
enum whp_rtl8111e_mp_result whp_rtl8111e_mp_tx_begin(
    struct whp_rtl8111e_mp *mp, struct whp_rtl8111e_mp_ticket *ticket);
enum whp_rtl8111e_mp_result whp_rtl8111e_mp_tx_retire(
    struct whp_rtl8111e_mp *mp, struct whp_rtl8111e_mp_ticket ticket);
enum whp_rtl8111e_mp_result whp_rtl8111e_mp_rx_begin(
    struct whp_rtl8111e_mp *mp, struct whp_rtl8111e_mp_ticket *ticket);
enum whp_rtl8111e_mp_result whp_rtl8111e_mp_rx_retire(
    struct whp_rtl8111e_mp *mp, struct whp_rtl8111e_mp_ticket ticket);

#endif
