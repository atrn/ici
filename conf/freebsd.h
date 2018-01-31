#ifndef ICI_CONF_H
#define ICI_CONF_H

#include <unistd.h>
#include <math.h>

#define NOEVENTS
#define ICI_HAS_BSD_STRUCT_TM
#define CONFIG_STR "FreeBSD"

#define  UNLIKELY(X) __builtin_expect((X), 0)
#define  LIKELY(X)   __builtin_expect((X), 1)

#define BSD 1

#define ntohll(x) (((uint64_t)(ntohl((uint32_t)(((x) << 32ull) >> 32ull) )) << 32ull) | ntohl(((uint32_t)((x) >> 32ull))))                                        
#define htonll(x) ntohll(x)

#endif /*ICI_CONF_H*/
