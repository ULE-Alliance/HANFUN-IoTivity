#include "unit_test.h"

#include "secure_mode_resource.h"

#include "ocpayload.h"
#include "ocstack.h"

class SecureMode : public HFOCSetUp
{
  public:
    virtual ~SecureMode() {}
    virtual void SetUp()
    {
      HFOCSetUp::SetUp();
      secure_mode_ = new SecureModeResource(mutex_, true);
      EXPECT_EQ(OC_STACK_OK, secure_mode_->Create());
    }
    virtual void TearDown()
    {
      delete secure_mode_;
      HFOCSetUp::TearDown();
    }
    DiscoverContext *DiscoverResource()
    {
      DiscoverContext *context = new DiscoverContext(OC_RSRVD_SECURE_MODE_URI);
      Callback discover_cb(Discover, context);
      EXPECT_EQ(OC_STACK_OK, OCDoResource(NULL, OC_REST_DISCOVER, "/oic/res", NULL, 0,
        CT_DEFAULT, OC_HIGH_QOS, discover_cb, NULL, 0));
      EXPECT_EQ(OC_STACK_OK, discover_cb.Wait(1000));
      return context;
    }
  
  private:
    std::mutex mutex_;
    SecureModeResource *secure_mode_;
};

TEST_F(SecureMode, WhenTrueAnyBridgedServerThatCannotBeCommunicatedWithSecurelyShallNotHaveACorrespondingVirtualOCFServer)
{
  FAIL();
}

TEST_F(SecureMode, WhenTrueAnyBridgedClientThatCannotBeCommunicatedWithSecurelyShallNotHaveACorrespondingVirtualOCFClient)
{
  FAIL();
}

TEST_F(SecureMode, WhenFalseAnyBridgedServerCanHaveACorrespondingVirtualOCFServer)
{
  FAIL();
}

TEST_F(SecureMode, WhenFalseAnyBridgedClientCanHaveACorrespondingVirtualOCFClient)
{
  FAIL();
}

static void VerifyBaselinePayload(OCRepPayload *payload, bool secure_mode)
{
  EXPECT_STREQ("oic.r.securemode", payload->types->value);
  EXPECT_TRUE(payload->types->next == NULL);
  EXPECT_STREQ("oic.if.baseline", payload->interfaces->value);
  EXPECT_STREQ("oic.if.rw", payload->interfaces->next->value);
  EXPECT_TRUE(payload->interfaces->next->next == NULL);
  bool mode;
  EXPECT_TRUE(OCRepPayloadGetPropBool(payload, "secureMode", &mode));
  EXPECT_EQ(secure_mode, mode);
}

TEST_F(SecureMode, GetBaseLine)
{
  DiscoverContext *context = DiscoverResource();

  ResourceCallback get_cb;
  EXPECT_EQ(OC_STACK_OK, OCDoResource(NULL, OC_REST_GET, "/securemode?if=oic.if.baseline",
    &context->resource->addrs_[0], 0, CT_DEFAULT, OC_HIGH_QOS, get_cb, NULL, 0));
  EXPECT_EQ(OC_STACK_OK, get_cb.Wait(1000));

  EXPECT_EQ(OC_STACK_OK, get_cb.response_->result);
  EXPECT_TRUE(get_cb.response_->payload != NULL);
  EXPECT_EQ(PAYLOAD_TYPE_REPRESENTATION, get_cb.response_->payload->type);
  OCRepPayload *payload = (OCRepPayload *) get_cb.response_->payload;
  VerifyBaselinePayload(payload, true);

  delete context;
}

static void VerifyPayload(OCRepPayload *payload, bool secure_mode)
{
  EXPECT_TRUE(payload->types == NULL);
  EXPECT_TRUE(payload->interfaces == NULL);
  bool mode;
  EXPECT_TRUE(OCRepPayloadGetPropBool(payload, "secureMode", &mode));
  EXPECT_EQ(secure_mode, mode);
}

TEST_F(SecureMode, GetRW)
{
  DiscoverContext *context = DiscoverResource();

  ResourceCallback get_cb;
  EXPECT_EQ(OC_STACK_OK, OCDoResource(NULL, OC_REST_GET, "/securemode?if=oic.if.rw",
    &context->resource->addrs_[0], 0, CT_DEFAULT, OC_HIGH_QOS, get_cb, NULL, 0));
  EXPECT_EQ(OC_STACK_OK, get_cb.Wait(1000));

  EXPECT_EQ(OC_STACK_OK, get_cb.response_->result);
  EXPECT_TRUE(get_cb.response_->payload != NULL);
  EXPECT_EQ(PAYLOAD_TYPE_REPRESENTATION, get_cb.response_->payload->type);
  OCRepPayload *payload = (OCRepPayload *) get_cb.response_->payload;
  VerifyPayload(payload, true);

  delete context;
}

TEST_F(SecureMode, Get)
{
  DiscoverContext *context = DiscoverResource();

  ResourceCallback get_cb;
  EXPECT_EQ(OC_STACK_OK, OCDoResource(NULL, OC_REST_GET, OC_RSRVD_SECURE_MODE_URI,
    &context->resource->addrs_[0], 0, CT_DEFAULT, OC_HIGH_QOS, get_cb, NULL, 0));
  EXPECT_EQ(OC_STACK_OK, get_cb.Wait(1000));

  EXPECT_EQ(OC_STACK_OK, get_cb.response_->result);
  EXPECT_TRUE(get_cb.response_->payload != NULL);
  EXPECT_EQ(PAYLOAD_TYPE_REPRESENTATION, get_cb.response_->payload->type);
  OCRepPayload *payload = (OCRepPayload *) get_cb.response_->payload;
  VerifyPayload(payload, true);

  delete context;
}

static void Post(const char *uri, const OCDevAddr *addr, ResourceCallback *cb)
{
  OCRepPayload *request = OCRepPayloadCreate();
  EXPECT_TRUE(OCRepPayloadSetPropBool(request, "secureMode", false));
  EXPECT_EQ(OC_STACK_OK, OCDoResource(NULL, OC_REST_POST, uri, addr, (OCPayload *) request,
    CT_DEFAULT, OC_HIGH_QOS, *cb, NULL, 0));
  EXPECT_EQ(OC_STACK_OK, cb->Wait(1000));
}

TEST_F(SecureMode, PostBaseline)
{
  DiscoverContext *context = DiscoverResource();

  ResourceCallback post_cb;
  Post("/securemode?if=oic.if.baseline", &context->resource->addrs_[0], &post_cb);

  EXPECT_EQ(OC_STACK_RESOURCE_CHANGED, post_cb.response_->result);
  EXPECT_TRUE(post_cb.response_->payload != NULL);
  EXPECT_EQ(PAYLOAD_TYPE_REPRESENTATION, post_cb.response_->payload->type);
  OCRepPayload *payload = (OCRepPayload *) post_cb.response_->payload;
  VerifyBaselinePayload(payload, false);

  delete context;
}

TEST_F(SecureMode, PostRW)
{
  DiscoverContext *context = DiscoverResource();

  ResourceCallback post_cb;
  Post("/securemode?if=oic.if.rw", &context->resource->addrs_[0], &post_cb);

  EXPECT_EQ(OC_STACK_RESOURCE_CHANGED, post_cb.response_->result);
  EXPECT_TRUE(post_cb.response_->payload != NULL);
  EXPECT_EQ(PAYLOAD_TYPE_REPRESENTATION, post_cb.response_->payload->type);
  OCRepPayload *payload = (OCRepPayload *) post_cb.response_->payload;
  VerifyPayload(payload, false);

  delete context;
}

TEST_F(SecureMode, Post)
{
  DiscoverContext *context = DiscoverResource();

  ResourceCallback post_cb;
  Post(OC_RSRVD_SECURE_MODE_URI, &context->resource->addrs_[0], &post_cb);

  EXPECT_EQ(OC_STACK_RESOURCE_CHANGED, post_cb.response_->result);
  EXPECT_TRUE(post_cb.response_->payload != NULL);
  EXPECT_EQ(PAYLOAD_TYPE_REPRESENTATION, post_cb.response_->payload->type);
  OCRepPayload *payload = (OCRepPayload *) post_cb.response_->payload;
  VerifyPayload(payload, false);

  delete context;
}

TEST_F(SecureMode, Observe)
{
  DiscoverContext *context = DiscoverResource();

  ObserveCallback observe_cb;
  EXPECT_EQ(OC_STACK_OK, OCDoResource(NULL, OC_REST_OBSERVE, OC_RSRVD_SECURE_MODE_URI,
    &context->resource->addrs_[0], 0, CT_DEFAULT, OC_HIGH_QOS, observe_cb, NULL, 0));
  EXPECT_EQ(OC_STACK_OK, observe_cb.Wait(1000));

  observe_cb.Reset();
  ResourceCallback post_cb;
  Post(OC_RSRVD_SECURE_MODE_URI, &context->resource->addrs_[0], &post_cb);
  EXPECT_EQ(OC_STACK_OK, observe_cb.Wait(1000));

  EXPECT_EQ(OC_STACK_OK, observe_cb.response_->result);
  EXPECT_TRUE(observe_cb.response_->payload != NULL);
  EXPECT_EQ(PAYLOAD_TYPE_REPRESENTATION, observe_cb.response_->payload->type);
  OCRepPayload *payload = (OCRepPayload *) observe_cb.response_->payload;
  VerifyPayload(payload, false);

  delete context;
}
