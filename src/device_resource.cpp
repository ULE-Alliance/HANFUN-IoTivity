#include "device_resource.h"

#include "device_information.h"
#include "ocpayload.h"
#include "experimental/ocrandom.h"
#include "ocstack.h"
#include "resource.h"

#include <algorithm>

void GetDeviceName(HF::Attributes::List *attributes, std::string &device_name)
{
  auto name_attribute = std::find_if(attributes->begin(), attributes->end(), HasAttribute(HF::Core::DeviceInformation::FRIENDLY_NAME_ATTR));

  if (name_attribute != attributes->end())
  {
    auto *names_attr = HF::Attributes::adapt<HF::Core::DeviceInformation::FriendlyName>(*name_attribute);
    auto const &names = names_attr->get();

    auto unit_zero = std::find_if(names.units.begin(), names.units.end(), [](const HF::Core::DeviceInformation::FriendlyName::Unit &unit)
    {
      return unit.id == 0;
    });

    if (unit_zero != names.units.end())
    {
      device_name = unit_zero->name;
    }
  }
}

void GetDataModelVersion(HF::Attributes::List *attributes, std::string &data_model_version)
{
  auto core_attribute = std::find_if(attributes->begin(), attributes->end(), HasAttribute(HF::Core::DeviceInformation::CORE_VERSION_ATTR));

  auto profile_attribute = std::find_if(attributes->begin(), attributes->end(), HasAttribute(HF::Core::DeviceInformation::PROFILE_VERSION_ATTR));

  auto interface_attribute = std::find_if(attributes->begin(), attributes->end(), HasAttribute(HF::Core::DeviceInformation::INTERFACE_VERSION_ATTR));

  if (core_attribute != attributes->end() && profile_attribute != attributes->end() && interface_attribute != attributes->end())
  {
    auto *core_version = HF::Attributes::adapt<uint8_t>(*core_attribute);
    auto *profile_version = HF::Attributes::adapt<uint8_t>(*profile_attribute);
    auto *interface_version = HF::Attributes::adapt<uint8_t>(*interface_attribute);

    data_model_version += DEVICE_DATA_MODEL_VERSION;
    data_model_version += ", x.core.";
    data_model_version += std::to_string(core_version->get());
    data_model_version += ", x.profile.";
    data_model_version += std::to_string(profile_version->get());
    data_model_version += ", x.interface.";
    data_model_version += std::to_string(interface_version->get());
  }
}

void GetLocalizedDescription(HF::Attributes::List *attributes, OCStringLL *localized_description)
{
  auto location_attribute = std::find_if(attributes->begin(), attributes->end(), HasAttribute(HF::Core::DeviceInformation::LOCATION_ATTR));

  if (location_attribute != attributes->end())
  {
    auto *location = HF::Attributes::adapt<std::string>(*location_attribute);
    OCResourcePayloadAddStringLL(&localized_description, "en");
    OCResourcePayloadAddStringLL(&localized_description, location->get().c_str());
  }
}

void GetSoftwareVersion(HF::Attributes::List *attributes, std::string &software_version)
{
  auto app_attribute = std::find_if(attributes->begin(), attributes->end(), HasAttribute(HF::Core::DeviceInformation::APP_VERSION_ATTR));

  if (app_attribute != attributes->end())
  {
    auto *version = HF::Attributes::adapt<std::string>(*app_attribute);
    software_version = version->get();
  }
}

void GetManufacturerName(HF::Attributes::List *attributes, OCStringLL *manufacturer_name)
{
  auto manufacturer_attribute = std::find_if(attributes->begin(), attributes->end(), HasAttribute(HF::Core::DeviceInformation::MANUFACTURER_NAME_ATTR));

  if (manufacturer_attribute != attributes->end())
  {
    auto *name = HF::Attributes::adapt<std::string>(*manufacturer_attribute);
    OCResourcePayloadAddStringLL(&manufacturer_name, "en");
    OCResourcePayloadAddStringLL(&manufacturer_name, name->get().c_str());
  }
}

OCStackResult SetDeviceProperties(HF::Attributes::List *attributes)
{
  assert(attributes);

  OCStackResult result = OC_STACK_OK;
  std::string device_name;
  GetDeviceName(attributes, device_name);
  if (!device_name.empty())
  {
    result = OCSetPropertyValue(PAYLOAD_TYPE_DEVICE, OC_RSRVD_DEVICE_NAME, device_name.c_str());
    if (result != OC_STACK_OK)
    {
      return result;
    }
  }

  result = OCSetPropertyValue(PAYLOAD_TYPE_DEVICE, OC_RSRVD_SPEC_VERSION, OC_SPEC_VERSION);
  if (result != OC_STACK_OK)
  {
    return result;
  }

  /* OC_RSRVD_DEVICE_ID is set internally by stack */

  char piid[UUID_STRING_SIZE] = { 0 };
  GetProtocolIndependentId(attributes, piid);
  result = OCSetPropertyValue(PAYLOAD_TYPE_DEVICE, OC_RSRVD_PROTOCOL_INDEPENDENT_ID, piid);
  if (result != OC_STACK_OK)
  {
    return result;
  }

  std::string data_model_version;
  GetDataModelVersion(attributes, data_model_version);
  result = OCSetPropertyValue(PAYLOAD_TYPE_DEVICE, OC_RSRVD_DATA_MODEL_VERSION, data_model_version.c_str());
  if (result != OC_STACK_OK)
  {
    return result;
  }

  OCStringLL *ll = NULL;
  GetLocalizedDescription(attributes, ll);
  if (ll)
  {
    result = OCSetPropertyValue(PAYLOAD_TYPE_DEVICE, OC_RSRVD_DEVICE_DESCRIPTION, ll);
    OCFreeOCStringLL(ll);
    ll = NULL;
    if (result != OC_STACK_OK)
    {
      return result;
    }
  }
  
  std::string software_version;
  GetSoftwareVersion(attributes, software_version);
  if (!software_version.empty())
  {
    result = OCSetPropertyValue(PAYLOAD_TYPE_DEVICE, OC_RSRVD_SOFTWARE_VERSION, software_version.c_str());
    if (result != OC_STACK_OK)
    {
      return result;
    }
  }
  
  GetManufacturerName(attributes, ll);
  if (ll)
  {
    result = OCSetPropertyValue(PAYLOAD_TYPE_DEVICE, OC_RSRVD_DEVICE_MFG_NAME, ll);
    OCFreeOCStringLL(ll);
    ll = NULL;
    if (result != OC_STACK_OK)
    {
      return result;
    }
  }
  
  std::string model_number;
  GetModelNumber(attributes, model_number);
  if (!model_number.empty())
  {
    result = OCSetPropertyValue(PAYLOAD_TYPE_DEVICE, OC_RSRVD_DEVICE_MODEL_NUM, model_number.c_str());
  }

  return result;
}

OCStackResult SetDeviceProperties(uint8_t *ipui, uint8_t *emc)
{
  OCStackResult result = OC_STACK_OK;
  std::string device_name("sample");
  if (!device_name.empty())
  {
    result = OCSetPropertyValue(PAYLOAD_TYPE_DEVICE, OC_RSRVD_DEVICE_NAME, device_name.c_str());
    if (result != OC_STACK_OK)
    {
      return result;
    }
  }

  result = OCSetPropertyValue(PAYLOAD_TYPE_DEVICE, OC_RSRVD_SPEC_VERSION, OC_SPEC_VERSION);
  if (result != OC_STACK_OK)
  {
    return result;
  }

  /* OC_RSRVD_DEVICE_ID is set internally by stack */

  char piid[UUID_STRING_SIZE] = { 0 };
  GetProtocolIndependentId(ipui, emc, piid);
  result = OCSetPropertyValue(PAYLOAD_TYPE_DEVICE, OC_RSRVD_PROTOCOL_INDEPENDENT_ID, piid);
  if (result != OC_STACK_OK)
  {
    return result;
  }

  std::string data_model_version("1");
  result = OCSetPropertyValue(PAYLOAD_TYPE_DEVICE, OC_RSRVD_DATA_MODEL_VERSION, data_model_version.c_str());
  if (result != OC_STACK_OK)
  {
    return result;
  }

  return result;
}
