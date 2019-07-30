//******************************************************************
//
// Copyright 2017 Intel Corporation All Rights Reserved.
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

#ifndef _RESOURCE_H
#define _RESOURCE_H

#include "cacommon.h"
#include "octypes.h"
#include <algorithm>
#include <map>
#include <string>
#include <vector>

/** Device Data Model version.*/
#define DEVICE_DATA_MODEL_VERSION            "ocf.res.1.1.0"

class Resource
{
public:
    std::string uri_;
    std::vector<std::string> ifs_;
    std::vector<std::string> rts_;
    bool is_observable_;
    std::vector<OCDevAddr> addrs_;
    std::vector<Resource> resources_;
    
    Resource(OCDevAddr origin, const char *di, OCResourcePayload *resource);
    bool IsSecure();
};

class Device
{
public:
    std::string di_;
    std::vector<Resource> resources_;
    
    Device(OCDevAddr origin, OCDiscoveryPayload *payload);
    Resource *GetResourceUri(const char *uri);
    Resource *GetResourceType(const char *rt);
    bool IsVirtual();
    bool SetCollectionLinks(std::string collectionUri, OCRepPayload *payload);
};

template <typename T>
bool HasResourceType(std::vector<std::string> &rts, T rt)
{
    return std::find(rts.begin(), rts.end(), rt) != rts.end();
}

std::vector<Resource>::iterator FindResourceFromUri(std::vector<Resource> &resources,
        std::string uri);

std::vector<Resource>::iterator FindResourceFromType(std::vector<Resource> &resources,
        std::string rt);

OCStackResult CreateResource(OCResourceHandle *handle, const char *uri, const char *typeName,
        const char *interfaceName, OCEntityHandler entityHandler, void *callbackParam,
        uint8_t properties);

typedef void *DoHandle;
OCStackResult DoResource(DoHandle *handle, OCMethod method, const char *uri,
        const OCDevAddr* destination, OCPayload *payload, OCCallbackData *cbData,
        OCHeaderOption *options, uint8_t numOptions);
OCStackResult DoResource(DoHandle *handle, OCMethod method, const char *uri,
        const std::vector<OCDevAddr> &destinations, OCPayload *payload, OCCallbackData *cbData,
        OCHeaderOption *options, uint8_t numOptions);
OCStackResult Cancel(DoHandle handle, OCQualityOfService qos, OCHeaderOption *options,
        uint8_t numOptions);

bool IsValidRequest(OCEntityHandlerRequest *request);
std::map<std::string, std::string> ParseQuery(OCResourceHandle resource, const char *query);
OCResourcePayload *ParseLink(OCRepPayload *payload);

OCRepPayload *CreatePayload(OCResourceHandle resource, const char *query);
bool SetResourceTypes(OCRepPayload *payload, OCResourceHandle resource);
bool SetInterfaces(OCRepPayload *payload, OCResourceHandle resource);

#endif
