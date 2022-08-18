# CHIP-8
A CHIP-8 emulator in C with SDL2

This is my venture into emulation development. My ultimate goal is to better understand hardware emulation,
and the CHIP-8 looks like the perfect primer to start my journey.

The emulator is simple enough, with 2-byte instructions, 16 registers to store intermediate values, a stack for subroutines
and an address space of 4KB. This is also the perfect project to start understanding SDL2 for rendering graphics and using it
for a practical project. Other techniques often used in lower-level programming such as bit manipulation is seen all over
the place. CHIP-8 is my first emulator, but it certainly is not my last. Thanks to this I was able to progress into
emulating the NES.
