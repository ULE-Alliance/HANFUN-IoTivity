#ifndef _PLATFORMRESOURCE_H
#define _PLATFORMRESOURCE_H

#include "hanfun.h"
#include "octypes.h"

OCStackResult SetPlatformProperties(const std::string &bridge_manufacturer, HF::Attributes::List *attributes);
OCStackResult SetPlatformProperties(uint8_t *ipui, uint8_t *emc);

#endif