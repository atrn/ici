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

unsigned long channel_type::mark(ici_obj_t *o)
{
    unsigned long mem = sizeof (ici_channel_t);
    o->o_flags |= ICI_O_MARK;
    mem += ici_mark(ici_objwsupof(o)->o_super);
    mem += ici_mark(ici_channelof(o)->c_q);
    if (ici_channelof(o)->c_altobj != NULL)
        mem += ici_mark(ici_channelof(o)->c_altobj);
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
    long                capacity = 0;
    ici_channel_t       *chan;

    if (ICI_NARGS() != 0)
    {
        if (ici_typecheck("i", &capacity))
            return 1;
        if (capacity < 0)
        {
            ici_set_error("channel capacity must be non-negative");
            return 1;
        }
    }
    // chan = ici_nalloc(sizeof (ici_channel_t));
    chan = ici_talloc(ici_channel_t);
    if (chan == NULL)
        return 1;
    if ((chan->c_q = ici_array_new(capacity ? capacity : 1)) == NULL)
    {
        ici_tfree(chan, ici_channel_t);
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
    ici_obj_t *c;
    ici_obj_t *o;
    ici_array_t *q;

    if (ici_typecheck("o", &c))
        return 1;
    if (!ici_ischannel(c))
        return ici_argerror(0);
    q = ici_channelof(c)->c_q;
    while (ici_array_nels(q) < 1)
    {
        if (ici_waitfor(q))
            return 1;
    }
    o = ici_array_rpop(q);
    ici_wakeup(q);
    if (ici_channelof(c)->c_altobj != NULL)
	ici_wakeup(ici_channelof(c)->c_altobj);
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
    ici_obj_t *c;
    ici_obj_t *o;
    ici_array_t *q;

    if (ici_typecheck("oo", &c, &o))
        return 1;
    if (!ici_ischannel(c))
        return 1;
    q = ici_channelof(c)->c_q;

    // unbuffered
    if (ici_channelof(c)->c_capacity == 0)
    {
        while (ici_array_nels(q) > 0)
        {
            if (ici_waitfor(q))
                return 1;
        }
    }
    else
    {
        while (ici_array_nels(q) >= ici_channelof(c)->c_capacity)
        {
            if (ici_waitfor(q))
                return 1;
        }
    }
    ici_array_push(q, o);
    ici_wakeup(q);
    if (ici_channelof(c)->c_altobj != NULL)
        ici_wakeup(ici_channelof(c)->c_altobj);
    return ici_null_ret();
}

//================================================================

static int
alt_setup(ici_array_t *alts, ici_obj_t *obj)
{
    int n = ici_array_nels(alts);
    int i;

    for (i = 0; i < n; ++i)
    {
	ici_obj_t *o = ici_array_get(alts, i);
        ici_channel_t *chan;
	if (ici_ischannel(o))
	{
	    chan = ici_channelof(o);
	    chan->c_altobj = obj;
	}
	else if (!ici_isnull(o))
	{
	    ici_set_error("bad object in array passed to channel.alt");
	    return 1;
	}
    }
    return 0;
}

static int
alt(ici_array_t *alts)
{
    int idx = -1;
    int n = ici_array_nels(alts);
    int i;

    for (i = 0; i < n && idx == -1; ++i)
    {
        ici_obj_t *o = ici_array_get(alts, i);
        if (ici_ischannel(o) && ici_array_nels(ici_channelof(o)->c_q) > 0)
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
    ici_array_t *alts;

    if (ici_typecheck("a", &alts))
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
    ICI_CFUNCS_END
};

} // namespace ici
