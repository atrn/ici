#define ICI_CORE
#include "fwd.h"
#include "channel.h"
#include "cfunc.h"
#include "array.h"
#include "int.h"
#include "null.h"
#include "str.h"

/*
 * TODO: add close() support
 */

/*
 * Inter-thread link object
 * 
 * A channel is a communications link between threads. Channels
 * allows threads to send objects to one another and to synchronize
 * their actions.
 *
 * Channels support two fundamental operations, "get" and "put".  To
 * send an object along a channel the "put" operation is used,
 * receieving an object uses the "get" operation. Any type of object
 * may be sent via channels including channels themselves. There is no
 * defined "end of communications" object however NULL is typically
 * used to indicate that no more communications will occur along a
 * channel.
 *
 * Channels have a defined "capacity",  the number of objects that may
 * be "in" the channel at any one  time.  The capacity of a channel is
 * defined when the channel is created and defaults to zero.  Channels
 * with  a capacity  of zero  are  "unbuffered" and  input and  output
 * operations   block   until   some  nother   thread   performs   the
 * complementary operation. This  provides for synchronization between
 * threads.
 *
 * Channels with capacity's greater than one are "buffered" - to the
 * degree of the channel's capacity. Objects may be "put" without
 * blocking until the channel is "full".
 *
 * Channels perform synchronization between threads. If there are no
 * objects when a thread attempts to "get" from a channel the thread
 * is blocked until an object is available.  Correspondingly, when a
 * thread attempts to "put" an object into a channel and there is no
 * room, the thread is blocked until room becomes available. This
 * behaviour forms the basis for channels' synchronization mechanism.
 *
 * The size, or capacity, of a channel is defined when the channel is
 * created and, currently, may not be changed*.  By default channels
 * have a capacity of one, i.e., only a single object may be in the
 * channel at the one time and the channel is an unbuffered synchronous
 * communications mechanism.  Sizes greater than one allow the channel
 * to buffer that many objects and to decouple the communicating threads
 * to that degree.
 *
 * * It is feasible that channel capacity may be made variable and
 * facilities added to support this however given the limited experience
 * with channels within ICI any decision on this matter is being
 * delayed until experience shows it is desirable.
 *
 * This --intro-- and --synopsis-- are part of --ici-channel-- documentation.
 *
 */

#include "channel.h"

namespace ici
{

size_t channel_type::mark(object *o)
{
    o->setmark();
    auto mem = typesize();
    mem += ici_mark(ici_objwsupof(o)->o_super);
    mem += ici_mark(channelof(o)->c_q);
    if (channelof(o)->c_altobj != NULL)
        mem += ici_mark(channelof(o)->c_altobj);
    return mem;
}

/*
 * channel = channel([capacity])
 *
 * Create a new channel with the given capacity. If the capacity
 * is not given it defaults to one, if it is given it must be a
 * positive integer (i.e, a value greater than zero).
 *
 * This --topic-- forms part of the --ici-channel-- documentation.
 */
static int
f_channel(...)
{
    size_t  capacity = 0;
    channel *chan;

    if (NARGS() != 0)
    {
        long val;
        if (typecheck("i", &val))
            return 1;
        if (val < 0)
        {
            ici_set_error("channel capacity must be non-negative");
            return 1;
        }
        capacity = size_t(val);
    }
    // chan = ici_nalloc(sizeof (ici_channel_t));
    chan = ici_talloc(channel);
    if (chan == NULL)
        return 1;
    if ((chan->c_q = ici_array_new(capacity ? capacity : 1)) == NULL)
    {
        ici_tfree(chan, channel);
        return 1;
    }
    ICI_OBJ_SET_TFNZ(chan, ICI_TC_CHANNEL, ICI_O_SUPER, 1, 0);
    chan->c_capacity = capacity;
    chan->c_altobj = NULL;
    ici_rego(chan);
    return ici_ret_with_decref(chan);
}

/*
 * any = get(channel)
 * 
 * Return the next object from the channel.  If there are no objects
 * in the channel the caller is blocked until an object is available.
 */
static int
f_get(...)
{
    object *c;
    object *o;
    array *q;

    if (typecheck("o", &c))
        return 1;
    if (!ischannel(c))
        return ici_argerror(0);
    q = channelof(c)->c_q;
    while (q->len() < 1)
    {
        if (ici_waitfor(q))
            return 1;
    }
    o = q->rpop();
    ici_wakeup(q);
    if (channelof(c)->c_altobj != NULL)
	ici_wakeup(channelof(c)->c_altobj);
    return ici_ret_no_decref(o);
}

/*
 * put(channel, any)
 *
 * Write an object to a channel.  If the channel has space for the
 * object the object is placed in the channel and the caller continues
 * execution.  If there is no space in the channel for the object the
 * caller is blocked until space is made available through another
 * threads calls to channel:get().
 *
 * This --topic-- forms part of the --ici-channel-- documentation.
 */
static int
f_put(...)
{
    object *c;
    object *o;
    array *q;

    if (typecheck("oo", &c, &o))
        return 1;
    if (!ischannel(c))
        return 1;
    q = channelof(c)->c_q;

    // unbuffered
    if (channelof(c)->c_capacity == 0)
    {
        while (q->len() > 0)
        {
            if (ici_waitfor(q))
                return 1;
        }
    }
    else
    {
        while (q->len() >= channelof(c)->c_capacity)
        {
            if (ici_waitfor(q))
                return 1;
        }
    }
    q->push(o);
    ici_wakeup(q);
    if (channelof(c)->c_altobj != NULL)
        ici_wakeup(channelof(c)->c_altobj);
    return ici_null_ret();
}

//================================================================

static int
alt_setup(array *alts, object *obj)
{
    int n = alts->len();
    int i;

    for (i = 0; i < n; ++i)
    {
	object *o = alts->get(i);
        channel *chan;
	if (ischannel(o))
	{
	    chan = channelof(o);
	    chan->c_altobj = obj;
	}
	else if (!isnull(o))
	{
	    ici_set_error("bad object in array passed to channel.alt");
	    return 1;
	}
    }
    return 0;
}

static int
alt(array *alts)
{
    int idx = -1;
    int n = alts->len();
    int i;

    for (i = 0; i < n && idx == -1; ++i)
    {
        object *o = alts->get(i);
        if (ischannel(o) && channelof(o)->c_q->len() > 0)
            idx = i;
    }
    return idx;
}

/*
 * int = alt(array)
 *
 * Determine which of a collection of channels is ready to perform I/O
 * and returns its index within that collection (allowing the I/O to
 * be performed).
 */
static int
f_alt(...)
{
    int idx;
    array *alts;

    if (typecheck("a", &alts))
        return 1;
    if (alt_setup(alts, alts))
	return 1;
    while ((idx = alt(alts)) == -1)
    {
        if (ici_waitfor(alts))
            return 1;
    }
    alt_setup(alts, NULL);
    return ici_int_ret(idx);
}

ICI_DEFINE_CFUNCS(channel)
{
    ICI_DEFINE_CFUNC(channel,   f_channel),
    ICI_DEFINE_CFUNC(get,       f_get),
    ICI_DEFINE_CFUNC(put,       f_put),
    ICI_DEFINE_CFUNC(alt,       f_alt),
    ICI_CFUNCS_END()
};

} // namespace ici
