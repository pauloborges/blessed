# Project blessed

**blessed** is a open source implementation of a Bluetooth Low Energy stack targeting embedded bare-metal devices.

## What "blessed" means?

**blessed** in an acronym for Bluetooth Low Energy Software Stack for Embedded Devices.

## Motivation

In addition to software licensing, proprietary stacks always create a abstractions, and restricts the access to the platform internals. blessed aims to provide a flexible and customizable software stack, and when necessary, allowing access to low layers (Radio and Link Layer).

## Features

**blessed** currently supports GAP Broadcaster role. More features and roles are on the roadmap.

## Platforms

Currently, only nRF51822 platform is supported. Support for **Ubertooth One** and other platforms are on the roadmap.

Check `platform/nrf51822/README.md` for specific instructions for the nRF51822 port.

## Building

To build blessed, just:

    $ make PLATFORM=my-platform

This will generate a static library `libblessed.a` in the `build` directory. The public headers are located in `include/blessed`.

If you always build to the same platform, you can:

    $ echo "PLATFORM=my-platform" > Makefile.config
    $ make

The `Makefile.config` file is included by `Makefile`.

### Configuration

You can configure your build by passing configuration variables in the make command:

    $ make MY-CONFIG=my-config-value

The configuration variables are:

* `CONFIG_LOG_ENABLE`: enable/disable log and error messages
    * `=1` enable (default value)
    * `=0` disable

## Example

There are several examples in `examples` directory showing how to use blessed. Check `examples/README.md` to learn how to build and install them.
