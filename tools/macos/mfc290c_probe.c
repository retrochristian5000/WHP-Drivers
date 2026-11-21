/* Brother MFC-290C: read-only macOS IORegistry USB inventory.
 * This is a diagnostic, not a USB driver. No USB requests or device opens.
 */
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/sysctl.h>

#include "mfc290c.h"
#include "mfc290c_location.h"
#include "mfc290c_cups.h"
#include "mfc290c_usb_commands.h"

/* Process ISA and translation never describe the USB peripheral firmware. */
static void report_process_architecture(void)
{
    const char *isa;
    int translated = 0;
    size_t translated_size = sizeof(translated);

#if defined(__arm64e__)
    isa = "arm64e";
#elif defined(__arm64__) || defined(__aarch64__)
    isa = "arm64";
#elif defined(__x86_64__)
    isa = "x86_64";
#elif defined(__i386__)
    isa = "i386";
#elif defined(__ppc64__)
    isa = "powerpc64";
#elif defined(__ppc__)
    isa = "powerpc";
#else
    isa = "unknown";
#endif
    printf("processISA=%s pointerBits=%zu", isa, sizeof(void *) * 8u);
    if (sysctlbyname("sysctl.proc_translated", &translated,
                     &translated_size, NULL, 0) == 0 &&
        translated_size == sizeof(translated) &&
        (translated == 0 || translated == 1)) {
        printf(" rosetta=%s", translated == 1 ? "yes" : "no");
    } else {
        printf(" rosetta=unknown");
    }
    putchar('\n');
    puts("Process architecture does not establish device or driver support.");
}

#define MAX_REGISTRY_DEPTH 8u
#define MAX_REGISTRY_NODES 512u

/* IORegistry numeric values are CFNumbers; reject unexpected types/ranges. */
static int read_u32(io_registry_entry_t entry, CFStringRef key, uint32_t *out)
{
    CFTypeRef value;
    int64_t number;
    int ok = 0;

    value = IORegistryEntryCreateCFProperty(entry, key, kCFAllocatorDefault, 0);
    if (value == NULL) {
        return 0;
    }
    if (CFGetTypeID(value) == CFNumberGetTypeID() &&
        CFNumberGetValue((CFNumberRef)value, kCFNumberSInt64Type, &number) &&
        number >= 0 && (uint64_t)number <= UINT32_MAX) {
        *out = (uint32_t)number;
        ok = 1;
    }
    CFRelease(value);
    return ok;
}

static int read_u16(io_registry_entry_t entry, CFStringRef key, uint16_t *out)
{
    uint32_t value;

    if (!read_u32(entry, key, &value) || value > UINT16_MAX) {
        return 0;
    }
    *out = (uint16_t)value;
    return 1;
}

/* Optional output avoids re-reading class/subclass/protocol properties. */
static int print_interface_field(io_registry_entry_t entry,
                                 CFStringRef key, const char *label,
                                 uint8_t *observed)
{
    uint32_t value;

    if (read_u32(entry, key, &value) && value <= UINT8_MAX) {
        printf(" %s=0x%02x", label, (unsigned)value);
        if (observed != NULL) {
            *observed = (uint8_t)value;
        }
        return 1;
    }
    printf(" %s=unknown", label);
    return 0;
}

static void print_interface_protocol_hint(int have_triplet, uint8_t cls,
                                          uint8_t subclass, uint8_t protocol)
{
    enum whp_mfc290c_usb_interface_hint hint;
    const char *meaning = "unclassified";

    if (!have_triplet) {
        printf(" protocolMeaning=insufficient");
        return;
    }
    hint = whp_mfc290c_usb_hint_interface(cls, subclass, protocol);
    switch (hint) {
    case WHP_MFC290C_INTERFACE_PRINTER_UNIDIRECTIONAL:
        meaning = "printer-unidirectional";
        break;
    case WHP_MFC290C_INTERFACE_PRINTER_BIDIRECTIONAL:
        meaning = "printer-bidirectional";
        break;
    case WHP_MFC290C_INTERFACE_PRINTER_IEEE1284_4:
        meaning = "printer-1284.4";
        break;
    case WHP_MFC290C_INTERFACE_PRINTER_VENDOR_PROTOCOL:
        meaning = "printer-vendor-protocol";
        break;
    case WHP_MFC290C_INTERFACE_VENDOR_CLASS:
        meaning = "vendor-class";
        break;
    case WHP_MFC290C_INTERFACE_STORAGE_SCSI_BULK_ONLY:
        meaning = "storage-scsi-bulk-only";
        break;
    case WHP_MFC290C_INTERFACE_UNCLASSIFIED:
        break;
    }
    printf(" protocolMeaning=%s", meaning);
}

/* The USB device registry can expose a serial string already obtained by
 * macOS. Observe its presence only: never print or persist its contents.
 * A missing/wrong-type property is "unavailable", not proof of no serial.
 * The property name is Apple's kUSBSerialNumberString registry key.
 */
static void print_serial_registry_hint(io_registry_entry_t device)
{
    CFTypeRef value = IORegistryEntryCreateCFProperty(
        device, CFSTR("USB Serial Number"), kCFAllocatorDefault, 0);

    if (value != NULL &&
        CFGetTypeID(value) == CFStringGetTypeID() &&
        CFStringGetLength((CFStringRef)value) > 0) {
        printf(" serialRegistry=present");
    } else {
        printf(" serialRegistry=unavailable");
    }
    if (value != NULL) {
        CFRelease(value);
    }
}

/* Registry class and bundle strings are diagnostic hints, not proof of a
 * functional Brother driver or an endpoint claim. Never print arbitrary
 * property values, queue names, serial numbers, or device URIs.
 */
static void print_bundle_hint(io_registry_entry_t entry)
{
    CFTypeRef value = IORegistryEntryCreateCFProperty(
        entry, CFSTR("CFBundleIdentifier"), kCFAllocatorDefault, 0);
    char bundle[128];
    size_t i;

    if (value == NULL) {
        return;
    }
    if (CFGetTypeID(value) == CFStringGetTypeID() &&
        CFStringGetCString((CFStringRef)value, bundle,
                           sizeof(bundle), kCFStringEncodingUTF8)) {
        for (i = 0u; bundle[i] != '\0'; ++i) {
            unsigned char c = (unsigned char)bundle[i];
            if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                  (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '_')) {
                bundle[i] = '_';
            }
        }
        printf(" bundleHint=%s", bundle);
    }
    CFRelease(value);
}

/* Apple's published legacy CUPS backend reads "USB Printing Class" on
 * its interface service when locating a class plugin. Checking for this
 * property is passive; it neither loads the plugin nor opens USB pipes.
 * On newer macOS versions absence is not evidence of driver failure.
 * Do not leak the path: it may reveal installed software details.
 */
static void print_legacy_cups_class_hint(io_registry_entry_t interface)
{
    CFTypeRef value = IORegistryEntryCreateCFProperty(
        interface, CFSTR("USB Printing Class"), kCFAllocatorDefault, 0);

    if (value != NULL &&
        CFGetTypeID(value) == CFStringGetTypeID() &&
        CFStringGetLength((CFStringRef)value) > 0) {
        printf(" legacyCupsPlugin=present");
    } else {
        printf(" legacyCupsPlugin=unavailable");
    }
    if (value != NULL) {
        CFRelease(value);
    }
}

/* The immediate client services are only provider-tree clues. There may be
 * multiple matching clients and the actual print/scan processes live above
 * this registry layer. Limit diagnostics to eight children per interface.
 */
static void print_service_hints(io_registry_entry_t interface)
{
    io_iterator_t children = 0;
    io_registry_entry_t child;
    unsigned int printed = 0u;

    if (IORegistryEntryGetChildIterator(interface, kIOServicePlane,
                                        &children) != KERN_SUCCESS) {
        return;
    }
    while (printed < 8u && (child = IOIteratorNext(children)) != 0) {
        io_name_t class_name;
        if (IOObjectGetClass(child, class_name) == KERN_SUCCESS) {
            printf("    childServiceClass=%s", class_name);
            print_bundle_hint(child);
            putchar('\n');
        }
        ++printed;
        IOObjectRelease(child);
    }
    IOObjectRelease(children);
}

/* Interface services may not be exposed as immediate children on every
 * release. Bound the traversal; unavailable metadata is not an empty device.
 */
static void print_interfaces(io_registry_entry_t entry, unsigned depth,
                             unsigned *remaining, unsigned *found)
{
    io_iterator_t iterator = 0;
    io_registry_entry_t child;
    kern_return_t status;

    if (depth >= MAX_REGISTRY_DEPTH || *remaining == 0u) {
        return;
    }
    status = IORegistryEntryGetChildIterator(entry, kIOServicePlane, &iterator);
    if (status != KERN_SUCCESS) {
        return;
    }
    while (*remaining != 0u && (child = IOIteratorNext(iterator)) != 0) {
        --*remaining;
        if (IOObjectConformsTo(child, "IOUSBHostInterface") ||
            IOObjectConformsTo(child, "IOUSBInterface")) {
            uint8_t cls = 0u;
            uint8_t subclass = 0u;
            uint8_t protocol = 0u;
            int have_class;
            int have_subclass;
            int have_protocol;

            ++*found;
            printf("  interface");
            print_interface_field(child, CFSTR("bInterfaceNumber"), "number", NULL);
            /* Registry hints only: neither setting nor endpoint is changed. */
            print_interface_field(child, CFSTR("bAlternateSetting"), "alternate", NULL);
            print_interface_field(child, CFSTR("bNumEndpoints"), "declaredEndpoints", NULL);
            have_class = print_interface_field(child, CFSTR("bInterfaceClass"),
                                               "class", &cls);
            have_subclass = print_interface_field(child, CFSTR("bInterfaceSubClass"),
                                                  "subclass", &subclass);
            have_protocol = print_interface_field(child, CFSTR("bInterfaceProtocol"),
                                                  "protocol", &protocol);
            print_interface_protocol_hint(have_class && have_subclass &&
                                          have_protocol, cls, subclass, protocol);
            print_legacy_cups_class_hint(child);
            print_bundle_hint(child);
            putchar('\n');
            print_service_hints(child);
        }
        print_interfaces(child, depth + 1u, remaining, found);
        IOObjectRelease(child);
    }
    IOObjectRelease(iterator);
}

static int search_usb_class(const char *class_name, unsigned *matched,
                            int have_location_hint, uint32_t location_hint)
{
    CFMutableDictionaryRef matching;
    io_iterator_t iterator = 0;
    io_service_t device;
    kern_return_t status;

    matching = IOServiceMatching(class_name);
    if (matching == NULL) {
        fprintf(stderr, "MFC-290C: could not create IOKit matcher\n");
        return -1;
    }
    /* IOKit consumes 'matching' regardless of whether the call succeeds. */
    status = IOServiceGetMatchingServices(MACH_PORT_NULL, matching, &iterator);
    if (status != KERN_SUCCESS) {
        fprintf(stderr, "MFC-290C: USB registry search failed (0x%x)\n",
                (unsigned)status);
        return -1;
    }

    while ((device = IOIteratorNext(iterator)) != 0) {
        uint16_t vendor;
        uint16_t product;

        if (read_u16(device, CFSTR("idVendor"), &vendor) &&
            read_u16(device, CFSTR("idProduct"), &product) &&
            vendor == WHP_MFC290C_USB_VENDOR_ID &&
            product == WHP_MFC290C_USB_PRODUCT_ID) {
            uint16_t revision;
            uint32_t location;
            unsigned remaining = MAX_REGISTRY_NODES;
            unsigned interfaces = 0u;

            ++*matched;
            printf("MFC-290C observed: idVendor=%04x idProduct=%04x",
                   (unsigned)vendor, (unsigned)product);
            if (read_u16(device, CFSTR("bcdDevice"), &revision)) {
                printf(" bcdDevice=%04x", (unsigned)revision);
            }
            if (read_u32(device, CFSTR("locationID"), &location)) {
                printf(" locationID=0x%08x", (unsigned)location);
                if (have_location_hint) {
                    printf(" locationHint=%s",
                           location == location_hint ? "same" : "different");
                }
            } else {
                printf(" locationID=unknown");
                if (have_location_hint) {
                    printf(" locationHint=unavailable");
                }
            }
            print_serial_registry_hint(device);
            printf(" registryClass=%s\n", class_name);
            print_interfaces(device, 0u, &remaining, &interfaces);
            if (interfaces == 0u) {
                puts("  No USB interface entries visible in this registry traversal.");
            }
            if (remaining == 0u) {
                puts("  Registry traversal bounded: some entries may be omitted.");
            }
        }
        IOObjectRelease(device);
    }
    IOObjectRelease(iterator);
    return 0;
}

int main(int argc, char **argv)
{
    unsigned matched = 0u;
    uint32_t location_hint = 0u;
    int have_location_hint = 0;

    if (argc == 2 && strcmp(argv[1], "--architecture-only") == 0) {
        puts("Brother MFC-290C macOS architecture audit (read-only)");
        report_process_architecture();
        puts("No USB registry or CUPS inspection was performed.");
        return 0;
    }
    if (argc == 3 && strcmp(argv[1], "--location-hint") == 0 &&
        whp_mfc290c_parse_location_hint(argv[2], &location_hint)) {
        have_location_hint = 1;
    } else if (argc != 1) {
        fprintf(stderr, "usage: %s [--location-hint 0xNNNNNNNN | --architecture-only]\n", argv[0]);
        return 2;
    }

    puts("Brother MFC-290C macOS USB registry observation (read-only)");
    report_process_architecture();
    if (have_location_hint) {
        printf("Expected topology hint=0x%08x (comparison only; never a filter)\n",
               (unsigned)location_hint);
    }
    if (search_usb_class("IOUSBHostDevice", &matched,
                         have_location_hint, location_hint) != 0) {
        return 1;
    }
    /* Older macOS USB families expose IOUSBDevice instead. Never print
     * duplicate matches for both class families in the same observation.
     */
    if (matched == 0u &&
        search_usb_class("IOUSBDevice", &matched,
                         have_location_hint, location_hint) != 0) {
        return 1;
    }
    if (matched == 0u) {
        puts("No matching 04f9:01fd device visible in the macOS USB registry.");
    }
    puts("Location IDs describe USB topology, not an immutable device identity.");
    puts("USB bcdDevice is a device revision, not the installed Brother firmware letter.");
    puts("No USB transfer performed; registry values are not raw descriptors.");
    whp_mfc290c_report_cups();
    return 0;
}
