#include "virtual_ocf_device.h"

#include "device_resource.h"
#include "log.h"
#include "platform_resource.h"

VirtualOcfDevice::VirtualOcfDevice(uint16_t address)
  : address_(address)
{
  LOG(LOG_DEBUG, "[%p] address=%u", this, address);

  OCResourceHandle handle = OCGetResourceHandleAtUri(OC_RSRVD_DEVICE_URI);
  if (!handle)
  {
    LOG(LOG_ERR, "OCGetResourceHandleAtUri(" OC_RSRVD_DEVICE_URI ") failed");
  }
  OCStackResult result = OCBindResourceTypeToResource(handle, "oic.d.virtual");
  if (result != OC_STACK_OK)
  {
    LOG(LOG_ERR, "OCBindResourceTypeToResource() - %d", result);
  }
}

VirtualOcfDevice::~VirtualOcfDevice()
{
    LOG(LOG_DEBUG, "[%p]", this);
}

OCStackResult VirtualOcfDevice::SetProperties(const std::string &bridge_manufacturer, HF::Attributes::List *attributes)
{
    std::lock_guard<std::mutex> lock(mutex_);
    attributes_ = *attributes;

    OCStackResult result = SetDeviceProperties(attributes);
    if (result == OC_STACK_OK)
    {
      result = SetPlatformProperties(bridge_manufacturer, attributes);
    }
    
    return result;
}

OCStackResult VirtualOcfDevice::SetProperties(uint8_t *ipui, uint8_t *emc)
{
  std::lock_guard<std::mutex> lock(mutex_);

  OCStackResult result = SetDeviceProperties(ipui, emc);
  if (result == OC_STACK_OK)
  {
    result = SetPlatformProperties(ipui, emc);
  }

  return result;
}
