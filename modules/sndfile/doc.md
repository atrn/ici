# ici libsndfile i/f

The sndfile module provides access to the libsndfile audio file I/O
library. The module provides a number of functions that map to those
in libsndfile and defines a number of types to represent audio files
and their contents.

The sndfile module has been built and tested on MacOS, FreeBSD and a
number of Linux systems with no issues.

## Copyright and license

Because libsndfile is GPL'd this module is also GPL'd, including the
indirect code such as the frames data type.  The GPL applies only to
this module which is loaded, at runtime, by the non-GPL'd ICI.

## functions

- open(path)  
Open the named file for reading returning a sndfile object.
- create(path, format, samplerate, channels)  
Open the named file for wrinting using the given format.
- read  
Read _frames_ from a file.
- write  
Write _frames_ to a file.
- close  
Close a file.

The `open` and `create` functions return a `sndfile` object.
A `sndfile` is used to read or write samples and provides
information about the format of the samples.

## sndfile type

A sndfile may be indexed using the following keys,

- frames  
The count number of sample _frames_ in the file.
- samplerate  
The sample rate of the file's data.
- channels  
The number of channels of data (samples per frame).
- format  
The file's format, see below.
- sections  
The number of sections in the file.
- seekable  
Non-zero if the file may accessed randomly.

## frames type

Reading and writing data uses the `frames` type. A `frames` object
represents some number of _frames_ of sample data, with each frame
containing a 32-bit float sample for each channel.

A `frames` is created with a maximum size and may contain up to size
valid samples. The number of valid samples, the frames' _count_, is
set when reading data from a file and may be set by the user when
writing data.

- size
- count
- samplerate
- channels

### accessing samples

Actual sample values are accessed using _indexing_ a frames object
with the sample offset, an _index_ in the range [0.._size_) where
_size_ is the frame's size.

Negative indices are permitted and indicate an offset from the end of
the frames data, i.e. frames[-1] is the last sample, frames[-2] the
second last and so on (assuming the frame is fully populated).

    
- _float_ = frames[_index_];
- frames[_index_] = `float(`_value_`)`;

## constants

The various constants defined libsndfile to represent formats and
other values are provided by the module with names altered to be more
readable in the ICI context - leading `SF_` prefixes are removed
relying on module qualification for readability. See ici-sndfile.ici

## helper functions

- float = duration(sndfile)  
Returns the duration of a sndfile, the number of seconds of sample
data it contains.

The following functions all take a format code as returned when by
indexing a sndfile object with `format`. Format codes are constructed
by or'ing together values taken from the FORMAT _constants_ defined in
`ici-sndfile.ici`.

- string = describe_type(format)  
Returns a string describing the format's overall type, e.g. "Microsoft WAV".
- string = describe_subtype(format)  
Returns a string describing the so-called _subtype_, e.g. "32-bit float".
- string = describe_endianess(format)  
Returns a string describing the endianess of the data, e.g. "little endian".
- string = describe_format(format)  
Returns a string formed from the above _describe_ functions to
full describe a format, e.g "Apple/SGI AIFF, 24-bit PCM, big endian".
- int = format_type(format)  
Returns format's type code.
- int = format_subtype(format)  
Return the format's subtype code.
- int = format_endianess(format)  
Return the format's endianess code.

