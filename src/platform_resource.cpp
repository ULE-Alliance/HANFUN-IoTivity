#include "platform_resource.h"

#include "device_information.h"
#include "experimental/ocrandom.h"
#include "ocstack.h"

void GetManufacturerName(const std::string &bridge_manufacturer, HF::Attributes::List *attributes, std::string &manufacturer_name)
{
  auto manufacturer_name_attr = std::find_if(attributes->begin(), attributes->end(), HasAttribute(HF::Core::DeviceInformation::MANUFACTURER_NAME_ATTR));

  if (manufacturer_name_attr != attributes->end())
  {
    auto *name = HF::Attributes::adapt<std::string>(*manufacturer_name_attr);
    manufacturer_name = name->get();
  }
  else
  {
    manufacturer_name = bridge_manufacturer;
  }
}

void GetHardwareVersion(HF::Attributes::List *attributes, std::string &hardware_version)
{
  auto hardware_attr = std::find_if(attributes->begin(), attributes->end(), HasAttribute(HF::Core::DeviceInformation::HW_VERSION_ATTR));

  if (hardware_attr != attributes->end())
  {
    auto *version = HF::Attributes::adapt<std::string>(*hardware_attr);
    hardware_version = version->get();
  }
}

void GetVendorId(HF::Attributes::List *attributes, std::string &vendor_id)
{
  auto emc_attr = std::find_if(attributes->begin(), attributes->end(), HasAttribute(HF::Core::DeviceInformation::EMC_ATTR));

  if (emc_attr != attributes->end())
  {
    auto *emc = HF::Attributes::adapt<uint16_t>(*emc_attr);
    vendor_id = emc->get() != 0 ? std::to_string(emc->get()) : "";
  }
}

OCStackResult SetPlatformProperties(const std::string &bridge_manufacturer, HF::Attributes::List *attributes)
{
  assert(attributes);

  OCStackResult result = OC_STACK_OK;
  char pi[UUID_STRING_SIZE] = { 0 };
  GetPlatformId(attributes, pi);
  result = OCSetPropertyValue(PAYLOAD_TYPE_PLATFORM, OC_RSRVD_PLATFORM_ID, pi);
  if (result != OC_STACK_OK)
  {
    return result;
  }
  
  std::string manufacturer_name;
  GetManufacturerName(bridge_manufacturer, attributes, manufacturer_name);
  result = OCSetPropertyValue(PAYLOAD_TYPE_PLATFORM, OC_RSRVD_MFG_NAME, manufacturer_name.c_str());
  if (result != OC_STACK_OK)
  {
    return result;
  }

  // Manufacturer Details Link (mnml) has no HF match

  std::string model_number;
  GetModelNumber(attributes, model_number);
  if (!model_number.empty())
  {
    result = OCSetPropertyValue(PAYLOAD_TYPE_PLATFORM, OC_RSRVD_MODEL_NUM, model_number.c_str());
    if (result != OC_STACK_OK)
    {
      return result;
    }
  }

  // Date of Manufacture (mndt) has no HF match

  // Platform Version (mnpv) has no HF match

  // OS Version (mnos) has no HF match

  std::string hardware_version;
  GetHardwareVersion(attributes, hardware_version);
  if (!hardware_version.empty())
  {
    result = OCSetPropertyValue(PAYLOAD_TYPE_PLATFORM, OC_RSRVD_HARDWARE_VERSION, hardware_version.c_str());
    if (result != OC_STACK_OK)
    {
      return result;
    }
  }

  // Firmware Version (mnfv) has no HF match

  // Support URL (mnsl) has no HF match

  // System Time (st) has no HF match
  
  std::string vendor_id;
  GetVendorId(attributes, vendor_id);
  if (!vendor_id.empty())
  {
    result = OCSetPropertyValue(PAYLOAD_TYPE_PLATFORM, OC_RSRVD_VID, vendor_id.c_str());
  }

  return result;
}

OCStackResult SetPlatformProperties(uint8_t *ipui, uint8_t *emc)
{
  OCStackResult result = OC_STACK_OK;
  char pi[UUID_STRING_SIZE] = { 0 };
  GetPlatformId(ipui, emc, pi);
  result = OCSetPropertyValue(PAYLOAD_TYPE_PLATFORM, OC_RSRVD_PLATFORM_ID, pi);
  if (result != OC_STACK_OK)
  {
    return result;
  }

  return result;
}