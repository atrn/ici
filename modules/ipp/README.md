# Numerics Using ICI

The ICI programming language is a lesser-known dynamic programming
language. The first version of ici appeared in the late 1980s and was
used in embedded systems and as a general purpose programming language
in both UNIX and MS-DOS/Windows environments. In the 1990s ici formed
the basis of a number of technologies and products from Canon and a
number of other companies. Most notably it was used as the language
part of Canon's OpenPage graphics language.

The use of ici faltered after the principal developers either left
Canon or moved to other areas. Also, without a language _evanglist_ it
it difficult to maintain a sufficiently engaged user base and the ici
developers were not the types to evangelize its merits.

Recent the ici language has been revived and its code base brought
up to date in many areas. Porting the implementation to C++ from its
original C and revisiting a number of aspects of the language.

One recent addition is the _vec_ type, which comes in two forms,
`vec32f` and `vec64f`. A _vec_ is an array of some number of single or
double precision, C, floating point values. I.e. it is a numeric
vector of up to N elements.

Arithmetic is defined on vec values. You can perform all the basic
operations uing both vector and scalar operands.

## Intel IPP

ici may be built with Intel IPP support where operations on _vec_
types, in particular arithmetic, are implemented using IPP functions.

## vec-types

## ipp module

## data
