//******************************************************************
//
// Copyright 2016 Intel Corporation All Rights Reserved.
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

#include "plugin.h"

#include "log.h"

#include "ocpayload.h"
#include "experimental/ocrandom.h"
#include "ocstack.h"
#include "rd_client.h"
#include <map>
#include <vector>

std::string kResourceDirectory;

static OCStackApplicationResult RDPublishCB(void *context,
                                            OCDoHandle handle,
                                            OCClientResponse *response)
{
  UNUSED(context);
  UNUSED(handle);

  int severity = (response && (response->result <= OC_STACK_RESOURCE_CHANGED)) ? LOG_DEBUG : LOG_ERR;
  LOG(severity, "response=%p,response->result=%d", response, response ? response->result : 0);
  return OC_STACK_DELETE_TRANSACTION;
}

OCStackResult RDPublish()
{
  uint8_t number_of_resources;
  OCStackResult result = OCGetNumberOfResources(&number_of_resources);
  if (result != OC_STACK_OK)
  {
    return result;
  }

  std::vector<OCResourceHandle> handles;
  for (uint8_t i = 0; i < number_of_resources; ++i)
  {
    OCResourceHandle handle = OCGetResourceHandle(i);
    if (OCGetResourceProperties(handle) & OC_DISCOVERABLE)
    {
      handles.push_back(handle);
    }
  }

  OCCallbackData callback_data;
  callback_data.cb = RDPublishCB;
  callback_data.context = NULL;
  callback_data.cd = NULL;
  return OCRDPublish(NULL,
                     kResourceDirectory.c_str(),
                     CT_DEFAULT, // default connectivity type
                     &handles[0],
                     handles.size(),
                     OIC_RD_PUBLISH_TTL, // publish TTL
                     &callback_data,
                     OC_HIGH_QOS); // High QoS
}
