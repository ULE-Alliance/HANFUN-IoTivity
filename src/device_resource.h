#ifndef _DEVICERESOURCE_H
#define _DEVICERESOURCE_H

#include "hanfun.h"
#include "octypes.h"

OCStackResult SetDeviceProperties(HF::Attributes::List *attributes);
OCStackResult SetDeviceProperties(uint8_t *ipui, uint8_t *emc);

#endif