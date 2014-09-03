# BLESSED

[![Build Status](https://travis-ci.org/pauloborges/blessed.svg?branch=devel)](https://travis-ci.org/pauloborges/blessed)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/1390/badge.svg)](https://scan.coverity.com/projects/1390)

**Blessed** is an open source implementation of a Bluetooth Low Energy stack
targeting embedded bare-metal devices. **Blessed** aims to provide a flexible
and customizable software stack, and when necessary, allow access to lower
layers, such as the link layer.

It's also an acronym for Bluetooth Low Energy Software Stack for Embedded
Devices.

## Features

* **GAP Broadcaster role**: non-connectable and scannable advertising are
implemented. Connectable advertising is also implemented, but connection
requests are ignored.
* **GAP Observer role**: passive scanning is implemented.

### Planned features¹

* Active scanning (check [`examples/radio-active-scanning`]
(https://github.com/pauloborges/blessed/tree/devel/examples/radio-active-observer)).
* Link layer device filtering.
* GAP Peripheral role (connection).
* High level API to easily create apps (unfinished draft can be found in
[`include/blessed/bci.h`]
(https://github.com/pauloborges/blessed/blob/devel/include/blessed/bci.h)).

¹ This is not a complete list.

## Supported platforms

Currently, only the [`nRF51822`]
(https://github.com/pauloborges/blessed/tree/devel/platform/nrf51822) SoC from
[Nordic Semiconductor](https://www.nordicsemi.com/) is supported.

## How to compile it

Execute:

    $ make PLATFORM=your-platform <other configuration variables²>

If all's well, this command will generate a static library located in
`build/libblessed.a`. The public headers are located in `include/blessed`.
Check the examples to see how to build and link an application with
`libblessed.a`. Our build system doesn't support `make install`.

² More on this soon.

## Examples

Check the [`examples/README.md`]
(https://github.com/pauloborges/blessed/blob/devel/examples/README.md) file.

## License

**Blessed** is distributed under the MIT license.
