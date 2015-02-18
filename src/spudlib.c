#include <string.h>

#include "spudlib.h"

bool spud_isSpud(const uint8_t *payload, uint16_t length)
{
    return ( memcmp(payload, (void *)SpudMagicCookie, SPUD_MAGIC_COOKIE_SIZE) == 0 );
}
