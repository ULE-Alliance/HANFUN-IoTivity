#include "unit_test.h"

#include "transport.h"

#include "device_resource.h"
#include "hanfun.h"
#include "hash.h"
#include "ocrandom.h"
#include "ocstack.h"

struct ExpectedProperties
{
  // oic.wk.d
  const char *n;
  const char *dmv;
  const char *piid;
  LocalizedString *ld;
  size_t nld;
  const char *sv;
  LocalizedString *dmn;
  size_t ndmn;
  const char *dmno;
};

typedef HF::Devices::Node::Unit0<HF::Core::DeviceInformation::Server,
                                 HF::Core::DeviceManagement::Client,
                                 HF::Core::AttributeReporting::Server,
                                 HF::Core::Time::Server,
                                 HF::Core::BatchProgramManagement::DefaultServer,
                                 HF::Core::Scheduling::Event::DefaultServer,
                                 HF::Core::Scheduling::Weekly::DefaultServer
                                > NodeUnit0;

template <class T>
class HanFunServerBase : public HF::Devices::Node::Abstract<NodeUnit0>, public T
{
  protected:
    Transport transport_;
    virtual ~HanFunServerBase() {}
    virtual void SetUp()
    {
      HFOCSetUp::SetUpStack();
      
      transport_.initialize();
      transport_.uid(new HF::UID::URI("hf://node.example.com/test"));
      transport_.add(this);
    }
    virtual void TearDown()
    {
      transport_.destroy();
      //delete transport_;
      HFOCSetUp::TearDownStack();
    }
    DiscoverContext *DiscoverVirtualResource(const char *path)
    {
      DiscoverContext *context = new DiscoverContext(path);
      Callback discover_callback(Discover, context);
      EXPECT_EQ(OC_STACK_OK, OCDoResource(NULL, OC_REST_DISCOVER, "/oic/res", NULL, 0, CT_DEFAULT, OC_HIGH_QOS, discover_callback, NULL, 0));
      EXPECT_EQ(OC_STACK_OK, discover_callback.Wait(1000));
      return context;
    }
};

class HanFunServer : public HanFunServerBase<::testing::Test>
{
  public:
    virtual ~HanFunServer() {}
};

TEST_F(HanFunServer, ThePiidPropertyValueShallBeDerivedFromTheDeviceUid)
{
  HF::Attributes::List *attributes = new HF::Attributes::List();
  // Hash(DeviceUid) => piid
  const char *device_uid = "hf://node.example.com/1";
  
  OCUUIdentity id;
  Hash(&id, device_uid);
  char piid[UUID_STRING_SIZE];
  OCConvertUuidToString(id.id, piid);

  EXPECT_EQ(OC_STACK_OK, SetDeviceProperties(attributes));

  char *s;
  EXPECT_EQ(OC_STACK_OK, OCGetPropertyValue(PAYLOAD_TYPE_DEVICE, OC_RSRVD_PROTOCOL_INDEPENDENT_ID, (void **) &s));
  EXPECT_STREQ(piid, s);
}