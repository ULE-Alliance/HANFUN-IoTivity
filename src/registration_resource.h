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

#ifndef _REGISTRATIONRESOURCE_H
#define _REGISTRATIONRESOURCE_H

#include "octypes.h"
#include <mutex>

#include "han_client.h"

/** To represent registration resource type.*/
#define OC_RSRVD_RESOURCE_TYPE_REGISTRATION "oic.r.registration"

#define OC_RSRVD_REGISTRATION_URI "/registration"

class RegistrationResource
{
  public:
    RegistrationResource(std::mutex &mutex, HanClient &han_client);
    ~RegistrationResource();
    OCStackResult Create();
    bool IsOpen() const { return open_; }
    void SetOpen(bool open) { open_ = open; }
    uint8_t GetTimeout() const { return timeout_; }
    void SetTimeout(uint8_t timeout) { timeout_ = timeout; }

  private:
    std::mutex &mutex_;
    HanClient &han_client_;
    bool open_;
    uint8_t timeout_;
    OCResourceHandle handle_;

    OCRepPayload *GetRegistration(OCEntityHandlerRequest *request);
    bool PostRegistration(OCEntityHandlerRequest *request, bool &has_changed);
    void HFSetRegistration();
    static OCEntityHandlerResult EntityHandlerCB(OCEntityHandlerFlag flag,
            OCEntityHandlerRequest *request, void *ctx);
};

#endif // _OPENREGISTRATIONRESOURCE_H