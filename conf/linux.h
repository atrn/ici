#ifndef ICI_CONF_H
#define ICI_CONF_H

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

#define NOEVENTS        /* Event loop and associated processing. */

/*
 * End of ici.h export. --ici.h-end--
 */

#define ICI_HAS_BSD_STRUCT_TM
#define ICI_SW_CRC

/*
 * Mentioned in the version string.
 */
#define CONFIG_STR      "Linux"

/*
 * To have GNU libc enable various things we need to...
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#define  UNLIKELY(X) __builtin_expect((X), 0)
#define  LIKELY(X)   __builtin_expect((X), 1)

#define ntohll(x) (((uint64_t)(ntohl((uint32_t)(((x) << 32ull) >> 32ull) )) << 32ull) | ntohl(((uint32_t)((x) >> 32ull))))                                        
#define htonll(x) ntohll(x)

#endif /*ICI_CONF_H*/
