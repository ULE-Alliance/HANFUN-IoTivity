#include "unit_test.h"

#include "ocstack.h"
#include "oic_malloc.h"
#include "oic_string.h"
#include "oic_time.h"
#include <thread>

static OCRepPayload *PayloadClone(OCRepPayload *payload)
{
  OCRepPayload *clone = NULL;
  if (payload)
  {
    clone = OCRepPayloadClone(payload);
    clone->next = PayloadClone(payload->next);
  }
  return clone;
}

Callback::Callback(OCClientResponseHandler callback, void *context)
  : callback_(callback), context_(context), called_(false)
{
  callback_data_.cb = &Callback::handler;
  callback_data_.cd = NULL;
  callback_data_.context = this;
}

OCStackResult Callback::Wait(long wait_ms)
{
  uint64_t start_time = OICGetCurrentTime(TIME_IN_MS);
  while (!called_)
  {
    uint64_t current_time = OICGetCurrentTime(TIME_IN_MS);
    long elapsed = (long)(current_time - start_time);
    if (elapsed > wait_ms)
    {
      break;
    }
    OCProcess();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  return called_ ? OC_STACK_OK : OC_STACK_TIMEOUT;
}

OCStackApplicationResult Callback::handler(void *ctx, OCDoHandle handle, OCClientResponse *response)
{
  Callback *callback = (Callback *) ctx;
  OCStackApplicationResult result = callback->callback_(callback->context_, handle, response);
  callback->called_ = true;
  return result;
}

ResourceCallback::ResourceCallback()
  : response_(NULL), called_(false)
{
  callback_data_.cb = &ResourceCallback::handler;
  callback_data_.cd = NULL;
  callback_data_.context = this;
}

ResourceCallback::~ResourceCallback()
{
  if (response_)
  {
    OICFree((void *) response_->resourceUri);
    OCPayloadDestroy(response_->payload);
  }
}

OCStackResult ResourceCallback::Wait(long wait_ms)
{
  uint64_t start_time = OICGetCurrentTime(TIME_IN_MS);
  while (!called_)
  {
    uint64_t current_time = OICGetCurrentTime(TIME_IN_MS);
    long elapsed = (long)(current_time - start_time);
    if (elapsed > wait_ms)
    {
      break;
    }
    OCProcess();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  return called_ ? OC_STACK_OK : OC_STACK_TIMEOUT;
}

void ResourceCallback::Reset()
{
  called_ = false;
}

OCStackApplicationResult ResourceCallback::Handler(OCDoHandle handle, OCClientResponse *response)
{
  (void) handle;
  response_ = (OCClientResponse *) OICCalloc(1, sizeof(OCClientResponse));
  EXPECT_TRUE(response_ != NULL);
  memcpy(response_, response, sizeof(OCClientResponse));
  response_->addr = &response_->devAddr;
  response->resourceUri = OICStrdup(response->resourceUri);
  if (response->payload)
  {
    switch (response->payload->type)
    {
      case PAYLOAD_TYPE_REPRESENTATION:
        response_->payload = (OCPayload *) PayloadClone((OCRepPayload *) response->payload);
        break;
      case PAYLOAD_TYPE_DIAGNOSTIC:
        response_->payload = (OCPayload *) OCDiagnosticPayloadCreate(((OCDiagnosticPayload *) response->payload)->message);
        break;
      default:
        response_->payload = NULL;
        break;
    }
  }
  called_ = true;
  return OC_STACK_DELETE_TRANSACTION;
}

OCStackApplicationResult ResourceCallback::handler(void *ctx, OCDoHandle handle, OCClientResponse *response)
{
  ResourceCallback *callback = (ResourceCallback *) ctx;
  return callback->Handler(handle, response);
}

void ObserveCallback::Reset()
{
  called_ = false;
}

OCStackApplicationResult ObserveCallback::Handler(OCDoHandle handle, OCClientResponse *response)
{
  (void) handle;
  response_ = (OCClientResponse *) OICCalloc(1, sizeof(OCClientResponse));
  EXPECT_TRUE(response_ != NULL);
  memcpy(response_, response, sizeof(OCClientResponse));
  response_->addr = &response_->devAddr;
  response_->resourceUri = OICStrdup(response->resourceUri);
  if (response->payload)
  {
    EXPECT_EQ(PAYLOAD_TYPE_REPRESENTATION, response->payload->type);
    response_->payload = (OCPayload *) PayloadClone((OCRepPayload *) response->payload);
  }
  called_ = true;
  return OC_STACK_KEEP_TRANSACTION;
}

void HFOCSetUp::SetUp()
{
  SetUpStack();
}

void HFOCSetUp::SetUpStack()
{
  EXPECT_EQ(OC_STACK_OK, OCInit2(OC_SERVER, OC_DEFAULT_FLAGS, OC_DEFAULT_FLAGS, OC_ADAPTER_IP));
  // HF Init
}

void HFOCSetUp::TearDown()
{
  TearDownStack();
}

void HFOCSetUp::TearDownStack()
{
  // HF Shutdown
  EXPECT_EQ(OC_STACK_OK, OCStop());
  // This will fail if a test starts the stack and did not stop it
  EXPECT_EQ(OC_STACK_ERROR, OCProcess());
}

void HFOCSetUp::Wait(long wait_ms)
{
  uint64_t start_time = OICGetCurrentTime(TIME_IN_MS);
  for (;;)
  {
    uint64_t curr_time = OICGetCurrentTime(TIME_IN_MS);
    long elapsed = (long)(curr_time - start_time);
    if (elapsed > wait_ms)
    {
      break;
    }
    OCProcess();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

OCStackApplicationResult Discover(void *ctx, OCDoHandle handle, OCClientResponse *response)
{
  (void) handle;
  DiscoverContext *context = (DiscoverContext *) ctx;
  EXPECT_EQ(OC_STACK_OK, response->result);
  EXPECT_TRUE(response->payload != NULL);
  EXPECT_EQ(PAYLOAD_TYPE_DISCOVERY, response->payload->type);
  OCDiscoveryPayload *payload = (OCDiscoveryPayload *) response->payload;
  context->device = new Device(response->devAddr, payload);
  context->resource = context->uri ? context->device->GetResourceUri(context->uri) : NULL;
  return OC_STACK_DELETE_TRANSACTION;
}
