#include "pci_bios_read.h"

enum whp_pci95_bios_result whp_pci95_bios_read_config(
    const struct whp_pci95_bios_reader *reader, uint8_t bus,
    uint8_t devfn, uint8_t *bytes, size_t size)
{
    uint8_t snapshot[WHP_PCI95_BIOS_CONFIG_SIZE];
    uint16_t value;
    size_t offset;
    int status;

    if (!reader || !reader->read_word || !bytes ||
        (size != WHP_PCI95_BIOS_HEADER_SIZE &&
         size != WHP_PCI95_BIOS_CONFIG_SIZE))
        return WHP_PCI95_BIOS_BAD_INPUT;

    for (offset = 0; offset < size; offset += 2u) {
        status = reader->read_word(reader->context, bus, devfn,
                                   (uint8_t)offset, &value);
        if (status != 1) {
            if (offset == 0u && status == 0)
                return WHP_PCI95_BIOS_NO_DEVICE;
            return WHP_PCI95_BIOS_READ_FAILED;
        }
        if (offset == 0u && (value == 0xffffu || value == 0u))
            return WHP_PCI95_BIOS_NO_DEVICE;

        snapshot[offset] = (uint8_t)value;
        snapshot[offset + 1u] = (uint8_t)(value >> 8);
    }
    for (offset = 0; offset < size; ++offset)
        bytes[offset] = snapshot[offset];
    return WHP_PCI95_BIOS_OK;
}
