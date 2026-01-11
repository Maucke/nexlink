#include "nexlink_core.h"
#include "nexlink_usb.h"

int nexlink_scan(
    char serials[][64],
    int max_count)
{
    if (!serials || max_count <= 0)
        return 0;

    return usb_scan(serials, max_count);
}
