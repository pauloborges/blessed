# nRF51822 platform

**blessed** supports nRF51822 based boards.

## Dependencies

### Segger's JLink software

Download the version `4.80` [here](http://www.segger.com/jlink-software.html) (you will need to provide a serial key). It is distributed in packages (DEB and RPM) and tarball. If you installed the tarball, make sure to put `JLinkExe` in your `PATH`.

### nrftool

`nrftool` is a tool built around `JLinkExe` to facilitate the flash/erase process. You can install it using `pip`:

    $ [sudo] pip install nrftool

Or download it [here](https://pypi.python.org/pypi/nrftool).

### Nordic NRF51 SDK

This port uses some files of the SDK, but we can't distribute them because of their license. Download the version `4.4.x` [here](https://www.nordicsemi.com/eng/nordic/Products/nRF51822-Development-Kit/nRF518-SDK/23275) (you will need a Nordic account with a valid product key).

The `4.4.x` branch is only distributed in Microsoft installer form (msi extension). If you are in a non-Windows system, you can:

* Install using [Wine](http://www.winehq.org/)
* Use a Windows system and copy the files

## Building

You have to provide a couple of variables to the build system in order to build a `nRF51822` **blessed** port. Those variables can be defined in the make command line or inside the `Makefile.config` file. `PLATFORM` must be defined to `nrf51822` and `NRF51_SDK_PATH` must points to the directory that contains `nrf51822` folder.

Other optional variables:

* `SOC` defaults to `NRF51822_QFAA_CA`
* `SOC_VARIANT` defalts to `xxaa`
* `BOARD` defaults to `BOARD_PCA10001`
* `HEAP_SIZE` defaults to `0`
* `STACK_SIZE` defaults to `1024`

## Flashing

If you have installed both `JLink software` and `nrftool`, just:

    $ make install
