#ifndef _VIRTUALDEVICE_H
#define _VIRTUALDEVICE_H

#include "hanfun.h"

#include "ocstack.h"
#include <mutex>

class VirtualOcfDevice
{
  public:
    VirtualOcfDevice(uint16_t address);
    ~VirtualOcfDevice();

    OCStackResult SetProperties(const std::string &bridge_manufacturer, HF::Attributes::List *attributes);
    OCStackResult SetProperties(uint8_t *ipui, uint8_t *emc);

    uint16_t address() const { return address_; }

  private:
    std::mutex mutex_;
    uint16_t address_;
    HF::Attributes::List attributes_;
};

#endif