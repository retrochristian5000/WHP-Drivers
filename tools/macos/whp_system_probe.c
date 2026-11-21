/* Read-only native macOS system inventory: no DOS BIOS interrupts,
 * USB transfers, PCI configuration reads or device reconfiguration.
 */
#include <IOKit/IOKitLib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/sysctl.h>
#include <sys/utsname.h>

#include "../probe/probe_scope.h"

#define WHP_MAX_REGISTRY_SERVICES 4096u

/* Enumerates published registry services, not actual hardware inventory.
 * IOKit consumes the matching dictionary. Bounded to prevent a runaway scan.
 */
static void report_service_count(const char *label, const char *service_class)
{
    CFMutableDictionaryRef matching;
    io_iterator_t iterator = IO_OBJECT_NULL;
    io_service_t service;
    kern_return_t status;
    unsigned count = 0u;
    int truncated = 0;

    matching = IOServiceMatching(service_class);
    if (matching == NULL) {
        printf("%s registryServices=unknown\n", label);
        return;
    }
    status = IOServiceGetMatchingServices(MACH_PORT_NULL, matching, &iterator);
    if (status != KERN_SUCCESS) {
        printf("%s registryServices=unknown\n", label);
        return;
    }
    while ((service = IOIteratorNext(iterator)) != IO_OBJECT_NULL) {
        IOObjectRelease(service);
        ++count;
        if (count == WHP_MAX_REGISTRY_SERVICES) {
            truncated = 1;
            break;
        }
    }
    IOObjectRelease(iterator);
    if (truncated) {
        printf("%s registryServicesAtLeast=%u (bounded)\n", label, count);
    } else {
        printf("%s registryServices=%u\n", label, count);
    }
}

static void report_memory(void)
{
    uint64_t physical = 0u;
    size_t length = sizeof(physical);

    if (sysctlbyname("hw.memsize", &physical, &length, NULL, 0) == 0 &&
        length == sizeof(physical) && physical != 0u) {
        printf("macOS physicalMemoryBytes=%llu (not DOS conventional memory)\n",
               (unsigned long long)physical);
    } else {
        puts("macOS physicalMemoryBytes=unknown");
    }
}

static void report_kernel(void)
{
    struct utsname info;
    if (uname(&info) == 0) {
        printf("macOS darwinKernelRelease=%s machine=%s\n",
               info.release, info.machine);
    } else {
        puts("macOS darwinKernelRelease=unknown machine=unknown");
    }
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        puts("usage: whp-system-probe [--help]");
        puts("Read-only macOS kernel/memory/PCI/serial registry summary.");
        return 0;
    }
    if (argc != 1) {
        fputs("usage: whp-system-probe [--help]\n", stderr);
        return 2;
    }

    puts(whp_probe_scope_line(WHP_PROBE_SCOPE_MACOS_SYSTEM));
    report_kernel();
    report_memory();
    report_service_count("PCI", "IOPCIDevice");
    report_service_count("serial", "IOSerialBSDClient");
    puts("Registry service counts do not prove connected hardware or active drivers.");
    puts("No DOS interrupts, USB enumeration, PCI config access or transfers performed.");
    return 0;
}
