/* Brother MFC-290C: read-only macOS IORegistry USB inventory.
 * This is a diagnostic, not a USB driver. No USB requests or device opens.
 */
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>

#include <stdint.h>
#include <stdio.h>

#include "mfc290c.h"
#include "mfc290c_cups.h"

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

static void print_interface_field(io_registry_entry_t entry,
                                  CFStringRef key, const char *label)
{
    uint32_t value;

    if (read_u32(entry, key, &value) && value <= UINT8_MAX) {
        printf(" %s=0x%02x", label, (unsigned)value);
    } else {
        printf(" %s=unknown", label);
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
            ++*found;
            printf("  interface");
            print_interface_field(child, CFSTR("bInterfaceNumber"), "number");
            print_interface_field(child, CFSTR("bInterfaceClass"), "class");
            print_interface_field(child, CFSTR("bInterfaceSubClass"), "subclass");
            print_interface_field(child, CFSTR("bInterfaceProtocol"), "protocol");
            print_bundle_hint(child);
            putchar('\n');
            print_service_hints(child);
        }
        print_interfaces(child, depth + 1u, remaining, found);
        IOObjectRelease(child);
    }
    IOObjectRelease(iterator);
}

static int search_usb_class(const char *class_name, unsigned *matched)
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
            }
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

    if (argc != 1) {
        fprintf(stderr, "usage: %s\n", argv[0]);
        return 2;
    }
    puts("Brother MFC-290C macOS USB registry observation (read-only)");
    if (search_usb_class("IOUSBHostDevice", &matched) != 0) {
        return 1;
    }
    /* Older macOS USB families expose IOUSBDevice instead. Never print
     * duplicate matches for both class families in the same observation.
     */
    if (matched == 0u && search_usb_class("IOUSBDevice", &matched) != 0) {
        return 1;
    }
    if (matched == 0u) {
        puts("No matching 04f9:01fd device visible in the macOS USB registry.");
    }
    puts("No USB transfer performed; registry values are not raw descriptors.");
    whp_mfc290c_report_cups();
    return 0;
}
