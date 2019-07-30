#ifndef _UNITTEST_H
#define _UNITTEST_H

#include "ocpayload.h"
#include "resource.h"
#include <gtest/gtest.h>

struct LocalizedString
{
  const char *language;
  const char *value;
};

struct DiscoverContext
{
  const char *uri;
  Device *device;
  Resource *resource;
  DiscoverContext(const char *uri = NULL) : uri(uri), device(NULL), resource(NULL) { }
  ~DiscoverContext() { delete device; }
};

class HFOCSetUp : public testing::Test
{
  public:
    static void SetUpStack();
    static void TearDownStack();
    
  protected:
    virtual ~HFOCSetUp() {}
    virtual void SetUp();
    virtual void TearDown();
    void Wait(long wait_ms);
};

class Callback
{
  public:
    Callback(OCClientResponseHandler callback, void *context = NULL);
    OCStackResult Wait(long wait_ms);
    operator OCCallbackData *() { return &callback_data_; }

  private:
    OCClientResponseHandler callback_;
    void *context_;
    bool called_;
    OCCallbackData callback_data_;
    static OCStackApplicationResult handler(void *context, OCDoHandle handle, OCClientResponse *response);
};

class ResourceCallback
{
  public:
    OCClientResponse *response_;
    ResourceCallback();
    virtual ~ResourceCallback();
    OCStackResult Wait(long wait_ms);
    void Reset();
    operator OCCallbackData *() { return &callback_data_; }

  protected:
    bool called_;
    virtual OCStackApplicationResult Handler(OCDoHandle handle, OCClientResponse *response);

  private:
    OCCallbackData callback_data_;
    static OCStackApplicationResult handler(void *context, OCDoHandle handle, OCClientResponse *response);
};

class ObserveCallback : public ResourceCallback
{
  public:
    virtual ~ObserveCallback() { }
    void Reset();

  protected:
    virtual OCStackApplicationResult Handler(OCDoHandle handle, OCClientResponse *response);
};

OCStackApplicationResult Discover(void *ctx, OCDoHandle handle, OCClientResponse *response);

#endif /* _UNITTEST_H */
