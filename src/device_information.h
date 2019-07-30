#ifndef _DEVICEINFORMATION_H
#define _DEVICEINFORMATION_H

#include "hanfun.h"
#include "experimental/ocrandom.h"
#include "octypes.h"
#include <string>

class DeviceInformation
{
  public:
    DeviceInformation();
    void Set(OCRepPayload *payload);

    void set_application_version(const char *version);
    std::string application_version();
    void set_device_uid(const char *uid);
    std::string device_uid();
    void set_friendly_name(const char *name);
    std::string friendly_name();
    void set_hardware_version(const char *version);
    std::string hardware_version();
    void set_manufacturer_name(const char *manufacturer);
    std::string manufacturer_name();
    void set_serial_number(const char *serial_number);
    std::string serial_number();

  private:
    std::string app_version_;
    std::string device_uid_;
    std::string friendly_name_;
    std::string hardware_version_;
    std::string manufacturer_name_;
    std::string serial_number_;
};

class HasAttribute
{
  public:
    HasAttribute(HF::Core::DeviceInformation::Attributes attribute) : attribute_(attribute) {}
    bool operator()(HF::Attributes::IAttribute *attribute) const
    {
      return attribute->uid() == attribute_;
    }

  private:
    HF::Core::DeviceInformation::Attributes attribute_;
};

bool GetProtocolIndependentId(HF::Attributes::List *attributes, char piid[UUID_STRING_SIZE]);
bool GetProtocolIndependentId(uint8_t *ipui, uint8_t *emc, char piid[UUID_STRING_SIZE]);
void GetPlatformId(HF::Attributes::List *attributes, char pi[UUID_STRING_SIZE]);
void GetPlatformId(uint8_t *ipui, uint8_t *emc, char pi[UUID_STRING_SIZE]);
void GetModelNumber(HF::Attributes::List *attributes, std::string &model_number);

#endif
