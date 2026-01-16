#include "nexlink_core.h"
#include "nexlink_usb.h"

int nexlink_scan(
    char serials[][64],
    char products[][64],
    int max_count)
{
    if (!serials || !products || max_count <= 0)
        return 0;

    return usb_scan(serials, products, max_count);
}
