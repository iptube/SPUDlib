#!/bin/bash

./adectest | nodejs /usr/local/lib/node_modules/cbor/bin/cbor2comment -x

#cisco-devnet@ciscodevnet-VirtualBox:/usr/local/lib/node_modules/cbor/bin$ echo "a26669706164647244c0a8000065746f6b656e452a2a2a2a2a" | nodejs cbor2comment  -x
