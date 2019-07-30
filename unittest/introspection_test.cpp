#include "unit_test.h"

#include "introspection.h"
#include "ocstack.h"
#include "virtual_hf_device.h"


class Introspection : public HFOCSetUp
{
  public:
    OCResourceHandle handle_;
    DiscoverContext *context_;
    VirtualHfDevice *hf_device_;
    virtual ~Introspection() { }
    virtual void SetUp()
    {
      HFOCSetUp::SetUp();
      EXPECT_EQ(OC_STACK_OK, OCCreateResource(&handle_, "x.org.iotivity.rt", NULL, "/resource",
        NULL, NULL, OC_DISCOVERABLE));

      context_ = new DiscoverContext();
      Callback discover_cb(Discover, context_);
      EXPECT_EQ(OC_STACK_OK, OCDoResource(NULL, OC_REST_DISCOVER, "/oic/res", NULL, 0, CT_DEFAULT,
        OC_HIGH_QOS, discover_cb, NULL, 0));
      EXPECT_EQ(OC_STACK_OK, discover_cb.Wait(1000));

      hf_device_ = VirtualHfDevice::Create(context_->device->di_.c_str(), NULL, false);
    }
    virtual void TearDown()
    {
      delete hf_device_;
      delete context_;
      OCDeleteResource(handle_);
      HFOCSetUp::TearDown();
    }
};

TEST_F(Introspection, TheTranslatorShallPreserveAsMuchOfTheOriginalInformationAsCanBeRepresentedInTheTranslatedFormat)
{
  FAIL();
}
