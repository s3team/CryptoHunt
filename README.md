# CryptoHunt: A Cryptographic Function Detection Tool

CryptoHunt is a binary analysis tool to detect cryptographic functions in a binary trace. Now we only support 32 bit traces.

## Prerequisites
1. You need to download PIN from Intel. I tested version 2.13 and 3.2, but other versions probably work as well.
2. You need a g++ (above 6.0 verion) installed.

## How to compile and install
1. Compile the tracer: run `make PIN_ROOT=PinDirectory TARGET=ia32 $*` in the `tracer` directory.
2. Compile CryptoHunt: run `make` in the project root directory.

## How to use
1. Use the tracer to record an execution trace.
   `pin -t tracer/obj-ia32/instracelog.so -- yourprogram`
2. Run loop detection on the trace.
   `./loopdetect tracefile`
3. Compare the loop bodies.
   `./llse refloop targetloop`
