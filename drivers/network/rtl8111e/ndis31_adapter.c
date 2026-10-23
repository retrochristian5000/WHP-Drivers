/* RTL8111E Windows 95 RTM / NT 3.51 NDIS miniport contract probe.
 * Compile ONLY with independently verified historical NDIS headers.
 * Not a DriverEntry, driver binary, DMA implementation or NIC initializer.
 * Every data-plane callback fails closed; hardware is never accessed.
 */
#ifndef WHP_RTL8111E_NDIS31_PROBE_ONLY
#error "RTL8111E NDIS adapter is a compile-only research probe"
#endif
#ifndef WHP_WIN95_NDIS31_USE_HISTORICAL_DDK
#error "Use the pinned Water NDIS boundary and verified NT 3.51 DDK"
#endif
#ifndef NDIS_MINIPORT_DRIVER
#define NDIS_MINIPORT_DRIVER 1
#endif
#include <ddk/win95rtm/network/ndis31.h>

/* Use the exact DDK typedefs for callback signatures and calling conventions.
 * No locally reconstructed NDIS structures, NT imports or Win9x VxD APIs.
 */
static NDIS_STATUS NTAPI whp_initialize(
    PNDIS_STATUS open_error, PUINT selected_medium,
    PNDIS_MEDIUM media, UINT media_count,
    NDIS_HANDLE adapter, NDIS_HANDLE configuration)
{
    (void)selected_medium;
    (void)media;
    (void)media_count;
    (void)adapter;
    (void)configuration;
    if (open_error)
        *open_error = NDIS_STATUS_FAILURE;
    /* No PCI resource/PnP/DMA proof: NEVER signal a usable adapter. */
    return NDIS_STATUS_FAILURE;
}

static VOID NTAPI whp_halt(NDIS_HANDLE adapter)
{
    (void)adapter; /* No resources have ever been acquired. */
}

static NDIS_STATUS NTAPI whp_reset(PBOOLEAN addressing_reset,
                                   NDIS_HANDLE adapter)
{
    (void)adapter;
    if (addressing_reset)
        *addressing_reset = FALSE;
    return NDIS_STATUS_FAILURE;
}

static NDIS_STATUS NTAPI whp_query(NDIS_HANDLE adapter, NDIS_OID oid,
    PVOID data, ULONG length, PULONG written, PULONG needed)
{
    (void)adapter;
    (void)oid;
    (void)data;
    (void)length;
    if (written)
        *written = 0;
    if (needed)
        *needed = 0;
    return NDIS_STATUS_FAILURE;
}

static NDIS_STATUS NTAPI whp_set(NDIS_HANDLE adapter, NDIS_OID oid,
    PVOID data, ULONG length, PULONG read, PULONG needed)
{
    (void)adapter;
    (void)oid;
    (void)data;
    (void)length;
    if (read)
        *read = 0;
    if (needed)
        *needed = 0;
    return NDIS_STATUS_FAILURE;
}

static NDIS_STATUS NTAPI whp_send(NDIS_HANDLE adapter, PNDIS_PACKET packet,
                                  UINT flags)
{
    (void)adapter;
    (void)packet;
    (void)flags;
    return NDIS_STATUS_FAILURE;
}

static NDIS_STATUS NTAPI whp_transfer(PNDIS_PACKET packet, PUINT transferred,
    NDIS_HANDLE adapter, NDIS_HANDLE receive_context,
    UINT offset, UINT length)
{
    (void)packet;
    (void)adapter;
    (void)receive_context;
    (void)offset;
    (void)length;
    if (transferred)
        *transferred = 0;
    return NDIS_STATUS_FAILURE;
}

/* The probe is intentionally NOT named DriverEntry or exported as one.
 * Only a trusted DDK-backed test harness should call this function.
 * A successful registration is not permission to initialize the NIC.
 */
NDIS_STATUS whp_rtl8111e_ndis31_registration_probe(PVOID driver_object,
                                                   PVOID registry_path)
{
    NDIS_MINIPORT_CHARACTERISTICS chars;
    NDIS_HANDLE wrapper = (NDIS_HANDLE)0;
    NDIS_STATUS status;
    unsigned char *bytes = (unsigned char *)&chars;
    UINT i;

    for (i = 0; i < (UINT)sizeof(chars); ++i)
        bytes[i] = 0;

    /* The NT 3.51 miniport DDK uses NDIS 3.0 callback fields; Windows 95
     * NDIS 3.1 adds Plug and Play in its NDIS wrapper. Not NDIS 4/5/6.
     */
    chars.MajorNdisVersion = 3;
    chars.MinorNdisVersion = 0;
    chars.InitializeHandler = whp_initialize;
    chars.HaltHandler = whp_halt;
    chars.ResetHandler = whp_reset;
    chars.QueryInformationHandler = whp_query;
    chars.SetInformationHandler = whp_set;
    chars.SendHandler = whp_send;
    chars.TransferDataHandler = whp_transfer;
    /* No ISR/interrupt handler without hardware resource ownership. */

    NdisMInitializeWrapper(&wrapper, driver_object, registry_path, (PVOID)0);
    if (!wrapper)
        return NDIS_STATUS_FAILURE;

    status = NdisMRegisterMiniport(wrapper, &chars, (UINT)sizeof(chars));
    if (status != NDIS_STATUS_SUCCESS)
        NdisTerminateWrapper(wrapper, (PVOID)0);
    return status;
}
