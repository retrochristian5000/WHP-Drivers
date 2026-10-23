/* Deterministic no-hardware lifecycle regression: never calls NDIS or PCI. */
#include "miniport_core.h"
#include <stdio.h>
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "FAIL line %d: %s\n",__LINE__,#x); return 1; } } while(0)
int main(void)
{
    struct whp_rtl8111e_mp mp;
    struct whp_rtl8111e_pci_id id = {WHP_RTL8111E_PCI_VENDOR,WHP_RTL8111E_PCI_DEVICE,2,0};
    struct whp_rtl8111e_mp_ticket tx[WHP_RTL8111E_MP_SLOTS],rx[WHP_RTL8111E_MP_SLOTS],stale, next, bad;
    uint8_t mac[6]={2,1,2,3,4,5};
    uint8_t zero[6]={0,0,0,0,0,0};
    unsigned int i;
    int checks=0;
    whp_rtl8111e_mp_construct(&mp);
    CHECK(mp.state==WHP_MP_HALTED);
    CHECK(whp_rtl8111e_mp_tx_begin(&mp,&next)==WHP_MP_BAD_STATE);
    CHECK(whp_rtl8111e_mp_begin_reset(&mp)==WHP_MP_BAD_STATE);
    CHECK(whp_rtl8111e_mp_finish_reset(&mp,1)==WHP_MP_BAD_STATE);
    CHECK(whp_rtl8111e_mp_begin_init(&mp,&id,0x48000000u,mac)==WHP_MP_UNSUPPORTED);
    CHECK(whp_rtl8111e_mp_begin_init(&mp,&id,0x2c100000u,zero)==WHP_MP_UNSUPPORTED);
    id.vendor=0x1022;
    CHECK(whp_rtl8111e_mp_begin_init(&mp,&id,0x2c100000u,mac)==WHP_MP_UNSUPPORTED);
    id.vendor=WHP_RTL8111E_PCI_VENDOR;
    CHECK(whp_rtl8111e_mp_begin_init(&mp,&id,0x2c100000u,mac)==WHP_MP_OK);
    CHECK(mp.state==WHP_MP_INITIALIZING && mp.revision==WHP_RTL8111E_E1_CANDIDATE);
    CHECK(whp_rtl8111e_mp_begin_init(&mp,&id,0x2c100000u,mac)==WHP_MP_BAD_STATE);
    CHECK(whp_rtl8111e_mp_rx_begin(&mp,&next)==WHP_MP_BAD_STATE);
    CHECK(whp_rtl8111e_mp_finish_init(&mp,0)==WHP_MP_OK);
    CHECK(mp.state==WHP_MP_HALTED && mp.revision==WHP_RTL8111E_NO_MATCH);
    CHECK(whp_rtl8111e_mp_begin_init(&mp,&id,0x2c000000u,mac)==WHP_MP_OK);
    CHECK(mp.revision==WHP_RTL8111E_E2_CANDIDATE);
    CHECK(whp_rtl8111e_mp_finish_init(&mp,1)==WHP_MP_OK);
    CHECK(mp.state==WHP_MP_RUNNING);
    for(i=0;i<WHP_RTL8111E_MP_SLOTS;i++){
        CHECK(whp_rtl8111e_mp_tx_begin(&mp,&tx[i])==WHP_MP_OK);
        CHECK(whp_rtl8111e_mp_rx_begin(&mp,&rx[i])==WHP_MP_OK);
    }
    CHECK(mp.tx_pending==WHP_RTL8111E_MP_SLOTS);
    CHECK(mp.rx_pending==WHP_RTL8111E_MP_SLOTS);
    next.slot=100;next.serial=99;
    CHECK(whp_rtl8111e_mp_tx_begin(&mp,&next)==WHP_MP_BUSY);
    CHECK(next.slot==100 && next.serial==99);
    CHECK(whp_rtl8111e_mp_rx_begin(&mp,&next)==WHP_MP_BUSY);
    CHECK(whp_rtl8111e_mp_tx_begin(&mp,0)==WHP_MP_BAD_TOKEN);
    CHECK(whp_rtl8111e_mp_tx_retire(&mp,rx[0])==WHP_MP_BAD_TOKEN);
    CHECK(whp_rtl8111e_mp_rx_retire(&mp,tx[0])==WHP_MP_BAD_TOKEN);
    CHECK(whp_rtl8111e_mp_begin_reset(&mp)==WHP_MP_OK);
    CHECK(mp.state==WHP_MP_RESETTING);
    CHECK(whp_rtl8111e_mp_begin_reset(&mp)==WHP_MP_BAD_STATE);
    CHECK(whp_rtl8111e_mp_tx_begin(&mp,&next)==WHP_MP_BAD_STATE);
    CHECK(whp_rtl8111e_mp_rx_begin(&mp,&next)==WHP_MP_BAD_STATE);
    CHECK(whp_rtl8111e_mp_finish_reset(&mp,1)==WHP_MP_BUSY);
    stale=tx[0];
    CHECK(whp_rtl8111e_mp_tx_retire(&mp,tx[0])==WHP_MP_OK);
    CHECK(whp_rtl8111e_mp_tx_retire(&mp,tx[0])==WHP_MP_BAD_TOKEN);
    bad=tx[1];bad.slot=9;
    CHECK(whp_rtl8111e_mp_tx_retire(&mp,bad)==WHP_MP_BAD_TOKEN);
    bad=tx[1];bad.serial++;
    CHECK(whp_rtl8111e_mp_tx_retire(&mp,bad)==WHP_MP_BAD_TOKEN);
    CHECK(mp.tx_pending==WHP_RTL8111E_MP_SLOTS-1);
    for(i=1;i<WHP_RTL8111E_MP_SLOTS;i++) CHECK(whp_rtl8111e_mp_tx_retire(&mp,tx[i])==WHP_MP_OK);
    CHECK(whp_rtl8111e_mp_finish_reset(&mp,1)==WHP_MP_BUSY);
    for(i=0;i<WHP_RTL8111E_MP_SLOTS;i++) CHECK(whp_rtl8111e_mp_rx_retire(&mp,rx[i])==WHP_MP_OK);
    CHECK(whp_rtl8111e_mp_finish_reset(&mp,1)==WHP_MP_OK);
    CHECK(mp.state==WHP_MP_RUNNING && mp.tx_pending==0 && mp.rx_pending==0);
    CHECK(whp_rtl8111e_mp_tx_begin(&mp,&next)==WHP_MP_OK);
    CHECK(next.serial!=stale.serial);
    CHECK(whp_rtl8111e_mp_tx_retire(&mp,stale)==WHP_MP_BAD_TOKEN);
    CHECK(whp_rtl8111e_mp_begin_halt(&mp)==WHP_MP_OK);
    CHECK(mp.state==WHP_MP_HALTING);
    CHECK(whp_rtl8111e_mp_tx_begin(&mp,&bad)==WHP_MP_BAD_STATE);
    CHECK(whp_rtl8111e_mp_finish_halt(&mp)==WHP_MP_BUSY);
    CHECK(whp_rtl8111e_mp_tx_retire(&mp,next)==WHP_MP_OK);
    CHECK(whp_rtl8111e_mp_finish_halt(&mp)==WHP_MP_OK);
    CHECK(mp.state==WHP_MP_HALTED);
    CHECK(whp_rtl8111e_mp_tx_retire(&mp,next)==WHP_MP_BAD_STATE);
    CHECK(whp_rtl8111e_mp_begin_init(&mp,&id,0x2c800000u,mac)==WHP_MP_OK);
    CHECK(mp.revision==WHP_RTL8111E_EVL_CANDIDATE);
    CHECK(whp_rtl8111e_mp_finish_init(&mp,1)==WHP_MP_OK);
    CHECK(whp_rtl8111e_mp_begin_reset(&mp)==WHP_MP_OK);
    CHECK(whp_rtl8111e_mp_finish_reset(&mp,0)==WHP_MP_OK);
    CHECK(mp.state==WHP_MP_HALTED && mp.revision==WHP_RTL8111E_NO_MATCH);
    CHECK(whp_rtl8111e_mp_begin_init(&mp,&id,0x2c100000u,mac)==WHP_MP_OK);
    CHECK(whp_rtl8111e_mp_begin_halt(&mp)==WHP_MP_OK);
    CHECK(whp_rtl8111e_mp_finish_halt(&mp)==WHP_MP_OK);
    CHECK(whp_rtl8111e_mp_begin_init(&mp,&id,0x2c100000u,mac)==WHP_MP_OK);
    CHECK(whp_rtl8111e_mp_finish_init(&mp,1)==WHP_MP_OK);
    mp.last_serial=UINT32_MAX;
    CHECK(whp_rtl8111e_mp_tx_begin(&mp,&next)==WHP_MP_EXHAUSTED);
    CHECK(mp.tx_pending==0 && mp.rx_pending==0);
    CHECK(whp_rtl8111e_mp_begin_halt(&mp)==WHP_MP_OK);
    CHECK(whp_rtl8111e_mp_finish_halt(&mp)==WHP_MP_OK);
    printf("RTL8111E miniport lifecycle: %d checks passed\n", checks);
    return 0;
}
