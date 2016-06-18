#ifndef	ICI_NET_NET_H
#define	ICI_NET_NET_H

#ifndef	ICI_OBJECT_H
#include <object.h>
#endif

#ifdef _WINDOWS
/*
 * Windows uses a special type to represent its SOCKET descriptors.
 * For correctness we include winsock.h here. Other platforms (i.e.,
 * BSD) are easier and use integers.
 */
#include <winsock.h>
#else
#define	SOCKET	int
#endif

typedef struct skt
{
    object_t	o_head;
    SOCKET	s_skt;
    int		s_closed;
}
net_skt_t;
#define	netsktof(o)	((net_skt_t *)(o))
#define	isnetskt(o)	(ici_typeof(o) == &net_netsocket_type)

extern type_t	net_netsocket_type;

#endif /* ICI_NET_NET_H */
