# "vector" arithmetic support

The `vec32` and `vec64` types provide support for _vector_
arithmetic. A _vec_ object is an array of N values, either C `float`
or `double`. Additionally each vec has a map to store user-defined
_properties_.

Vec objects act much like arrays and may be indexed by an integer to
access to the i'th element of the vec, up to its _size_. Vec objects
also support ariethmetic operations with both vec and scalar terms.

Where possible vec objects are implemented using processor specific
acceleration libraries. In particular on Intel processors the IPP
library may be used and ICI contains a number of optimizations that
use IPP, i.e. all arithmetic using vec types calls IPP functions and a
corresponding module provides access to more of the IPP library.

## Basics

When a vec object is created the user defines its maximum size, known
as the vec's _capacity_. The capacity is used to allocate the memory
used for the vec's array (done when the vec is created).

In addition to a _capacity_ a vec also has a _size_, the count of vec
elements in use, between 0 and capacity. Setting the value of a vec
element sets its size to that element's index plus one. Other
operations may directly set the size and some functions _fill_ vec's
and _resize_ them to their capacity.

## Properties

Each vec has a _properties_ map used to store user-defined information
about a vec, e.g. audio processing using vec objects often includes a
`samplerate` property. To access properties vec's are indexed by an
non-integer key, typically a string, and typically via ICI's
dot-notation.  The resultant _fetch_, or_assign_, operation is
forwarded to the vec's properties map, e.g.,

    block.samplerate = 44100;
    block.format = "32-bit PCM"

## Operators

Vector types implement basic arithmetic operators.

- vec + vec
- vec - vec
- vec * vec
- vec / vec
- vec + scalar
- vec - scalar
- vec * scalar
- vec / scalar

## Functions

## IPP

On Darwin and Linux the Intel IPP libraries may be used to implement
the vec type. IPP provides for higher performance than the _naive_
ad purposefully simple C++ inplementation.

A corresponding `ipp` module provides further access to the Intel IPP
libraries. It uses vec objects in its interface to the IPP-implemented
functions.

The module provides access to IPP's math functions (sqrt, log etc..),
filters, transforms (ffts, DFTs) and other IPP features.

The interface provided by the module is _ICI friendly_, vec objects
represent the blocks of data used by IPP with the vec type, float or
double, defining which IPP functions to use. The module uses function
names where the leaning _ipp_ is removed so a C/C++ function such as
`ippsSqrt_32f_I(ptr, len)` becomes the ICI `ipp.Sqrt(vec)`

Functions such as `ipp.Add` are also provided for completeness
but the ICI interpreter is assumed to use IPP for its vec type so
direct expressions are supported (e.g. `vec + 1.0`)
