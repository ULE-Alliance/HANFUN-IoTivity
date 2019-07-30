#include "unit_test.h"

#include "device_information.h"
#include "oic_malloc.h"

class DeviceInformationTest : public HFOCSetUp
{
  public:
    virtual ~DeviceInformationTest() { }
};

TEST_F(DeviceInformationTest, IsValid)
{
  FAIL();
}

TEST_F(DeviceInformationTest, SetFieldsFromDevice)
{
  OCRepPayload *payload = OCRepPayloadCreate();
  // di => Serial Number
  const char *di = "7d529297-6f9f-83e8-aec0-72dd5392b584";
  EXPECT_TRUE(OCRepPayloadSetPropString(payload, OC_RSRVD_DEVICE_ID, di));
  // n => Friendly Name
  const char *name = "name";
  EXPECT_TRUE(OCRepPayloadSetPropString(payload, OC_RSRVD_DEVICE_NAME, name));
  // d.dmn => Manufacturer
  const char *en_manufacturer = "en-manufacturer";
  LocalizedString manufacturers[] = {
    { "en", en_manufacturer },
    { "es", "es-manufacturer" }
  };
  size_t dmn_dim[MAX_REP_ARRAY_DEPTH] = { sizeof(manufacturers) / sizeof(manufacturers[0]), 0, 0 };
  size_t dim_total = calcDimTotal(dmn_dim);
  OCRepPayload **dmn = (OCRepPayload **) OICCalloc(dim_total, sizeof(OCRepPayload *));
  for (size_t i = 0; i < dim_total; ++i)
  {
    dmn[i] = OCRepPayloadCreate();
    EXPECT_TRUE(OCRepPayloadSetPropString(dmn[i], "language", manufacturers[i].language));
    EXPECT_TRUE(OCRepPayloadSetPropString(dmn[i], "value", manufacturers[i].value));
  }
  EXPECT_TRUE(OCRepPayloadSetPropObjectArrayAsOwner(payload, OC_RSRVD_DEVICE_MFG_NAME, dmn, dmn_dim));
  // d.sv => Application Version
  const char *software_version = "software-version";
  EXPECT_TRUE(OCRepPayloadSetPropString(payload, OC_RSRVD_SOFTWARE_VERSION, software_version));

  DeviceInformation device_information;
  device_information.Set(payload);

  EXPECT_STREQ(di, device_information.serial_number().c_str());
  EXPECT_STREQ(software_version, device_information.application_version().c_str());
  EXPECT_STREQ(en_manufacturer, device_information.manufacturer_name().c_str());
  EXPECT_STREQ(name, device_information.friendly_name().c_str());

  OCRepPayloadDestroy(payload);
}

TEST_F(DeviceInformationTest, SetFieldsFromPlatform)
{
  OCRepPayload *payload = OCRepPayloadCreate();
  // p.mnhw => Hardware Version
  const char *hardware_version = "hardware-version";
  EXPECT_TRUE(OCRepPayloadSetPropString(payload, OC_RSRVD_HARDWARE_VERSION, hardware_version));
  // p.pi => Device UID
  const char *pi = "c208d3b0-169b-4ace-bf5a-54ad2d6549f7";
  EXPECT_TRUE(OCRepPayloadSetPropString(payload, OC_RSRVD_PLATFORM_ID, pi));

  DeviceInformation device_information;
  device_information.Set(payload);

  EXPECT_STREQ(hardware_version, device_information.hardware_version().c_str());
  EXPECT_STREQ(pi, device_information.device_uid().c_str());

  OCRepPayloadDestroy(payload);
}

TEST_F(DeviceInformationTest, SetFieldsFromDeviceConfiguration)
{
  FAIL();
}

TEST_F(DeviceInformationTest, SetFieldsFromPlatformConfiguration)
{
  FAIL();
}

TEST_F(DeviceInformationTest, UseNameWhenLocalizedNamesNotPresent)
{
  FAIL();
}

TEST_F(DeviceInformationTest, UseLocalizedNamesWhenNamePresent)
{
  FAIL();
}

class DeviceProperties : public HFOCSetUp
{
  public:
    virtual ~DeviceProperties() { }
};

TEST_F(DeviceProperties, SetFromDeviceInformation)
{
  FAIL();
}

class PlatformProperties : public HFOCSetUp
{
  public:
    virtual ~PlatformProperties() { }
};

TEST_F(PlatformProperties, SetFromDeviceInformation)
{
  FAIL();
}

TEST_F(PlatformProperties, UseHashForPlatformId)
{
  FAIL();
}

class DeviceConfigurationProperties : public HFOCSetUp
{
  public:
    virtual ~DeviceConfigurationProperties() { }
};

TEST_F(DeviceConfigurationProperties, SetFromDeviceInformation)
{
  FAIL();
};

class PlatformConfigurationProperties : public HFOCSetUp
{
  public:
    virtual ~PlatformConfigurationProperties() { }
};

TEST_F(PlatformConfigurationProperties, SetFromDeviceInformation)
{
  FAIL();
}