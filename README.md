# Bus

Implementation of a compile-time and run-time bus and their comparison.

## Description

_main.cpp_ contains simple demonstration of how interconnected components may look like and a time-measurement harness.

## Features

Compile-time bus also contains _request-response_ support which translates message into request from a particular component and which also contains a response callback that is to be called when the request is processed.

## Build

No special libraries are needed. Simply build with

```
g++ -pedantic -Wall -Wextra -O3 main.cpp
```

(or build a CMake project...)

## TODO

So far, the Bus does not support delayed messages as it requires special queue instead of plain `std::dequeue`.
