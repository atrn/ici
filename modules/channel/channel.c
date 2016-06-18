/*
 * $Id: channel.c,v 1.2 2003/03/08 06:48:05 timl Exp $
 */

/*
 * Inter-thread link object
 * 
 * A channel is a communications link between threads. Channels
 * allows threads to send objects to one another and to synchronize
 * their actions.
 *
 * Channels have a defined "capacity", the number of objects that
 * may be "in" the channel at any one time.  The capacity of a
 * channel is defined when the channel is created and defaults
 * to one.  Channels with a capacity of one are "unbuffered" and
 * any input and output performs an implicit thread synchronization.
 * Channels with capacity's greater than one are "buffered" (to
 * the degree of the channel's capacity).
 *
 * Channels support two fundamental operations, "get" and "put".  To
 * send an object along a channel the "put" operation is used, to
 * receieving an object uses the "get" operation. Any type of object
 * may be sent via channels including channels themselves. There is
 * no defined "end of communications" object however NULL is typically
 * used to indicate that no more communications will occur along a
 * channel.
 *
 * Channels perform synchronization between threads. If, when a thread
 * attempts to "get" from a channel, there are no objects
 * available the thread is blocked until an object is available.
 * Correspondingly, when a thread attempts to "put" an object into a
 * channel and there is no room, the thread is blocked until room
 * becomes available. This behaviour forms the basis for channels'
 * synchronization mechanism.
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

#define ICI_NO_OLD_NAMES

#include <ici.h>
#include "icistr.h"
#include <icistr-setup.h>

static ici_objwsup_t *channel_class;
static int           channel_tcode;

typedef struct
{
    ici_objwsup_t o_head;
    ici_array_t *c_q;
    long c_capacity;
    ici_obj_t *c_altobj;
}
channel_t;

#define channelof(o) ((channel_t *)(o))
#define ischannel(o) (ici_typeof(o) == &channel_type)

static unsigned long
mark_channel(ici_obj_t *o)
{
    unsigned long mem = sizeof (channel_t);
    o->o_flags |= ICI_O_MARK;
    mem += ici_mark(ici_objwsupof(o)->o_super);
    mem += ici_mark(channelof(o)->c_q);
    if (channelof(o)->c_altobj != NULL)
	mem += ici_mark(channelof(o)->c_altobj);
    return mem;
}

static void
free_channel(ici_obj_t *o)
{
    ici_tfree(o, channel_t);
}

static int
assign_super_channel(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v, ici_struct_t *b)
{
    return ici_assign_fail(o, k, v);
}

static int
fetch_super_channel(ici_obj_t *o, ici_obj_t *k, ici_obj_t **v, ici_struct_t *b)
{
    *v = ici_fetch(ici_objwsupof(o)->o_super, k);
    return *v == NULL;
}

static int
assign_base_channel(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v)
{
    return ici_assign_fail(o, k, v);
}

static ici_obj_t *
fetch_base_channel(ici_obj_t *o, ici_obj_t *k)
{
    return ici_fetch(ici_objwsupof(o)->o_super, k);
}

static ici_type_t channel_type =
{
    mark_channel,
    free_channel,
    ici_hash_unique,
    ici_cmp_unique,
    ici_copy_simple,
    ici_assign_fail,
    fetch_base_channel,
    "channel",
    NULL,
    NULL,
    NULL,
    assign_super_channel,
    fetch_super_channel,
    assign_base_channel,
    fetch_base_channel
};


/*
 * channel = channel:new([capacity])
 *
 * Create a new channel with the given capacity. If the capacity
 * is not given it defaults to one, if it is given it must be a
 * positive integer (i.e, a value greater than zero).
 *
 * This --topic-- forms part of the --ici-channel-- documentation.
 */
static int
channel_new(ici_objwsup_t *klass)
{
    long capacity = 1;
    channel_t *chan;

    if (ici_method_check(ici_objof(klass), 0))
        return 1;
    if (ICI_NARGS() != 0)
    {
        if (ici_typecheck("i", &capacity))
            return 1;
        if (capacity < 1)
        {
            ici_error = "channel capacity must be a positive value";
            return 1;
        }
    }
    if ((chan = ici_talloc(channel_t)) == NULL)
        return 1;
    if ((chan->c_q = ici_array_new(capacity)) == NULL)
    {
        ici_tfree(chan, channel_t);
        return 1;
    }
    ICI_OBJ_SET_TFNZ(chan, channel_tcode, ICI_O_SUPER, 1, 0);
    chan->o_head.o_super = channel_class;
    chan->c_capacity = capacity;
    chan->c_altobj = NULL;
    ici_rego(chan);
    return ici_ret_with_decref(ici_objof(chan));
}

/* 
 * any = channel:get()
 *
 * Return the next object from the channel.  If there are no objects
 * in the channel the caller is blocked until an object is available.
 *
 * This --topic-- forms part of the --ici-channel-- documentation.
 */
static int
channel_get(ici_objwsup_t *inst)
{
    ici_obj_t *o;
    ici_array_t *q;

    if (ici_method_check(ici_objof(inst), channel_tcode))
        return 1;
    q = channelof(inst)->c_q;
    while (ici_array_nels(q) < 1)
    {
        if (ici_waitfor(ici_objof(q)))
            return 1;
    }
    o = ici_array_rpop(q);
    ici_wakeup(ici_objof(q));
    if (channelof(inst)->c_altobj != NULL)
	ici_wakeup(channelof(inst)->c_altobj);
    return ici_ret_no_decref(o);
}

/*
 * channel:put(any)
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
channel_put(ici_objwsup_t *inst)
{
    ici_obj_t *o;
    ici_array_t *q;

    if (ici_method_check(ici_objof(inst), channel_tcode))
        return 1;
    if (ici_typecheck("o", &o))
        return 1;
    q = channelof(inst)->c_q;
    while (ici_array_nels(q) >= channelof(inst)->c_capacity)
    {
        if (ici_waitfor(ici_objof(q)))
            return 1;
    }
    ici_array_push(q, o);
    ici_wakeup(ici_objof(q));
    if (channelof(inst)->c_altobj != NULL)
	ici_wakeup(channelof(inst)->c_altobj);
    return ici_null_ret();
}

/*
 * int = channel:capacity()
 *
 * Return the capacity of a channel, the number of objects
 * thay may be queueud within a channel.
 *
 * This --topic-- forms part of the --ici-channel-- documentation.
 */
static int
channel_capacity(ici_objwsup_t *inst)
{
    if (ici_method_check(ici_objof(inst), channel_tcode))
        return 1;
    return ici_int_ret(channelof(inst)->c_capacity);
}

/*
 * int = channel:queued()
 *
 * Return the number of objects queued on a channel, the number
 * of objects that may be "gotten" without blocking.
 *
 * This --topic-- forms part of the --ici-channel-- documentation.
 */
static int
channel_queued(ici_objwsup_t *inst)
{
    if (ici_method_check(ici_objof(inst), channel_tcode))
        return 1;
    return ici_int_ret(ici_array_nels(channelof(inst)->c_q));
}

/*
 * int = channel:space()
 *
 * Returns the space available within a channel, the number
 * of objects that may be "put" into the channel before the
 * sending thread will block.  The space within a channel
 * is its capacity minus the number of objects queued.
 *
 * This --topic-- forms part of the --ici-channel-- documentation.
 */
static int
channel_space(ici_objwsup_t *inst)
{
    if (ici_method_check(ici_objof(inst), channel_tcode))
        return 1;
    return ici_int_ret(channelof(inst)->c_capacity - ici_array_nels(channelof(inst)->c_q));
}


static int
alt_setup(ici_array_t *alts, ici_obj_t *obj)
{
    int n = ici_array_nels(alts);
    int i;

    for (i = 0; i < n; ++i)
    {
	ici_obj_t *o = ici_array_get(alts, i);
        channel_t *chan;
	if (ischannel(o))
	{
	    chan = channelof(o);
	    chan->c_altobj = obj;
	}
	else if (!ici_isnull(o))
	{
	    ici_error = "bad object in array passed to channel.alt";
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
	if (ischannel(o) && ici_array_nels(channelof(o)->c_q) > 0)
	    idx = i;
    }
    return idx;
}

/*
 * int = channel.alt(array)
 *
 * Determine which of a set of channels is ready to perform I/O
 * and returns its "number".
 */
static int
channel_alt(ici_objwsup_t *ign)
{
    int idx;
    ici_array_t *alts;
    (void)ign;

    if (ici_typecheck("a", &alts))
        return 1;
    if (alt_setup(alts, ici_objof(alts)))
	return 1;
    while ((idx = alt(alts)) == -1)
    {
        if (ici_waitfor(ici_objof(alts)))
            return 1;
    }
    alt_setup(alts, NULL);
    return ici_int_ret(idx);
}

static ici_cfunc_t channel_cfuncs[] =
{
    {ICI_CF_OBJ, "new", channel_new},
    {ICI_CF_OBJ, "get", channel_get},
    {ICI_CF_OBJ, "put", channel_put},
    {ICI_CF_OBJ, "capacity", channel_capacity},
    {ICI_CF_OBJ, "queued", channel_queued},
    {ICI_CF_OBJ, "alt", channel_alt},
    {ICI_CF_OBJ, "space", channel_space},
    {ICI_CF_OBJ}
};

ici_obj_t *
ici_channel_library_init(void)
{
    if (ici_interface_check(ICI_VER, ICI_BACK_COMPAT_VER, "channel"))
        return NULL;
    if (init_ici_str())
        return NULL;
    if ((channel_tcode = ici_register_type(&channel_type)) == 0)
        return NULL;
    if ((channel_class = ici_class_new(channel_cfuncs, NULL)) == NULL)
        return NULL;
    /* ici_decref(channel_class); */
    return ici_objof(channel_class);
}
