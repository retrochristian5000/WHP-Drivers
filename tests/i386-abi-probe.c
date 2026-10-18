/* Compilation-only ABI probe; NOT a Windows 95 driver. */
#if !defined(__i386__) && !defined(_M_IX86)
#error "This probe requires the 32-bit x86 target"
#endif
_Static_assert(sizeof(void *) == 4, "i386 pointers must be 32 bits");
_Static_assert(sizeof(unsigned long) == 4, "Windows i386 long must be 32 bits");

unsigned long whp_driver_abi_probe(unsigned long value)
{
    return value ^ 0x95UL;
}
