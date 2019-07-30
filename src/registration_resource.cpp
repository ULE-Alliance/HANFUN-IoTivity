//******************************************************************
//
// Copyright 2018 Intel Corporation All Rights Reserved.
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "registration_resource.h"

#include "log.h"
#include "resource.h"
#include "ocpayload.h"
#include "ocstack.h"

RegistrationResource::RegistrationResource(std::mutex &mutex, HanClient &han_client)
  : mutex_(mutex), han_client_(han_client), open_(false), handle_(NULL)
{

}

RegistrationResource::~RegistrationResource()
{
  OCDeleteResource(handle_);
}

OCStackResult RegistrationResource::Create()
{
  return CreateResource(&handle_, OC_RSRVD_REGISTRATION_URI, OC_RSRVD_RESOURCE_TYPE_REGISTRATION,
          OC_RSRVD_INTERFACE_READ_WRITE, RegistrationResource::EntityHandlerCB, this,
          OC_DISCOVERABLE | OC_OBSERVABLE | OC_SECURE);
}

/* Called with mutex_ held. */
OCRepPayload *RegistrationResource::GetRegistration(OCEntityHandlerRequest *request)
{
  OCRepPayload *payload = CreatePayload(request->resource, request->query);
  if (!OCRepPayloadSetPropBool(payload, "open", open_))
  {
    OCRepPayloadDestroy(payload);
    payload = NULL;
  }
  size_t dimensions[MAX_REP_ARRAY_DEPTH] = { 0 };
  dimensions[0] = 1;
  const char** resource_type = new const char*[dimensions[0]];
  resource_type[0] = OC_RSRVD_RESOURCE_TYPE_REGISTRATION;
  if (!OCRepPayloadSetStringArray(payload, OC_RSRVD_RESOURCE_TYPE,
                                  resource_type, dimensions))
  {
    OCRepPayloadDestroy(payload);
    payload = NULL;
  }
  const char** interfaces = new const char*[dimensions[0]];
  interfaces[0] = OC_RSRVD_INTERFACE_READ_WRITE;
  if (!OCRepPayloadSetStringArray(payload, OC_RSRVD_INTERFACE,
                                  interfaces, dimensions))
  {
    OCRepPayloadDestroy(payload);
    payload = NULL;
  }
  return payload;
}

void RegistrationResource::HFSetRegistration()
{
  if (open_)
  {
    han_client_.open_registration();
  }
  else
  {
    han_client_.close_registration();
  }
}

/* Called with mutex_ held. */
bool RegistrationResource::PostRegistration(OCEntityHandlerRequest *request, bool &has_changed)
{
  OCRepPayload *payload = (OCRepPayload *) request->payload;
  bool open;
  if (!OCRepPayloadGetPropBool(payload, "open", &open))
  {
    return false;
  }
  has_changed = (open_ != open);
  SetOpen(open);
  HFSetRegistration();
  return true;
}

OCEntityHandlerResult RegistrationResource::EntityHandlerCB(OCEntityHandlerFlag flag,
        OCEntityHandlerRequest *request, void *ctx)
{
  LOG(LOG_DEBUG, "[%p] flag=%x,request=%p,ctx=%p", ctx, flag, request, ctx);
  if (!IsValidRequest(request))
  {
    LOG(LOG_WARN, "Invalid request received");
    return OC_EH_BAD_REQ;
  }

  RegistrationResource *thiz = reinterpret_cast<RegistrationResource *>(ctx);
  thiz->mutex_.lock();
  bool has_changed = false;
  OCEntityHandlerResult result;
  switch (request->method)
  {
    case OC_REST_GET:
      {
        OCEntityHandlerResponse response;
        memset(&response, 0, sizeof(response));
        response.requestHandle = request->requestHandle;
        response.resourceHandle = request->resource;
        OCRepPayload *payload = thiz->GetRegistration(request);
        if (!payload)
        {
          result = OC_EH_ERROR;
          break;
        }
        result = OC_EH_OK;
        response.ehResult = result;
        response.payload = reinterpret_cast<OCPayload *>(payload);
        OCStackResult do_result = OCDoResponse(&response);
        if (do_result != OC_STACK_OK)
        {
          LOG(LOG_ERR, "OCDoResponse - %d", do_result);
          OCRepPayloadDestroy(payload);
        }
        break;
      }
    case OC_REST_POST:
      {
        if (!request->payload || request->payload->type != PAYLOAD_TYPE_REPRESENTATION)
        {
          result = OC_EH_ERROR;
          break;
        }
        if (!thiz->PostRegistration(request, has_changed))
        {
          result = OC_EH_ERROR;
          break;
        }
        OCEntityHandlerResponse response;
        memset(&response, 0, sizeof(response));
        response.requestHandle = request->requestHandle;
        response.resourceHandle = request->resource;
        OCRepPayload *out_payload = thiz->GetRegistration(request);
        result = OC_EH_OK;
        response.ehResult = result;
        response.payload = reinterpret_cast<OCPayload *>(out_payload);
        OCStackResult do_result = OCDoResponse(&response);
        if (do_result != OC_STACK_OK)
        {
          LOG(LOG_ERR, "OCDoResponse - %d", do_result);
          OCRepPayloadDestroy(out_payload);
        }
        break;
      }
    default:
      result = OC_EH_METHOD_NOT_ALLOWED;
      break;
  }
  thiz->mutex_.unlock();
  if (has_changed)
  {
    OCNotifyAllObservers(request->resource, OC_NA_QOS);
  }
  return result;
}