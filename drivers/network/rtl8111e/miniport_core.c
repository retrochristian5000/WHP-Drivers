#include "miniport_core.h"

void whp_rtl8111e_mp_construct(struct whp_rtl8111e_mp *mp)
{
    unsigned int i;
    if (!mp)
        return;
    mp->state = WHP_MP_HALTED;
    mp->revision = WHP_RTL8111E_NO_MATCH;
    mp->last_serial = 0;
    mp->tx_pending = 0;
    mp->rx_pending = 0;
    for (i = 0; i < WHP_RTL8111E_MP_SLOTS; ++i) {
        mp->tx[i] = 0;
        mp->rx[i] = 0;
    }
}

enum whp_rtl8111e_mp_result whp_rtl8111e_mp_begin_init(
    struct whp_rtl8111e_mp *mp, const struct whp_rtl8111e_pci_id *pci,
    uint32_t txconfig, const uint8_t mac[6])
{
    enum whp_rtl8111e_identity revision;
    if (!mp)
        return WHP_MP_BAD_STATE;
    if (mp->state != WHP_MP_HALTED || mp->tx_pending || mp->rx_pending)
        return WHP_MP_BAD_STATE;
    revision = whp_rtl8111e_identify_txconfig(pci, txconfig);
    if (revision != WHP_RTL8111E_E1_CANDIDATE &&
        revision != WHP_RTL8111E_E2_CANDIDATE &&
        revision != WHP_RTL8111E_EVL_CANDIDATE)
        return WHP_MP_UNSUPPORTED;
    if (!whp_rtl8111e_mac_valid(mac))
        return WHP_MP_UNSUPPORTED;
    mp->revision = revision;
    mp->state = WHP_MP_INITIALIZING;
    return WHP_MP_OK;
}

enum whp_rtl8111e_mp_result whp_rtl8111e_mp_finish_init(
    struct whp_rtl8111e_mp *mp, int succeeded)
{
    if (!mp || mp->state != WHP_MP_INITIALIZING)
        return WHP_MP_BAD_STATE;
    if (succeeded) {
        mp->state = WHP_MP_RUNNING;
    } else {
        mp->state = WHP_MP_HALTED;
        mp->revision = WHP_RTL8111E_NO_MATCH;
    }
    return WHP_MP_OK;
}

enum whp_rtl8111e_mp_result whp_rtl8111e_mp_begin_reset(
    struct whp_rtl8111e_mp *mp)
{
    if (!mp || mp->state != WHP_MP_RUNNING)
        return WHP_MP_BAD_STATE;
    mp->state = WHP_MP_RESETTING;
    return WHP_MP_OK;
}

enum whp_rtl8111e_mp_result whp_rtl8111e_mp_finish_reset(
    struct whp_rtl8111e_mp *mp, int succeeded)
{
    if (!mp || mp->state != WHP_MP_RESETTING)
        return WHP_MP_BAD_STATE;
    if (mp->tx_pending || mp->rx_pending)
        return WHP_MP_BUSY;
    mp->state = succeeded ? WHP_MP_RUNNING : WHP_MP_HALTED;
    if (!succeeded)
        mp->revision = WHP_RTL8111E_NO_MATCH;
    return WHP_MP_OK;
}

enum whp_rtl8111e_mp_result whp_rtl8111e_mp_begin_halt(
    struct whp_rtl8111e_mp *mp)
{
    if (!mp || (mp->state != WHP_MP_RUNNING &&
                mp->state != WHP_MP_RESETTING &&
                mp->state != WHP_MP_INITIALIZING))
        return WHP_MP_BAD_STATE;
    mp->state = WHP_MP_HALTING;
    return WHP_MP_OK;
}

enum whp_rtl8111e_mp_result whp_rtl8111e_mp_finish_halt(
    struct whp_rtl8111e_mp *mp)
{
    if (!mp || mp->state != WHP_MP_HALTING)
        return WHP_MP_BAD_STATE;
    if (mp->tx_pending || mp->rx_pending)
        return WHP_MP_BUSY;
    mp->state = WHP_MP_HALTED;
    mp->revision = WHP_RTL8111E_NO_MATCH;
    return WHP_MP_OK;
}

static enum whp_rtl8111e_mp_result begin_io(
    struct whp_rtl8111e_mp *mp, uint32_t slots[WHP_RTL8111E_MP_SLOTS],
    uint32_t *pending, struct whp_rtl8111e_mp_ticket *ticket)
{
    unsigned int i;
    if (!mp || mp->state != WHP_MP_RUNNING)
        return WHP_MP_BAD_STATE;
    if (!ticket)
        return WHP_MP_BAD_TOKEN;
    if (*pending >= WHP_RTL8111E_MP_SLOTS)
        return WHP_MP_BUSY;
    /* Never reuse a serial: old completion tickets must not alias new I/O. */
    if (mp->last_serial == UINT32_MAX)
        return WHP_MP_EXHAUSTED;
    for (i = 0; i < WHP_RTL8111E_MP_SLOTS; ++i) {
        if (slots[i] == 0) {
            uint32_t serial = ++mp->last_serial;
            slots[i] = serial;
            ++*pending;
            ticket->slot = i;
            ticket->serial = serial;
            return WHP_MP_OK;
        }
    }
    /* Corrupt/mismatched slot accounting never enables untracked I/O. */
    return WHP_MP_BUSY;
}

static enum whp_rtl8111e_mp_result retire_io(
    struct whp_rtl8111e_mp *mp, uint32_t slots[WHP_RTL8111E_MP_SLOTS],
    uint32_t *pending, struct whp_rtl8111e_mp_ticket ticket)
{
    if (!mp || (mp->state != WHP_MP_RUNNING &&
                mp->state != WHP_MP_RESETTING &&
                mp->state != WHP_MP_HALTING))
        return WHP_MP_BAD_STATE;
    if (ticket.slot >= WHP_RTL8111E_MP_SLOTS || ticket.serial == 0 ||
        slots[ticket.slot] != ticket.serial || *pending == 0)
        return WHP_MP_BAD_TOKEN;
    slots[ticket.slot] = 0;
    --*pending;
    return WHP_MP_OK;
}

enum whp_rtl8111e_mp_result whp_rtl8111e_mp_tx_begin(
    struct whp_rtl8111e_mp *mp, struct whp_rtl8111e_mp_ticket *ticket)
{
    if (!mp)
        return WHP_MP_BAD_STATE;
    return begin_io(mp, mp->tx, &mp->tx_pending, ticket);
}

enum whp_rtl8111e_mp_result whp_rtl8111e_mp_tx_retire(
    struct whp_rtl8111e_mp *mp, struct whp_rtl8111e_mp_ticket ticket)
{
    if (!mp)
        return WHP_MP_BAD_STATE;
    return retire_io(mp, mp->tx, &mp->tx_pending, ticket);
}

enum whp_rtl8111e_mp_result whp_rtl8111e_mp_rx_begin(
    struct whp_rtl8111e_mp *mp, struct whp_rtl8111e_mp_ticket *ticket)
{
    if (!mp)
        return WHP_MP_BAD_STATE;
    return begin_io(mp, mp->rx, &mp->rx_pending, ticket);
}

enum whp_rtl8111e_mp_result whp_rtl8111e_mp_rx_retire(
    struct whp_rtl8111e_mp *mp, struct whp_rtl8111e_mp_ticket ticket)
{
    if (!mp)
        return WHP_MP_BAD_STATE;
    return retire_io(mp, mp->rx, &mp->rx_pending, ticket);
}
