/* Dr. Mario 64 -> Wii native port: per-axis wrap mode host test (task 24).
 * GATE: gcc -O1 -Iport/gfx -Iport/include -o twrap test_gfx_wrap.c gfx_wrap.c
 * Prints "wrap OK" when all five mapping cases match the RDP rules. */

#include <stdio.h>
#include "gfx_wrap.h"

static int fails = 0;

static void check(int cm, int mask, int w, int mode, int size)
{
    int gx_mode = -1, gx_size = -1;
    gfx_axis_mode(cm, mask, w, &gx_mode, &gx_size);
    if (gx_mode != mode || gx_size != size) {
        printf("FAIL cm=%d mask=%d w=%d -> got (%d,%d) want (%d,%d)\n",
               cm, mask, w, gx_mode, gx_size, mode, size);
        fails++;
    }
}

int main(void)
{
    check(0, 5, 32, 1, 32); /* wrap, period 32 -> REPEAT,32 */
    check(1, 4, 16, 2, 16); /* mirror, period 16 -> MIRROR,16 */
    check(2, 5, 20, 0, 32); /* clamp, POT(20)=32 -> CLAMP,32 */
    check(0, 0, 24, 0, 32); /* mask 0 behaves like clamp -> CLAMP,32 */
    check(3, 4, 16, 0, 16); /* mirror|clamp -> clamp wins -> CLAMP,16 */
    if (fails == 0) printf("wrap OK\n");
    return fails != 0;
}
