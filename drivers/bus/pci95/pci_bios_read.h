/* Generic read-only PCI BIOS configuration-word transport.
 * No INT calls or bus writes here: DOS and host tests supply a reader.
 */
#ifndef WHP_PCI95_BIOS_READ_H
#define WHP_PCI95_BIOS_READ_H
#include <stddef.h>
#include <stdint.h>

#define WHP_PCI95_BIOS_HEADER_SIZE 64u
#define WHP_PCI95_BIOS_CONFIG_SIZE 256u

/* Return 1 for a successful 16-bit read, 0 for absent function,
 * -1 for a firmware or transport error.
 */
typedef int (*whp_pci95_read_word_fn)(void *context, uint8_t bus,
    uint8_t devfn, uint8_t offset, uint16_t *value);

struct whp_pci95_bios_reader {
    whp_pci95_read_word_fn read_word;
    void *context;
};

enum whp_pci95_bios_result {
    WHP_PCI95_BIOS_OK = 0,
    WHP_PCI95_BIOS_BAD_INPUT,
    WHP_PCI95_BIOS_NO_DEVICE,
    WHP_PCI95_BIOS_READ_FAILED
};

/* Read standard PCI configuration space into an owned byte snapshot.
 * Only 64 or 256 bytes are accepted (within the legacy PCI BIOS limit).
 * Output buffer is never changed on errors. The first word is read once.
 * Callers must never treat a PCIe capability or bus number as proof that
 * Windows 95 enumerated the endpoint.
 */
enum whp_pci95_bios_result whp_pci95_bios_read_config(
    const struct whp_pci95_bios_reader *reader, uint8_t bus,
    uint8_t devfn, uint8_t *bytes, size_t size);
#endif
