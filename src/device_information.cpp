#include "device_information.h"

#include "hash.h"
#include "log.h"
#include "ocpayload.h"
#include "oic_malloc.h"

DeviceInformation::DeviceInformation()
{

}

void DeviceInformation::set_application_version(const char *app_version)
{
  app_version_ = std::string(app_version);
}

std::string DeviceInformation::application_version()
{
  return app_version_;
}

void DeviceInformation::set_device_uid(const char *uid)
{
  device_uid_ = std::string(uid);
}

std::string DeviceInformation::device_uid()
{
  return device_uid_;
}

void DeviceInformation::set_friendly_name(const char *name)
{
  friendly_name_ = std::string(name);
}

std::string DeviceInformation::friendly_name()
{
  return friendly_name_;
}

void DeviceInformation::set_hardware_version(const char *version)
{
  hardware_version_ = std::string(version);
}

std::string DeviceInformation::hardware_version()
{
  return hardware_version_;
}

void DeviceInformation::set_manufacturer_name(const char *manufacturer)
{
  manufacturer_name_ = std::string(manufacturer);
}

std::string DeviceInformation::manufacturer_name()
{
  return manufacturer_name_;
}

void DeviceInformation::set_serial_number(const char *serial_number)
{
  serial_number_ = std::string(serial_number);
}

std::string DeviceInformation::serial_number()
{
  return serial_number_;
}

void DeviceInformation::Set(OCRepPayload *payload)
{
  char *value = NULL;
  OCRepPayload **arr = NULL;
  size_t dim[MAX_REP_ARRAY_DEPTH] = { 0 };
  if (OCRepPayloadGetPropString(payload, OC_RSRVD_DEVICE_NAME, &value))
  {
    set_friendly_name(value);
    OICFree(value);
    value = NULL;
  }
  if (OCRepPayloadGetPropString(payload, OC_RSRVD_DEVICE_ID, &value))
  {
    set_serial_number(value);
    OICFree(value);
    value = NULL;
  }
  if (OCRepPayloadGetPropObjectArray(payload, OC_RSRVD_DEVICE_MFG_NAME, &arr, dim))
  {
    size_t dim_total = calcDimTotal(dim);
    for (size_t i = 0; i < dim_total; ++i)
    {
      if (i == 0)
      {
        char *language = NULL;
        if (OCRepPayloadGetPropString(arr[i], "language", &language) &&
            OCRepPayloadGetPropString(arr[i], "value", &value))
        {
          set_manufacturer_name(value);
        }
        OICFree(language);
        OICFree(value);
      }
    }
    for (size_t i = 0; i < dim_total; ++i)
    {
      OCRepPayloadDestroy(arr[i]);
    }
    OICFree(arr);
    arr = NULL;
  }
  if (OCRepPayloadGetPropString(payload, OC_RSRVD_DEVICE_MODEL_NUM, &value))
  {

  }
  if (OCRepPayloadGetPropObjectArray(payload, OC_RSRVD_DEVICE_DESCRIPTION, &arr, dim))
  {

  }
  if (OCRepPayloadGetPropString(payload, OC_RSRVD_PROTOCOL_INDEPENDENT_ID, &value))
  {

  }
  if (OCRepPayloadGetPropString(payload, OC_RSRVD_SOFTWARE_VERSION, &value))
  {
    set_application_version(value);
    OICFree(value);
    value = NULL;
  }
  if (OCRepPayloadGetPropString(payload, OC_RSRVD_MFG_DATE, &value))
  {

  }
  if (OCRepPayloadGetPropString(payload, OC_RSRVD_FIRMWARE_VERSION, &value))
  {
    
  }
  if (OCRepPayloadGetPropString(payload, OC_RSRVD_HARDWARE_VERSION, &value))
  {
    set_hardware_version(value);
    OICFree(value);
    value = NULL;
  }
  if (OCRepPayloadGetPropString(payload, OC_RSRVD_MFG_URL, &value))
  {
    
  }
  if (OCRepPayloadGetPropString(payload, OC_RSRVD_OS_VERSION, &value))
  {
    
  }
  if (OCRepPayloadGetPropString(payload, OC_RSRVD_PLATFORM_VERSION, &value))
  {
    
  }
  if (OCRepPayloadGetPropString(payload, OC_RSRVD_SUPPORT_URL, &value))
  {
    
  }
  if (OCRepPayloadGetPropString(payload, OC_RSRVD_PLATFORM_ID, &value))
  {
    set_device_uid(value);
    OICFree(value);
    value = NULL;
  }
  if (OCRepPayloadGetPropString(payload, OC_RSRVD_SYSTEM_TIME, &value))
  {
    
  }
  /*if (OCRepPayloadGetPropString(payload, OC_RSRVD_DEFAULT_LANGUAGE, &value))
  {
    
  }
  if (OCRepPayloadGetPropObjectArray(payload, OC_RSRVD_DEVICE_NAME_LOCALIZED, &arr, dim))
  {
    
  }
  double *loc = NULL;
  if (OCRepPayloadGetDoubleArray(payload, OC_RSRVD_LOCATION, &loc, dim))
  {
    
  }
  if (OCRepPayloadGetPropString(payload, OC_RSRVD_LOCATION_NAME, &value))
  {
    
  }
  if (OCRepPayloadGetPropString(payload, OC_RSRVD_CURRENCY, &value))
  {
    
  }
  if (OCRepPayloadGetPropString(payload, OC_RSRVD_REGION, &value))
  {
    
  }
  if (OCRepPayloadGetPropObjectArray(payload, OC_RSRVD_PLATFORM_NAME, &arr, dim))
  {
    
  }*/
}

bool GetProtocolIndependentId(HF::Attributes::List *attributes, char piid[UUID_STRING_SIZE])
{
  OCUUIdentity id;

  auto uid_attribute = std::find_if(attributes->begin(), attributes->end(), HasAttribute(HF::Core::DeviceInformation::UID_ATTR));

  if (uid_attribute != attributes->end())
  {
    auto *uid = HF::Attributes::adapt<HF::UID::UID>(*uid_attribute);
    switch (uid->get().type())
    {
      case HF::UID::NONE_UID:
        // Do nothing
        break;
      case HF::UID::DECT_UID: // RFPI
        // TODO
        break;
      case HF::UID::MAC_UID:
        // TODO
        break;
      case HF::UID::URI_UID:
        Hash(&id, static_cast<const HF::UID::URI *>(uid->get().raw())->str().c_str());
        break;
    }
  }
  else
  {
    // TODO: Add last-chance option
    LOG(LOG_WARN, "Cannot calculate piid. Aborting translation...");
    return false;
  }

  return OCConvertUuidToString(id.id, piid);
}

bool GetProtocolIndependentId(uint8_t *ipui, uint8_t *emc, char piid[UUID_STRING_SIZE])
{
  OCUUIdentity id;

  if (ipui != NULL && emc != NULL)
  {
    Hash(&id, ipui, emc);
  }
  else
  {
    LOG(LOG_WARN, "Cannot calculate piid. Aborting translation...");
    return false;
  }

  return OCConvertUuidToString(id.id, piid);
}

void GetPlatformId(HF::Attributes::List *attributes, char pi[UUID_STRING_SIZE])
{
  OCUUIdentity id;

  auto uid_attribute = std::find_if(attributes->begin(), attributes->end(), HasAttribute(HF::Core::DeviceInformation::UID_ATTR));

  if (uid_attribute != attributes->end())
  {
    auto *uid = HF::Attributes::adapt<HF::UID::UID>(*uid_attribute);
    switch (uid->get().type())
    {
      case HF::UID::NONE_UID:
        // Do nothing
        break;
      case HF::UID::DECT_UID: // RFPI
        // TODO
        break;
      case HF::UID::MAC_UID:
        // TODO
        break;
      case HF::UID::URI_UID:
        Hash(&id, static_cast<const HF::UID::URI *>(uid->get().raw())->str().c_str());
        break;
    }
  }
  else
  {
    // TODO: Add last-chance option
  }

  OCConvertUuidToString(id.id, pi);
}

void GetPlatformId(uint8_t *ipui, uint8_t *emc, char pi[UUID_STRING_SIZE])
{
  OCUUIdentity id;

  if (ipui != NULL && emc != NULL)
  {
    Hash(&id, ipui, emc);
  }
  else
  {
    LOG(LOG_WARN, "Cannot calculate pi");
  }

  OCConvertUuidToString(id.id, pi);
}

void GetModelNumber(HF::Attributes::List *attributes, std::string &model_number)
{
  auto serial_num_attr = std::find_if(attributes->begin(), attributes->end(), HasAttribute(HF::Core::DeviceInformation::SERIAL_NUMBER_ATTR));

  if (serial_num_attr != attributes->end())
  {
    auto *serial = HF::Attributes::adapt<std::string>(*serial_num_attr);
    model_number = serial->get();
  }
}