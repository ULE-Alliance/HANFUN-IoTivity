#include "unit_test.h"

class OCFResource : public HFOCSetUp
{
  public:
    virtual ~OCFResource() { }
};

TEST_F(OCFResource, EachOCFResourceShallBeMappedToASeparateAllJoynObject)
{
  FAIL();
}

TEST_F(OCFResource, EachOCFServerShallBeExposedAsASeparateAllJoynProducerApplicationWithItsOwnAboutData)
{
  // TODO: This cannot be unit tested yet
  FAIL();
}

TEST_F(OCFResource, TheHanFunServerDeviceShallImplementTheOicDVirtualHanFunInterface)
{
  FAIL();
}