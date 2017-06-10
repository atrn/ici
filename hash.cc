#include <stddef.h>
#include <stdint.h>

namespace ici
{

#if defined(ICI_USE_SF_HASH)

#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8) + (uint32_t)(((const uint8_t *)(d))[0]))

uint32_t
ici_superfast_hash(const char * data, int len)
{
    uint32_t hash = len, tmp;
    int rem;

    if (len <= 0 || data == NULL) return 0;

    rem = len & 3;
    len >>= 2;

    /* Main loop */
    for (;len > 0; len--) {
        hash  += get16bits (data);
        tmp    = (get16bits (data+2) << 11) ^ hash;
        hash   = (hash << 16) ^ tmp;
        data  += 2*sizeof (uint16_t);
        hash  += hash >> 11;
    }

    /* Handle end cases */
    switch (rem) {
        case 3: hash += get16bits (data);
                hash ^= hash << 16;
                hash ^= data[sizeof (uint16_t)] << 18;
                hash += hash >> 11;
                break;
        case 2: hash += get16bits (data);
                hash ^= hash << 11;
                hash += hash >> 17;
                break;
        case 1: hash += *data;
                hash ^= hash << 10;
                hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
}
#endif

#if defined(ICI_USE_MURMUR_HASH)
unsigned int
ici_murmur_hash(const unsigned char * data, int len, unsigned int h)
{
    const unsigned int m = 0x7fd652ad;
    const int r = 16;

    h += 0xdeadbeef;

    while (len >= 4)
    {
        h += *(unsigned int *)data;
        h *= m;
        h ^= h >> r;
        data += 4;
        len -= 4;
    }

    switch(len)
    {
    case 3:
        h += data[2] << 16;
    case 2:
        h += data[1] << 8;
    case 1:
        h += data[0];
        h *= m;
        h ^= h >> r;
    };

    h *= m;
    h ^= h >> 10;
    h *= m;
    h ^= h >> 17;

    return h;
}
#endif

} // namespace ici
