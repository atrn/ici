#define ICI_NO_OLD_NAMES

/*
 * $Id: uuid.c,v 1.5 2003/03/08 06:48:06 timl Exp $
 *
 * ICI UUID library interface. Placed in the PUBLIC DOMAIN, A.Newman, October 2000.
 */

/*
 * Universal Unique Identifier manipulation
 *
 * The ICI `uuid' module provides facilities for creating and manipulating
 * `Universal Unique Identifiers' or UUIDs. These are also known as
 * `Globally Unique Identifiers' or GUIDs. UUIDs are 128-bit values
 * calculated in a manner that provides a high probabiliity they are
 * unique.
 *
 * The `uuid' module provides a new type, uuid, that represents a UUID. UUIDs
 * are atomic, two UUIDs with the same value are the same object, and are not
 * modifiable in any way.  Being atomic equal UUIDs are both `==' and `eq',
 * i.e. equal UUIDs have equal values and are the same object.
 *
 * The module sites atop libuuid written by Theodore Ts'o and included
 * with the `e2fsprogs' source code distribution. This library is under
 * the GNU Library General Public License and is included with most Linux
 * distributions or is otherwise widely available.
 *
 * This --intro-- and --synopsis-- are part of --ici-uuid-- documentation.
 */

#include <ici.h>
#include <uuid/uuid.h>

#define UUID_PRIME 0x000f4c07

typedef struct
{
    ici_obj_t	o_head;
    uuid_t	u_uuid;
}
ici_uuid_t;

static int uuid_tcode = 0;

static unsigned long
mark_uuid(ici_obj_t *o)
{
    o->o_flags |= ICI_O_MARK;
    return sizeof (ici_uuid_t);
}

static void
free_uuid(ici_obj_t *o)
{
    ici_tfree(o, ici_uuid_t);
}

#define isuuid(o) (ici_typeof(o) == &ici_uuid_type)
#define uuidof(o) ((ici_uuid_t *)(o))

static unsigned long
hash_uuid(ici_obj_t *o)
{
    return ici_crc(UUID_PRIME, (unsigned char *)&uuidof(o)->u_uuid, sizeof (uuid_t));
}

static int
cmp_uuid(ici_obj_t *a, ici_obj_t *b)
{
    return uuid_compare(uuidof(a)->u_uuid, uuidof(b)->u_uuid);
}

static ici_type_t ici_uuid_type =
{
    mark_uuid,
    free_uuid,
    hash_uuid,
    cmp_uuid,
    ici_copy_simple,
    ici_assign_fail,
    ici_fetch_fail,
    "uuid"
};

static ici_uuid_t *
new_uuid(uuid_t uuid)
{
    ici_uuid_t	*u;
    ici_uuid_t  proto;

    ICI_OBJ_SET_TFNZ(&proto, uuid_tcode, 0, 1, sizeof (ici_uuid_t));
    memcpy(proto.u_uuid, uuid, sizeof (uuid_t));
    u = (ici_uuid_t *)ici_atom_probe(ici_objof(&proto));
    if (u != NULL)
    {
        ici_incref(u);
        return u;
    }
    if ((u = ici_talloc(ici_uuid_t)) == NULL)
        return NULL;
    *u = proto;
    ici_rego(u);
    return (ici_uuid_t *)ici_atom(ici_objof(u), 1);
}

/*
 * uuid = uuid.generate()  - generate a new UUID
 *
 * Generate a UUID using either uuid.generate_random() or
 * uuid.generate_time() according to the availability of a
 * source of "good" random numbers. If the system provides
 * a source of cryptographic-grade randomness this function
 * is equivilent to uuid.generate_random() otherwise it is
 * equivilent to uuid.generate_time().
 *
 * This --topic-- forms part of the --ici-uuid-- documentation.
 */
static int
ici_uuid_generate(void)
{
    uuid_t	uu;

    uuid_generate(uu);
    return ici_ret_with_decref(ici_objof(new_uuid(uu)));
}

/*
 * uuid = uuid.generate_random() - generate a "random" UUID
 *
 * Generate a UUID using random numbers for the time fields. If
 * the system does not provide a "good" source of randomness
 * the C library random number generator is used to generate
 * random bytes.
 *
 * This --topic-- forms part of the --ici-uuid-- documentation
 */
static int
ici_uuid_generate_random(void)
{
    uuid_t	uu;

    uuid_generate_random(uu);
    return ici_ret_with_decref(ici_objof(new_uuid(uu)));
}

/*
 * uuid = uuid.generate_time() - generate a "time" UUID
 *
 * Generate a `conventional' UUID that uses the system time
 * in the UUID's time fields.
 *
 * This --topic-- forms part of the --ici-uuid-- documentation.
 */
static int
ici_uuid_generate_time(void)
{
    uuid_t	uu;

    uuid_generate_time(uu);
    return ici_ret_with_decref(ici_objof(new_uuid(uu)));
}

/*
 * uuid = uuid.parse(string) - create a UUID from its string represenation
 *
 * Given a string representation of a UUID, as created by uuid.unparse(),
 * this returns the associated uuid object. See uuid.unparse() for details
 * of the string representation.
 *
 * This --topic-- forms part of the --ici-uuid-- documentation.
 */
static int
ici_uuid_parse(void)
{
    char	*str;
    uuid_t	uu;

    if (ici_typecheck("s", &str))
	return 1;
    if (uuid_parse(str, uu) == -1)
    {
	ici_error = "invalid uuid";
	return 1;
    }
    return ici_ret_with_decref(ici_objof(new_uuid(uu)));
}

/*
 * string = uuid.unparse(uuid) - convert a UUID to its string representation
 *
 * UUIDs have a well defined internal structure and corresponding string
 * representation. This function converts a UUID to its string value.
 * UUID strings are of the form:
 *
 * tttttttt-tttt-tttv-cccc-nnnnnnnnnn
 *
 * Where `t' represents a, hexadecimal, digit that is part of the UUID's
 * time value, `v' represents a digit that contains version information
 * (and time, the version field is only 4-bits), `c' is a sequence number
 * used to overcome clocks with too low a precision and `n' represents a
 * portion of the system's `node' address (Ethernet address).
 *
 * This --topic-- forms part of the --ici-uuid-- documentation.
 */
static int
ici_uuid_unparse(void)
{
    ici_uuid_t	*u;
    char	mybuf[64];

    if (ici_typecheck("o", &u))
	return 1;
    if (!isuuid(ici_objof(u)))
	return ici_argerror(0);
    uuid_unparse(u->u_uuid, mybuf);
    return ici_str_ret(mybuf);
}

/*
 * int = uuid.time(uuid) - return the time portion of a UUID
 *
 * This function returns the time stored within a UUID converted
 * to the local system time.  This is likely only to be valid 
 * if the UUID's type is uuid.TYPE_TIME.
 *
 * This --topic-- forms part of the --ici-uuid-- documentation.
 */
static int
ici_uuid_time(void)
{
    ici_uuid_t		*u;

    if (ici_typecheck("o", &u))
	return 1;
    if (!isuuid(ici_objof(u)))
	return ici_argerror(0);
    return ici_int_ret(uuid_time(u->u_uuid, NULL));
}

/*
 * int = uuid.type(uuid) - return the type of a UUID
 *
 * UUIDs specify a `type' that specifies how the UUID
 * was generated. The types are defined in the section
 * `UUID Types' above.
 *
 * This --topic-- forms part of the --ici-uuid-- documentation.
 */
static int
ici_uuid_typef(void)
{
    ici_uuid_t	*u;

    if (ici_typecheck("o", &u))
	return 1;
    if (!isuuid(ici_objof(u)))
	return ici_argerror(0);
    return ici_int_ret(uuid_type(u->u_uuid));
}

/*
 * int = uuid.variant(uuid) - return the variant of a UUID
 *
 * Returns the variant of a UUID as an integer. This will be
 * one of the four values defined in the `UUID Variants'
 * section above.
 *
 * This --topic-- forms part of the --ici-uuid-- documentation.
 */
static int
ici_uuid_variant(void)
{
    ici_uuid_t	*u;

    if (ici_typecheck("o", &u))
	return 1;
    if (!isuuid(ici_objof(u)))
	return ici_argerror(0);
    return ici_int_ret(uuid_variant(u->u_uuid));
}

static ici_cfunc_t ici_uuid_cfuncs[] =
{
    {ICI_CF_OBJ, "generate", ici_uuid_generate},
    {ICI_CF_OBJ, "generate_random", ici_uuid_generate_random},
    {ICI_CF_OBJ, "generate_time", ici_uuid_generate_time},
    {ICI_CF_OBJ, "parse", ici_uuid_parse},
    {ICI_CF_OBJ, "unparse", ici_uuid_unparse},
    {ICI_CF_OBJ, "time", ici_uuid_time},
    {ICI_CF_OBJ, "type", ici_uuid_typef},
    {ICI_CF_OBJ, "variant", ici_uuid_variant},
    {ICI_CF_OBJ}
};

ici_obj_t *
ici_uuid_library_init(void)
{
    if (ici_interface_check(ICI_VER, ICI_BACK_COMPAT_VER, "uuid"))
        return NULL;
    if ((uuid_tcode = ici_register_type(&ici_uuid_type)) == 0)
        return NULL;
    return ici_objof(ici_module_new(ici_uuid_cfuncs));
}
