/* Real code bounds for the address guards (gfx.c, naudio.c, os_rsp.c, os_stub.c): computed by the linker, never
 * hardcoded, so they follow the code size. Passed to the Wii link after n64_syms.ld. */
__text_start = ADDR(.init);
__etext = ADDR(.text) + SIZEOF(.text);
