if [ ! -d "extlibs/ule-starterkit" ] ; then
  mkdir extlibs/ule-starterkit
  wget https://github.com/DSPGroup/ule-starterkit/releases/download/v1.0/ULE-StarterKit-v1.0.zip -O extlibs/ule-starterkit/ule-starterkit.zip
  unzip extlibs/ule-starterkit/ule-starterkit.zip -d extlibs/ule-starterkit
fi

mkdir package
mkdir package/bin
mkdir package/lib

cp extlibs/ule-starterkit/base/tools/cmbs_tcx.linux-arm package/bin/cmbs_tcx
cp extlibs/iotivity/out/linux/x86_64/release/libconnectivity_abstraction.so package/lib
cp extlibs/iotivity/out/linux/x86_64/release/libocpmapi.so package/lib
cp extlibs/iotivity/out/linux/x86_64/release/liboc.so package/lib
cp extlibs/iotivity/out/linux/x86_64/release/liboc_logger.so package/lib
cp extlibs/iotivity/out/linux/x86_64/release/liboctbstack.so package/lib
cp extlibs/iotivity/out/linux/x86_64/release/libresource_directory.so package/lib
cp extlibs/iotivity/out/linux/x86_64/release/resource/csdk/security/tool/json2cbor package/bin
cp extlibs/libuv/.libs/libuv.so package/lib
cp extlibs/libuv/.libs/libuv.so.1 package/lib
cp extlibs/libuv/.libs/libuv.so.1.0.0 package/lib
cp oic_svr_db.json_ package/oic_svr_db.json_
cp out/linux/x86_64/release/bin/HanFunBridge package/bin
cp out/linux/x86_64/release/bin/PluginManager package/bin
cp out/linux/x86_64/release/bin/OCClient package/bin
cp scripts/package/clear-state.sh.package package/clear-state.sh
cp scripts/package/setenv.sh.package package/setenv.sh
cp scripts/package/INSTRUCTIONS.md package/INSTRUCTIONS.md

cd package
7z a iotivity-hanfun-bridge.7z *
cp iotivity-hanfun-bridge.7z ../iotivity-hanfun-bridge.7z
cd ..
rm -rf package