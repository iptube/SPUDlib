[![Build Status](https://travis-ci.org/iptube/SPUDlib.svg?branch=master)](https://travis-ci.org/iptube/SPUDlib) 
[![Coverage Status](https://coveralls.io/repos/iptube/SPUDlib/badge.svg?branch=master)](https://coveralls.io/r/iptube/SPUDlib?branch=master)
# SPUDlib
Session Protocol for User Datagrams (SPUD) Prototype

## Compiling

./bootstrap   (To create the ./configure script)
./configure   (To create the Makefiles)
make          (To build the code)

You will end up with two test binaries in samplecode and a library you can link against in src.

sampelcode/spudecho is a simple echo-server that echoes back all UDP packets.
samplecode/spudtest [iface] [ipaddr] sends SPUD packets to the desired IP from the given interface

## Development

### Unit Tests
Turn on unit tests with:
./configure --with-check

This need the check library installed

Build and run the checks with
make check

If tests fail it can help to run
test/check_spudlib to see where it fails.

### Coding Standard

We like to follow C90. Will make sure the library compile on most platforms.

Braces can be placed where you want them. But try to be consistent..

Whit-spaces when committing should be avoided. (Usual rant on diffs and readability)

