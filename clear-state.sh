#!/bin/bash
PREFIX=${1:-HanFunBridge_}

# Clear out all the old state
rm -f hanfun.json $PREFIX*.dat $PREFIX*.db $PREFIX*.state

# Create a new SVR DB
UUID=`uuidgen`
sed -e s/UUID/$UUID/g ./oic_svr_db.json_ > $PREFIX\oic_svr_db.json
#export $(cat ./BuildOptions.txt | grep -v ^# | sed -e 's/ = /=/' | xargs)
extlibs/iotivity/out/linux/x86_64/release/resource/csdk/security/tool/json2cbor $PREFIX\oic_svr_db.json $PREFIX\oic_svr_db.dat > /dev/null
