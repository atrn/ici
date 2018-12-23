# ICI Modules

This is a collection of dynamically loaded modules to extend ICI in
various ways. Modules are loaded upon demand as part of ICI's name
resolution process and user-code can simply use a module without any
prior declaration. This assumes the module is installed in the
_normal_ places.

Modules can contain both ICI and C++ code and by convention a module
is represented as an ICI map containing various definitions, the
module's _scope_. A solely native code modules may, however, represent
itself as any type which allows data-only modules and other oddities.

## Modules

- bignum  
Arbitrary precision numbers.
- env  
Simple process environment access.
- example  
An example C++ module.
- small  
A number of ICI-only _modules_.

## Example Module

The `example` directory contains an example of a C module and the code
is heavily commented describing the whats and whys of a basic ICI
module.


## Building

The combination of make and dmake is used in this directory tree. I
haven't got around to writing the cmake stuff yet.
