# HAN-FUN / IoTivity Adaptor

## Overview

## Project Setup

### Submodules

In order to download the submodules associated with this project, run the following commands in the root of the project:

```
git submodule init
git submodule update
```

#### IoTivity

* Dependencies

  * gcc (>= 5.0)
  * scons

#### HAN-FUN

The HAN-FUN project is implemented in C++ and depends only on the standard C++ library for usage.

* Dependencies

  * gcc (>= 4.6)
  * cmake (>= 2.8.11)

Ubuntu 16.04 installs by default gcc-5. In order to install and configure gcc-4.9, run the following commands:

```
sudo apt-get install gcc-4.9 g++-4.9
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-5 100 --slave /usr/bin/g++ g++ /usr/bin/g++-5
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.9 50 --slave /usr/bing/g++ g++ /usr/bin/g++-4.9
```

To switch between gcc versions, use:

```
sudo update-alternatives --config gcc
```
#### libuv

## Build

### Submodules

The project includes a shell script for submodules release building. The instructions are the following:

```
cd extlibs
chmod +x prep.sh
./prep.sh
```

Instructions for specific submodule building are written below.

#### IoTivity

IoTivity library must be built with the resource directory enabled.

The building instructions are the following:

```
cd extlibs/iotivity
scons RELEASE=true RD_MODE=CLIENT,SERVER
```

#### HAN-FUN

HAN-FUN library must be built with the __HF_SHARED_SUPPORT__ option in order to be linked into a shared library. This option will remove the flag '-fno-rtti' and add the '-fPIC' flag from the compilations flags.

The building instructions are the following:

```
cd extlibs/hanfun
mkdir build
cd build
cmake -DHF_SHARED_SUPPORT=ON ..
make
```

#### libuv

```
sh autogen.sh
./configure
make
```

## Testing

## Usage
> **Linux Only** You will need to copy the setenv.sh.example to setenv.sh then setup
> the setenv.sh file to reflect your environment. You can then source the setenv.sh
> to setup the environment.
>
> ```
> cp setenv.sh.example setenv.sh
> vi setenv.sh
> source setenv.sh
> ```

A CMBS kit is required as an independent process that acts as Base Station and uses DECT stack. The HAN-FUN to IoTivity Adaptor communicates with it via UDP/IP and listens to the HAN-FUN network. The CMBS binaries and documentation can be downloaded at https://github.com/DSPGroup/ule-starterkit/releases/download/v1.0/ULE-StarterKit-v1.0.zip.

CMBS supports the following platforms:
1. Windows: $UNZIP_DIRECTORY/base/tools/cmbs_tcx.exe
2. Linux (32 bit): $UNZIP_DIRECTORY/base/tools/cmbs_tcx.linux-arm
3. Linux (64 bit): $UNZIP_DIRECTORY/base/tools/cmbs_tcx.linux-amd64

> **Linux Only** To execute it, run a terminal and:
> ```
> cd /path/to/base/tools
> ./cmbs_tcx -usb -com 0 -han
> ```
> this will look for /dev/ttyACM0, use -com 1 for /dev/ttyACM1

**Only HAN-FUN virtualization is implemented in Phase 1.**

Currently the bridge requires multiple processes to manage multiple instances of IoTivity (one per bridged HF device). Under Linux a helper application is provided to manage the processes. The bridge may be run as follows:

```
.out/linux/x86_64/debug/bin/PluginManager ./out/linux/x86_64/debug/bin/HanFunBridge --hf
```

