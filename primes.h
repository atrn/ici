/*
 * "random" primes (if you know what I mean). Hash functions (of which
 * there is about one per ICI type) tend to use primes in their hashes.
 * The hashes all live in the same space so it is important that they
 * use different primes to keep them independent. So here are lots of
 * different large primes.
 *
 * In practice all sorts of shortcuts are taken and a lot of primeness
 * is thrown away when things overflow etc.
 */
#define ARRAY_PRIME     0x2FF59909UL
#define FILE_PRIME      0x20918C2FUL
#define FLOAT_PRIME     0x2DEF4E41UL
#define FUNC_PRIME      0x00C14737UL
#define INT_PRIME       0x2C006C9FUL
#define MEM_PRIME_0     0x06BD6DE5UL
#define MEM_PRIME_1     0x0816B771UL
#define MEM_PRIME_2     0x0736230DUL
#define OP_PRIME        0x4630278DUL
#define PTR_PRIME_0     0x1ECFFDB1UL
#define PTR_PRIME_1     0x463027B5UL
#define REGEXP_PRIME    0x220897E7UL
#define SET_PRIME_0     0x0EDF42B9UL
#define SET_PRIME_1     0x006BE2B5UL
#define STR_PRIME_0     0x0BFEB2FFUL
#define STR_PRIME_1     0x04067281UL
#define STRUCT_PRIME_0  0x0E210821UL
#define STRUCT_PRIME_1  0x3329D2D7UL
#define STRUCT_PRIME_2  0x184852F1UL
#define UNIQUE_PRIME    0x066F87B5UL

#define FLOAT_PRIME_1   0x1EDD289BUL
#define FLOAT_PRIME_2   0x5010FC67UL
#define FLOAT_PRIME_3   0x5D77DE21UL
#define LIST_PRIME      0x47F46277UL
#define PRIME24         0x0020C323UL
