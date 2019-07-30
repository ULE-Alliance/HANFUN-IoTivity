# build hanfun
cd hanfun
  git apply ../hanfun.patch
if [ ! -d "build" ] ; then
  mkdir -p build
fi
cd build
cmake -DHF_SHARED_SUPPORT=ON -DHF_NODE_APP=ON -DHF_TIME_SUPPORT=ON \
      -DHF_BATCH_PROGRAM_SUPPORT=ON -DHF_EVENT_SCHEDULING_SUPPORT=ON \
      -DHF_WEEKLY_SCHEDULING_SUPPORT=ON ..
make

# build libuv
cd ../../libuv
sh autogen.sh
./configure
make

# build iotivity
cd ../iotivity
if [ ! -d "extlibs/tinycbor/tinycbor" ] ; then
  git clone https://github.com/01org/tinycbor.git extlibs/tinycbor/tinycbor -b v0.4.1
fi
if [ ! -d "extlibs/mbedtls/mbedtls" ] ; then
  git clone https://github.com/ARMmbed/mbedtls.git extlibs/mbedtls/mbedtls -b mbedtls-2.4.2
fi
scons RD_MODE=CLIENT,SERVER
