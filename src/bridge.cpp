#include "bridge.h"

#include "device_configuration_resource.h"
#include "device_information.h"
#include "interfaces.h"
#include "introspection.h"
#include "log.h"
#include "platform_configuration_resource.h"
#include "plugin.h"
#include "presence.h"
#include "registration_resource.h"
#include "resource.h"
#include "secure_mode_resource.h"
#include "security.h"
#include "virtual_ocf_device.h"
#include "virtual_resource.h"

#include "ocstack.h"
#include "oic_malloc.h"
#include <algorithm>
#include <assert.h>
#include <chrono>
#include <thread>

#if __WITH_DTLS__
#define SECURE_MODE_DEFAULT true
#else
#define SECURE_MODE_DEFAULT false
#endif

struct Bridge::DiscoverContext
{
  Bridge *bridge;
  Device device;
  OCRepPayload *paths;
  OCRepPayload *definitions;
  std::vector<Resource>::iterator rit;
  DiscoverContext(Bridge *bridge, OCDevAddr origin, OCDiscoveryPayload *payload)
    : bridge(bridge), device(origin, payload), paths(NULL), definitions(NULL) {}
  ~DiscoverContext()
  {
    OCRepPayloadDestroy(paths);
    OCRepPayloadDestroy(definitions);
  }
  std::vector<OCDevAddr> GetDevAddrs(const char *uri)
  {
    Resource *resource = device.GetResourceUri(uri);
    if (resource)
    {
      return resource->addrs_;
    }
    return std::vector<OCDevAddr>();
  }
  
  // Each iteration returns a translatable resource and resource type pair.
  struct Iterator
  {
    DiscoverContext *context;
    std::vector<Resource>::iterator resource_it;
    std::vector<std::string>::iterator resource_type_it;
    
    Iterator() {}
    Iterator(DiscoverContext *context, bool is_begin = true) : context(context)
    {
      if (is_begin)
      {
        Iterate(is_begin);
      }
      else
      {
        resource_it = context->device.resources_.end();
        resource_type_it = context->device.resources_.back().rts_.end();
      }
    }
    void Iterate(bool is_begin)
    {
      if (!is_begin)
      {
        goto next;
      }
      resource_it = context->device.resources_.begin();
      while (resource_it != context->device.resources_.end())
      {
        if (!context->bridge->secure_mode_->GetSecureMode() || resource_it->IsSecure())
        {
          resource_type_it = resource_it->rts_.begin();
          while (resource_type_it != resource_it->rts_.end())
          {
            if (TranslateResourceType(resource_type_it->c_str()))
            {
              return;
            }
          next:
            ++resource_type_it;
          }
        }
        ++resource_it;
      }
    }
    std::string GetUri()
    {
      std::string uri = resource_it->uri_;
      if (resource_it->rts_.size() > 1)
      {
        uri += "?rt=" + *resource_type_it;
      }
      return uri;
    }
    std::vector<OCDevAddr> GetDevAddrs()
    {
      return resource_it->addrs_;
    }
    Resource& GetResource()
    {
      return *resource_it;
    }
    std::string GetResourceType()
    {
      return *resource_type_it;
    }
    Iterator& operator++()
    {
      Iterate(false);
      return *this;
    }
    bool operator!=(const Iterator &rhs)
    {
      return (resource_it != rhs.resource_it) && (resource_type_it != rhs.resource_type_it);
    }
  };
  Iterator Begin() { return Iterator(this, true); }
  Iterator End() { return Iterator(this, false); }
  Iterator it;
};

Bridge::Bridge(const std::string &base_uri, Protocol protocols)
  : exec_cb_(NULL), disconnected_cb_(NULL), protocols_(protocols), sender_(0),
    discover_handle_(NULL), discover_next_tick_(0), secure_mode_(NULL),
    rd_publish_task_(NULL), pending_(0), get_devices_next_tick_(0)
{
  han_client_ = new HanClient("127.0.0.1", 3490, uv_default_loop());
  han_state_ = CREATED;
  oc_security_ = new OCSecurity();
  secure_mode_ = new SecureModeResource(mutex_, SECURE_MODE_DEFAULT);
  registration_ = new RegistrationResource(mutex_, *han_client_);
}

Bridge::Bridge(const std::string &base_uri, uint16_t sender)
  : exec_cb_(NULL), disconnected_cb_(NULL), protocols_(HF), sender_(sender),
    discover_handle_(NULL), discover_next_tick_(0), secure_mode_(NULL),
    rd_publish_task_(NULL), pending_(0), get_devices_next_tick_(0)
{
  han_client_ = new HanClient("127.0.0.1", 3490, uv_default_loop());
  han_state_ = CREATED;
  oc_security_ = new OCSecurity();
  secure_mode_ = new SecureModeResource(mutex_, SECURE_MODE_DEFAULT);
}

Bridge::~Bridge()
{
  LOG(LOG_DEBUG, "[%p]", this);
  
  std::unique_lock<std::mutex> lock(mutex_);
  while (pending_ > 0)
  {
    cond_.wait(lock);
  }
  for (Presence *presence : presence_)
  {
    delete presence;
  }
  presence_.clear();
  for (auto &dc : discovered_)
  {
    DiscoverContext *discoverContext = dc.second;
    delete discoverContext;
  }
  discovered_.clear();
  for (VirtualResource *resource : virtual_resources_)
  {
    delete resource;
  }
  virtual_resources_.clear();
  for (VirtualOcfDevice *device : virtual_ocf_devices_)
  {
    delete device;
  }
  virtual_ocf_devices_.clear();
  
  delete oc_security_;

  han_client_->stop();
  delete han_client_;
}

void Bridge::SetSecureMode(bool secure_mode)
{
  secure_mode_->SetSecureMode(secure_mode);
}

// Called with mutex_ held.
void Bridge::Destroy(const char *id)
{
  std::map<OCDoHandle, DiscoverContext *>::iterator dc = discovered_.begin();
  while (dc != discovered_.end())
  {
    DiscoverContext *context = dc->second;
    if (context->device.di_ == id)
    {
      delete context;
      dc = discovered_.erase(dc);
    }
    else
    {
      ++dc;
    }
  }
  std::vector<Presence *>::iterator p = presence_.begin();
  while (p != presence_.end())
  {
    Presence *presence = *p;
    if (presence->GetId() == id)
    {
      delete presence;
      p = presence_.erase(p);
    }
    else
    {
      ++p;
    }
  }
}

/* Called with mutex_ held. */
void Bridge::Destroy(uint16_t id)
{
  std::vector<VirtualResource *>::iterator vr = virtual_resources_.begin();
  while (vr != virtual_resources_.end())
  {
    VirtualResource *resource = *vr;
    if (resource->address() == id)
    {
      delete resource;
      vr = virtual_resources_.erase(vr);
    }
    else
    {
      ++vr;
    }
  }
  std::vector<VirtualOcfDevice *>::iterator vd = virtual_ocf_devices_.begin();
  while (vd != virtual_ocf_devices_.end())
  {
    VirtualOcfDevice *device = *vd;
    if (device->address() == id)
    {
      delete device;
      vd = virtual_ocf_devices_.erase(vd);
    }
    else
    {
      ++vd;
    }
  }
}

bool Bridge::Start()
{
  std::lock_guard<std::mutex> lock(mutex_);
  
  if (!oc_security_->Init())
  {
    return false;
  }
  if (sender_ == 0)
  {
    OCStackResult result;
    OCResourceHandle handle;
    handle = OCGetResourceHandleAtUri(OC_RSRVD_WELL_KNOWN_URI);
    if (!handle)
    {
      LOG(LOG_ERR, "OCGetResourceHandleAtUri(" OC_RSRVD_WELL_KNOWN_URI ") failed");
      return false;
    }
    result = OCSetResourceProperties(handle, OC_DISCOVERABLE | OC_OBSERVABLE);
    if (result != OC_STACK_OK)
    {
      LOG(LOG_ERR, "OCSetResourceProperties() - %d", result);
      return false;
    }
    result = OCSetPropertyValue(PAYLOAD_TYPE_DEVICE, OC_RSRVD_DEVICE_NAME, device_name_.c_str());
    if (result != OC_STACK_OK)
    {
      LOG(LOG_ERR, "OCSetPropertyValue() - %d", result);
      return false;
    }
    result = OCSetPropertyValue(PAYLOAD_TYPE_DEVICE, OC_RSRVD_SPEC_VERSION, OC_SPEC_VERSION);
    if (result != OC_STACK_OK)
    {
      LOG(LOG_ERR, "OCSetPropertyValue() - %d", result);
      return false;
    }
    result = OCSetPropertyValue(PAYLOAD_TYPE_DEVICE, OC_RSRVD_DATA_MODEL_VERSION, DEVICE_DATA_MODEL_VERSION);
    if (result != OC_STACK_OK)
    {
      LOG(LOG_ERR, "OCSetPropertyValue() - %d", result);
      return false;
    }
    handle = OCGetResourceHandleAtUri(OC_RSRVD_DEVICE_URI);
    if (!handle)
    {
      LOG(LOG_ERR, "OCGetResourceHandleAtUri(" OC_RSRVD_DEVICE_URI ") failed");
      return false;
    }
    result = OCBindResourceTypeToResource(handle, "oic.d.bridge");
    if (result != OC_STACK_OK)
    {
      LOG(LOG_ERR, "OCBindResourceTypeToResource() - %d", result);
      return false;
    }
    result = OCSetPropertyValue(PAYLOAD_TYPE_PLATFORM, OC_RSRVD_MFG_NAME, manufacturer_name_.c_str());
    if (result != OC_STACK_OK)
    {
      LOG(LOG_ERR, "OCSetPropertyValue() - %d", result);
      return false;
    }
    result = secure_mode_->Create();
    if (result != OC_STACK_OK)
    {
      LOG(LOG_ERR, "SecureModeResource::Create() - %d", result);
      return false;
    }
    result = registration_->Create();
    if (result != OC_STACK_OK)
    {
      LOG(LOG_ERR, "RegistrationResource::Create() - %d", result);
      return false;
    }
    SetIntrospectionData(/* HF Info */"TITLE", "VERSION");
  }
  LOG(LOG_INFO, "di=%s", OCGetServerInstanceIDString());
  
  if (protocols_ & HF)
  {
    int result = han_client_->start();
    if (result != 0)
    {
      LOG(LOG_ERR, "HanClient::start %d", result);
      return false;
    }
  }
  return true;
}

bool Bridge::Stop()
{
  LOG(LOG_DEBUG, "[%p]", this);
  
  std::lock_guard<std::mutex> lock(mutex_);

  // TODO: Add HF Virtual Devices stop
  
  if (discover_handle_)
  {
    OCStackResult result = Cancel(discover_handle_, OC_LOW_QOS, NULL, 0);
    if (result != OC_STACK_OK)
    {
      LOG(LOG_ERR, "Cancel() - %d", result);
    }
    discover_handle_ = NULL;
  }
  return true;
}

void Bridge::ResetSecurity()
{
  LOG(LOG_DEBUG, "[%p]", this);
  
  if (oc_security_)
  {
    oc_security_->Reset();
  }
}

bool Bridge::Process()
{
  std::lock_guard<std::mutex> lock(mutex_);
  if (protocols_ & HF)
  {
    switch (han_state_)
    {
      case CREATED:
        if (han_client_->is_initialized())
        {
          han_client_->set_device_table_cb(Bridge::GetDeviceTableCB);
          han_state_ = STARTED;
        }
        break;
      case STARTED:
        if (time(NULL) >= get_devices_next_tick_)
        {
          if (sender_ == 0)
          {
            han_client_->get_device_table(0, 5, this);
          }
          else
          {
            // Initialize virtualization
            han_client_->get_device_table(sender_ - 1, 1, this);
          }
          get_devices_next_tick_ = time(NULL) + HF_DISCOVER_PERIOD_SECS;
        }
        break;
      case RUNNING:
        /* Do nothing */
        break;
    }
  }
  if (protocols_ & OC)
  {
    if (time(NULL) >= discover_next_tick_)
    {
      if (discover_handle_)
      {
        OCStackResult result = Cancel(discover_handle_, OC_LOW_QOS, NULL, 0);
        if (result != OC_STACK_OK)
        {
          LOG(LOG_ERR, "Cancel() - %d", result);
        }
        discover_handle_ = NULL;
      }
      OCCallbackData cbData;
      cbData.cb = Bridge::DiscoverCB;
      cbData.context = this;
      cbData.cd = NULL;
      OCHeaderOption options[1];
      size_t numOptions = 0;
      uint16_t format = COAP_MEDIATYPE_APPLICATION_VND_OCF_CBOR; // TODO retry with CBOR
      OCSetHeaderOption(options, &numOptions, CA_OPTION_ACCEPT, &format, sizeof(format));
      ::DoResource(&discover_handle_, OC_REST_DISCOVER, OC_RSRVD_WELL_KNOWN_URI, NULL, 0, &cbData, options, numOptions);
      discover_next_tick_ = time(NULL) + OCF_DISCOVER_PERIOD_SECS;
    }
  }
  std::vector<std::string> absent;
  for (Presence *presence : presence_)
  {
    if (!presence->IsPresent())
    {
      absent.push_back(presence->GetId());
    }
  }
  for (std::string &id : absent)
  {
    LOG(LOG_DEBUG, "[%p] %s absent", this, id.c_str());
    Destroy(id.c_str());
  }
  std::list<Task *>::iterator task = tasks_.begin();
  while (task != tasks_.end())
  {
    if (time(NULL) >= (*task)->tick)
    {
      (*task)->Run(this);
      delete (*task);
      task = tasks_.erase(task);
    }
    else
    {
      ++task;
    }
  }
  return true;
}

VirtualResource *Bridge::CreateVirtualResource(uint16_t address, const char *path)
{
  return VirtualResource::Create(address, path, RDPublish, this);
}

bool Bridge::IsVirtual(HF::Attributes::List *attributes)
{
  bool is_virtual = false;
  auto uid_attribute = std::find_if(attributes->begin(), attributes->end(), HasAttribute(HF::Core::DeviceInformation::UID_ATTR));

  if (uid_attribute != attributes->end())
  {
    auto *uid = HF::Attributes::adapt<HF::UID::UID>(*uid_attribute);
    switch (uid->get().type())
    {
      case HF::UID::NONE_UID:
        // Do nothing
        break;
      case HF::UID::DECT_UID: // RFPI
        // TODO
        break;
      case HF::UID::MAC_UID:
        // TODO
        break;
      case HF::UID::URI_UID:
        is_virtual = static_cast<const HF::UID::URI *>(uid->get().raw())->str().find("hf://node.virtual.com/") == 0;
        break;
    }
  }
  else
  {
    // TODO: Add last-chance option
  }

  return is_virtual;
}

void Bridge::UpdatePresenceStatus(const OCDiscoveryPayload *payload)
{
  for (Presence *p : presence_)
  {
    if (p->GetId() == payload->sid)
    {
      p->Seen();
    }
  }
}

bool Bridge::IsSelf(const OCDiscoveryPayload *payload)
{
  return !strcmp(payload->sid, OCGetServerInstanceIDString());
}

bool Bridge::HasSeenBefore(const OCDiscoveryPayload *payload)
{
  for (auto &d : discovered_)
  {
    if (d.second->device.di_ == payload->sid)
    {
      return true;
    }
  }
  return false;
}

bool Bridge::IsSecure(const OCResourcePayload *resource)
{
  if (resource->secure)
  {
    return true;
  }
  for (OCEndpointPayload *ep = resource->eps; ep; ep = ep->next)
  {
    if (ep->family & OC_SECURE)
    {
      return true;
    }
  }
  return false;
}

bool Bridge::HasTranslatableResource(OCDiscoveryPayload *payload)
{
  for (OCResourcePayload *resource = payload->resources; resource; resource = resource->next)
  {
    if (secure_mode_->GetSecureMode() && !IsSecure(resource))
    {
      continue;
    }
    for (OCStringLL *type = resource->types; type; type = type->next)
    {
      if (TranslateResourceType(type->value))
      {
        return true;
      }
    }
  }
  return false;
}

OCStackResult Bridge::DoResource(OCDoHandle *handle, OCMethod method, const char *uri, const std::vector<OCDevAddr> &addrs, OCClientResponseHandler cb)
{
  OCCallbackData cbData;
  cbData.cb = cb;
  cbData.context = this;
  cbData.cd = NULL;
  return ::DoResource(handle, method, uri, addrs, NULL, &cbData, NULL, 0);
}

OCStackResult Bridge::DoResource(OCDoHandle *handle, OCMethod method, const char *uri, OCDevAddr *addr, OCClientResponseHandler cb)
{
  std::vector<OCDevAddr> addrs = { *addr };
  return DoResource(handle, method, uri, addrs, cb);
}

void Bridge::GetContextAndRepPayload(OCDoHandle handle, OCClientResponse *response, DiscoverContext **context, OCRepPayload **payload)
{
  *context = NULL;
  *payload = NULL;
  
  std::map<OCDoHandle, DiscoverContext *>::iterator it = discovered_.find(handle);
  if (it != discovered_.end())
  {
    *context = it->second;
  }
  
  if (response && response->result == OC_STACK_OK &&
      response->payload && response->payload->type == PAYLOAD_TYPE_REPRESENTATION)
  {
    *payload = (OCRepPayload *) response->payload;
  }
  else if (response)
  {
    LOG(LOG_DEBUG, "[%p] response={result=%d,resourceUri=%s,payload={type=%d}}",
      this, response->result, response->resourceUri,
      response->payload ? response->payload->type : PAYLOAD_TYPE_INVALID);
  }
  else
  {
    LOG(LOG_DEBUG, "[%p] response=%p", this, response);
  }
}

OCStackResult Bridge::ContinueDiscovery(DiscoverContext *context,
                                        const char *uri,
                                        const std::vector<OCDevAddr> &addrs,
                                        OCClientResponseHandler cb)
{
  OCDoHandle cbHandle;
  OCStackResult result = DoResource(&cbHandle, OC_REST_GET, uri, addrs, cb);
  if (result == OC_STACK_OK)
  {
    discovered_[cbHandle] = context;
  }
  return result;
}

OCStackResult Bridge::ContinueDiscovery(DiscoverContext *context,
                                        const char *uri,
                                        OCDevAddr *addr,
                                        OCClientResponseHandler cb)
{
  OCDoHandle cbHandle;
  OCStackResult result = DoResource(&cbHandle, OC_REST_GET, uri, addr, cb);
  if (result == OC_STACK_OK)
  {
    discovered_[cbHandle] = context;
  }
  return result;
}

OCStackApplicationResult Bridge::DiscoverCB(void *ctx,
                                            OCDoHandle handle,
                                            OCClientResponse *response)
{
  (void) handle;
  Bridge *thiz = reinterpret_cast<Bridge *>(ctx);
  
  std::lock_guard<std::mutex> lock(thiz->mutex_);
  if (!response || response->result != OC_STACK_OK)
  {
    goto exit;
  }
  OCDiscoveryPayload *payload;
  for (payload = (OCDiscoveryPayload *) response->payload; payload; payload = payload->next)
  {
    DiscoverContext *context = NULL;
    std::vector<OCDevAddr> addrs;
    OCStackResult result;
    thiz->UpdatePresenceStatus(payload);
    LOG(LOG_DEBUG, "isSelf=%d, hasSeenBefore=%d, hasTranslatableResource=%d", thiz->IsSelf(payload),
      thiz->HasSeenBefore(payload), thiz->HasTranslatableResource(payload));
    if (thiz->IsSelf(payload) || thiz->HasSeenBefore(payload) ||
        !thiz->HasTranslatableResource(payload))
    {
      goto next;
    }
    context = new DiscoverContext(thiz, response->devAddr, payload);
    if (!context)
    {
      goto next;
    }
    result = thiz->ContinueDiscovery(context, OC_RSRVD_DEVICE_URI,
      context->GetDevAddrs(OC_RSRVD_DEVICE_URI), Bridge::GetDeviceCB);
    if (result == OC_STACK_OK)
    {
      context = NULL;
    }
  next:
    delete context;
  }
  
exit:
  return OC_STACK_KEEP_TRANSACTION;
}

OCStackApplicationResult Bridge::GetDeviceCB(void *ctx, OCDoHandle handle, OCClientResponse *response)
{
  Bridge *thiz = reinterpret_cast<Bridge *>(ctx);
  LOG(LOG_DEBUG, "[%p]", thiz);
  
  std::lock_guard<std::mutex> lock(thiz->mutex_);
  OCStackResult result;
  bool is_virtual;
  char *piid = NULL;
  DiscoverContext *context;
  OCRepPayload *payload;
  thiz->GetContextAndRepPayload(handle, response, &context, &payload);
  if (!context || !payload)
  {
    goto exit;
  }
  OCRepPayloadGetPropString(payload, OC_RSRVD_PROTOCOL_INDEPENDENT_ID, &piid);
  is_virtual = context->device.IsVirtual();
  switch (thiz->GetSeenState(piid))
  {
    case NOT_SEEN:
      if (is_virtual)
      {
        // Delay creating virtual objects from a virtual device
        LOG(LOG_DEBUG, "[%p] Delaying creation of virtual objects from a virtual device", thiz);
        thiz->tasks_.push_back(new DiscoverTask(time(NULL) + 10, piid, payload, context));
        context = NULL;
        goto exit;
      }
      else
      {
        
      }
      break;
    case SEEN_NATIVE:
      // Do nothing
      goto exit;
    case SEEN_VIRTUAL:
      if (is_virtual)
      {
        // Do nothing
      }
      else
      {
        thiz->DestroyPiid(piid);
      }
      break;
  }
  result = thiz->ContinueDiscovery(context, OC_RSRVD_PLATFORM_URI, context->GetDevAddrs(OC_RSRVD_PLATFORM_URI), Bridge::GetPlatformCB);
  if (result == OC_STACK_OK)
  {
    context = NULL;
  }
  
exit:
  OICFree(piid);
  delete context;
  thiz->discovered_.erase(handle);
  return OC_STACK_DELETE_TRANSACTION;
}

OCStackApplicationResult Bridge::GetPlatformCB(void *ctx, OCDoHandle handle, OCClientResponse *response)
{
  Bridge *thiz = reinterpret_cast<Bridge *>(ctx);
  LOG(LOG_DEBUG, "[%p]", thiz);
  
  std::lock_guard<std::mutex> lock(thiz->mutex_);
  OCStackResult result = OC_STACK_OK;
  DiscoverContext *context;
  OCRepPayload *payload;
  Resource *resource;
  thiz->GetContextAndRepPayload(handle, response, &context, &payload);
  if (!context || !payload)
  {
    goto exit;
  }
  
  resource = context->device.GetResourceType(OC_RSRVD_RESOURCE_TYPE_DEVICE_CONFIGURATION);
  if (resource)
  {
    result = thiz->ContinueDiscovery(context, resource->uri_.c_str(), resource->addrs_, Bridge::GetDeviceConfigurationCB);
  }
  else
  {
    result = thiz->GetPlatformConfiguration(context);
  }
  if (result == OC_STACK_OK)
  {
    context = NULL;
  }

exit:
  delete context;
  thiz->discovered_.erase(handle);
  return OC_STACK_DELETE_TRANSACTION;
}

OCStackApplicationResult Bridge::GetDeviceConfigurationCB(void *ctx, OCDoHandle handle, OCClientResponse *response)
{
  Bridge *thiz = reinterpret_cast<Bridge *>(ctx);
  LOG(LOG_DEBUG, "[%p]", thiz);
  
  std::lock_guard<std::mutex> lock(thiz->mutex_);
  OCStackResult result = OC_STACK_OK;
  DiscoverContext *context;
  OCRepPayload *payload;
  thiz->GetContextAndRepPayload(handle, response, &context, &payload);
  if (!context || !payload)
  {
    goto exit;
  }
  
  result = thiz->GetPlatformConfiguration(context);
  if (result == OC_STACK_OK)
  {
    context = NULL;
  }

exit:
  delete context;
  thiz->discovered_.erase(handle);
  return OC_STACK_DELETE_TRANSACTION;
}

OCStackResult Bridge::GetPlatformConfiguration(DiscoverContext *context)
{
  OCStackResult result = OC_STACK_OK;
  Resource *resource;
  resource = context->device.GetResourceType(OC_RSRVD_RESOURCE_TYPE_PLATFORM_CONFIGURATION);
  if (resource)
  {
    result = ContinueDiscovery(context, resource->uri_.c_str(), resource->addrs_,
      Bridge::GetPlatformConfigurationCB);
  }
  else
  {
    result = GetCollection(context);
  }
  return result;
}

OCStackApplicationResult Bridge::GetPlatformConfigurationCB(void *ctx,
                                                            OCDoHandle handle,
                                                            OCClientResponse *response)
{
  Bridge *thiz = reinterpret_cast<Bridge *>(ctx);
  LOG(LOG_DEBUG, "[%p]", thiz);
  
  std::lock_guard<std::mutex> lock(thiz->mutex_);
  OCStackResult result = OC_STACK_OK;
  DiscoverContext *context;
  OCRepPayload *payload;
  thiz->GetContextAndRepPayload(handle, response, &context, &payload);
  if (!context)
  {
    goto exit;
  }
  
  result = thiz->GetCollection(context);
  if (result == OC_STACK_OK)
  {
    context = NULL;
  }
  
exit:
  delete context;
  thiz->discovered_.erase(handle);
  return OC_STACK_DELETE_TRANSACTION;
}

OCStackResult Bridge::GetCollection(DiscoverContext *context)
{
  OCStackResult result = OC_STACK_OK;
  for (context->rit = context->device.resources_.begin(); context->rit != context->device.resources_.end(); ++context->rit)
  {
    Resource &r = *context->rit;
    if (HasResourceType(r.rts_, "oic.r.hanfunobject"))
    {
      result = ContinueDiscovery(context, r.uri_.c_str(), r.addrs_, Bridge::GetCollectionCB);
      if (result == OC_STACK_OK)
      {
        context = NULL;
      }
      goto exit;
    }
  }
  result = GetIntrospection(context);
  if (result == OC_STACK_OK)
  {
    context = NULL;
  }
  
exit:
  return result;
}

OCStackApplicationResult Bridge::GetCollectionCB(void *ctx, OCDoHandle handle, OCClientResponse *response)
{
  Bridge *thiz = reinterpret_cast<Bridge *>(ctx);
  LOG(LOG_DEBUG, "[%p]", thiz);
  
  std::lock_guard<std::mutex> lock(thiz->mutex_);
  OCStackResult result = OC_STACK_OK;
  DiscoverContext *context;
  OCRepPayload *payload;
  thiz->GetContextAndRepPayload(handle, response, &context, &payload);
  if (!context || !payload)
  {
    goto exit;
  }
  if (!context->device.SetCollectionLinks(context->rit->uri_, payload))
  {
    goto exit;
  }
  for (++context->rit; context->rit != context->device.resources_.end(); ++context->rit)
  {
    Resource &r = *context->rit;
    if (HasResourceType(r.rts_, "oic.r.hanfunobject"))
    {
      result = thiz->ContinueDiscovery(context, r.uri_.c_str(), r.addrs_, Bridge::GetCollectionCB);
      if (result == OC_STACK_OK)
      {
        context = NULL;
      }
      goto exit;
    }
  }
  result = thiz->GetIntrospection(context);
  if (result == OC_STACK_OK)
  {
    context = NULL;
  }
  
exit:
  delete context;
  thiz->discovered_.erase(handle);
  return OC_STACK_DELETE_TRANSACTION;
}

OCStackResult Bridge::GetIntrospection(DiscoverContext *context)
{
  OCStackResult result = OC_STACK_OK;
  Resource *resource;
  resource = context->device.GetResourceType(OC_RSRVD_RESOURCE_TYPE_INTROSPECTION);
  if (resource)
  {
    result = ContinueDiscovery(context, resource->uri_.c_str(), resource->addrs_, Bridge::GetIntrospectionCB);
  }
  else
  {
    LOG(LOG_DEBUG, "[%p] Missing introspection resource", this);
    context->paths = OCRepPayloadCreate();
    context->definitions = OCRepPayloadCreate();
    if (!context->paths || !context->definitions)
    {
      LOG(LOG_ERR, "Failed to create payload");
      return result;
    }
    context->it = context->Begin();
    result = ContinueDiscovery(context, context->it.GetUri().c_str(), context->it.GetDevAddrs(), Bridge::GetCB);
  }
  return result;
}

OCStackApplicationResult Bridge::GetIntrospectionCB(void *ctx, OCDoHandle handle, OCClientResponse *response)
{
  Bridge *thiz = reinterpret_cast<Bridge *>(ctx);
  LOG(LOG_DEBUG, "[%p]", thiz);
  
  std::lock_guard<std::mutex> lock(thiz->mutex_);
  OCStackResult result = OC_STACK_ERROR;
  char *url = NULL;
  char *protocol = NULL;
  size_t dim[MAX_REP_ARRAY_DEPTH] = { 0 };
  size_t dim_total;
  OCRepPayload **url_info = NULL;
  DiscoverContext *context;
  OCRepPayload *payload;
  thiz->GetContextAndRepPayload(handle, response, &context, &payload);
  if (!context || !payload)
  {
    goto exit;
  }
  if (!OCRepPayloadGetPropObjectArray(payload, OC_RSRVD_INTROSPECTION_URL_INFO, &url_info, dim))
  {
    goto exit;
  }
  dim_total = calcDimTotal(dim);
  for (size_t i = 0; i < dim_total; ++i)
  {
    if (!OCRepPayloadGetPropString(url_info[i], OC_RSRVD_INTROSPECTION_PROTOCOL, &protocol) ||
        !OCRepPayloadGetPropString(url_info[i], OC_RSRVD_INTROSPECTION_URL, &url))
    {
      LOG(LOG_ERR, "[%p] Failed to get mandatory protocol or url properties", thiz);
      goto exit;
    }
    if (!strcmp(protocol, "coap") || !strcmp(protocol, "coaps")
#ifdef TCP_ADAPTER
      || !strcmp(protocol, "coap+tcp") || !strcmp(protocol, "coaps+tcp"))
#endif
    )
    {
      LOG(LOG_DEBUG, "[%p] protocol=%s,url=%s", thiz, protocol, url);
      OCStackResult result = thiz->ContinueDiscovery(context, url, &response->devAddr, Bridge::GetIntrospectionDataCB);
      if (result == OC_STACK_OK)
      {
        context = NULL;
        break;
      }
    }
    OICFree(protocol);
    protocol = NULL;
    OICFree(url);
    url = NULL;
  }
  
exit:
  if (context && (result != OC_STACK_OK))
  {
    context->paths = OCRepPayloadCreate();
    context->definitions = OCRepPayloadCreate();
    if (!context->paths || !context->definitions)
    {
      LOG(LOG_ERR, "Failed to create payload");
      goto exit;
    }
    context->it = context->Begin();
    result = thiz->ContinueDiscovery(context, context->it.GetUri().c_str(), context->it.GetDevAddrs(), Bridge::GetCB);
    if (result == OC_STACK_OK)
    {
      context = NULL;
    }
  }
  OICFree(url);
  OICFree(protocol);
  if (url_info)
  {
    size_t dim_total = calcDimTotal(dim);
    for (size_t i = 0; i < dim_total; ++i)
    {
      OCRepPayloadDestroy(url_info[i]);
    }
  }
  OICFree(url_info);
  delete context;
  thiz->discovered_.erase(handle);
  return OC_STACK_DELETE_TRANSACTION;  
}

OCStackApplicationResult Bridge::GetIntrospectionDataCB(void *ctx, OCDoHandle handle, OCClientResponse *response)
{
  Bridge *thiz = reinterpret_cast<Bridge *>(ctx);
  LOG(LOG_DEBUG, "[%p]", thiz);
  
  std::lock_guard<std::mutex> lock(thiz->mutex_);
  OCStackResult result = OC_STACK_ERROR;
  DiscoverContext *context;
  OCRepPayload *payload;
  
  thiz->GetContextAndRepPayload(handle, response, &context, &payload);
  if (!context || !payload)
  {
    goto exit;
  }
  if (thiz->ParseIntrospectionPayload(context, payload))
  {
    result = OC_STACK_OK;
  }
  
exit:
  if (context && (result != OC_STACK_OK))
  {
    context->paths = OCRepPayloadCreate();
    context->definitions = OCRepPayloadCreate();
    if (!context->paths || !context->definitions)
    {
      LOG(LOG_ERR, "Failed to create payload");
      goto exit;
    }
    context->it = context->Begin();
    result = thiz->ContinueDiscovery(context, context->it.GetUri().c_str(), context->it.GetDevAddrs(), Bridge::GetCB);
    if (result == OC_STACK_OK)
    {
      context = NULL;
    }
  }
  delete context;
  thiz->discovered_.erase(handle);
  return OC_STACK_DELETE_TRANSACTION;
}

OCStackApplicationResult Bridge::GetCB(void *ctx, OCDoHandle handle, OCClientResponse *response)
{
  Bridge *thiz = reinterpret_cast<Bridge *>(ctx);
  LOG(LOG_DEBUG, "[%p]", thiz);
  
  std::lock_guard<std::mutex> lock(thiz->mutex_);
  DiscoverContext *context;
  OCRepPayload *payload;
  bool found;
  OCRepPayload *definition = NULL;
  OCRepPayload *path = NULL;
  OCRepPayload *outPayload = NULL;
  thiz->GetContextAndRepPayload(handle, response, &context, &payload);
  if (!context)
  {
    goto exit;
  }
  
  found = false;
  for (OCRepPayloadValue *d = context->definitions->values; d; d = d->next)
  {
    if (context->it.GetResourceType() == d->name)
    {
      found = true;
      break;
    }
  }
  if (!found)
  {
    std::string resource_type = context->it.GetResourceType();
    // The definition of a resource type has the union of all possible interfaces listed
    std::set<std::string> ifSet;
    for (DiscoverContext::Iterator it = context->Begin(); it != context->End(); ++it)
    {
      Resource &r = it.GetResource();
      if (HasResourceType(r.rts_, resource_type))
      {
        ifSet.insert(r.ifs_.begin(), r.ifs_.end());
      }
    }
    std::vector<std::string> interfaces(ifSet.begin(), ifSet.end());
    definition = IntrospectDefinition(payload, resource_type, interfaces);
    if (!OCRepPayloadSetPropObjectAsOwner(context->definitions, resource_type.c_str(), definition))
    {
      goto exit;
    }
    definition = NULL;
  }
  
  found = false;
  for (OCRepPayloadValue *p = context->paths->values; p; p = p->next)
  {
    if (context->it.GetResource().uri_ == p->name)
    {
      found = true;
      break;
    }
  }
  if (!found)
  {
    path = IntrospectPath(context->it.GetResource().rts_, context->it.GetResource().ifs_);
    if (!OCRepPayloadSetPropObjectAsOwner(context->paths, context->it.GetResource().uri_.c_str(), path))
    {
      goto exit;
    }
    path = NULL;
  }
  
  if (++context->it != context->End())
  {
    OCStackResult result = thiz->ContinueDiscovery(context, context->it.GetUri().c_str(), context->it.GetDevAddrs(), Bridge::GetCB);
    if (result == OC_STACK_OK)
    {
      context = NULL;
    }
  }
  else
  {
    outPayload = OCRepPayloadCreate();
    if (!outPayload)
    {
      LOG(LOG_ERR, "Failed to create payload");
      goto exit;
    }
    if (!OCRepPayloadSetPropObjectAsOwner(outPayload, "paths", context->paths))
    {
      goto exit;
    }
    context->paths = NULL;
    if (!OCRepPayloadSetPropObjectAsOwner(outPayload, "definitions", context->definitions))
    {
      goto exit;
    }
    context->definitions = NULL;
    thiz->ParseIntrospectionPayload(context, outPayload);
  }
  
exit:
  OCRepPayloadDestroy(outPayload);
  OCRepPayloadDestroy(path);
  OCRepPayloadDestroy(definition);
  thiz->discovered_.erase(handle);
  delete context;
  return OC_STACK_DELETE_TRANSACTION;
}

bool Bridge::ParseIntrospectionPayload(DiscoverContext *context, OCRepPayload *payload)
{
  OCPresence *presence = NULL;
  bool success = ::ParseIntrospectionPayload(&context->device, payload);
  if (success)
  {
    //QStatus status;
    presence = new OCPresence(context->device.di_.c_str(), OCF_DISCOVER_PERIOD_SECS);
    if (!presence)
    {
      LOG(LOG_ERR, "new OCPresence() failed");
      goto exit;
    }
    presence_.push_back(presence);
    presence = NULL; // presence now belongs to this 
    /*status = context->bus_->Announce();
    if (status != ER_OK)
    {
      LOG(LOG_ERR, "Announce() failed - %s", QCC_StatusText(status));
      goto exit;
    }*/
  }
exit:
  delete presence;
  return success;
  
}

static const char *SeenStateText[] = { "NOT_SEEN", "SEEN_NATIVE", "SEEN_VIRTUAL" };

// Called with mutex_ held.
Bridge::SeenState Bridge::GetSeenState(const char *piid)
{
  SeenState state = NOT_SEEN;
  
  // Check what we've seen on the HF side.
  if (piid && (NOT_SEEN == state))
  {
    state = seen_state_cb_(piid);
  }
  
  // Check what we've seen on the OC side.
  if (piid && (NOT_SEEN == state))
  {
    
  }
  
  LOG(LOG_DEBUG, "piid=%s,state=%s", piid, SeenStateText[state]);
  return state;
}

// Called with mutex_ held.
void Bridge::DiscoverTask::Run(Bridge *thiz)
{
  OCStackResult result = OC_STACK_ERROR;
  DiscoverContext *ctx = NULL;
  bool is_virtual;
  for (std::map<OCDoHandle, DiscoverContext *>::iterator it = thiz->discovered_.begin();
    it != thiz->discovered_.end(); ++it)
  {
    if (it->second == context)
    {
      ctx = it->second;
    }
  }
  if (!ctx)
  {
    goto exit;
  }

  is_virtual = ctx->device.IsVirtual();
  switch (thiz->GetSeenState(piid.c_str()))
  {
    case NOT_SEEN:
      break;
    case SEEN_NATIVE:
      // Do nothing
      break;
    case SEEN_VIRTUAL:
      if (is_virtual)
      {
        // Do nothing
      }
      else
      {
        thiz->DestroyPiid(piid.c_str());
      }
      break;
  }
  result = thiz->ContinueDiscovery(context, OC_RSRVD_PLATFORM_URI,
    context->GetDevAddrs(OC_RSRVD_PLATFORM_URI), Bridge::GetPlatformCB);
  if (result == OC_STACK_OK)
  {
    context = NULL;
  }
exit:
  delete context;
}

/* Called with mutex_ held. */
void Bridge::DestroyPiid(const char *piid)
{
  /* Destroy virtual OC devices */
  kill_cb_(piid);
  
  /* Destroy virtual HF devices */
}

void Bridge::RDPublish(void *context)
{
  Bridge *thiz = reinterpret_cast<Bridge *>(context);
  std::lock_guard<std::mutex> lock(thiz->mutex_);
  if (thiz->rd_publish_task_)
  {
    // Delay the pending publication to give time for multiple resources to be created.
    thiz->rd_publish_task_->tick = time(NULL) + 1;
  }
  else
  {
    thiz->rd_publish_task_ = new RDPublishTask(time(NULL) + 1);
    thiz->tasks_.push_back(thiz->rd_publish_task_);
  }
}

// Called with mutex_ held.
void Bridge::SetIntrospectionData(/* HF Data */const char *title, const char *version)
{
  LOG(LOG_DEBUG, "[%p]", this);
  
  OCPersistentStorage *persistent_storage_handler = OCGetPersistentStorageHandler();
  assert(persistent_storage_handler);
  size_t cur_size = 1024;
  uint8_t *out = NULL;
  CborError err;
  FILE *file = NULL;
  size_t ret;
  for (;;)
  {
    out = (uint8_t *) OICCalloc(1, cur_size);
    if (!out)
    {
      LOG(LOG_ERR, "Failed to allocate introspection data buffer");
      goto exit;
    }
    err = Introspect(/* HF Data */title, version, out, &cur_size);
    if (err != CborErrorOutOfMemory)
    {
      break;
    }
    OICFree(out);
  }
  file = persistent_storage_handler->open(OC_INTROSPECTION_FILE_NAME, "wb");
  if (!file)
  {
    LOG(LOG_ERR, "open failed");
    goto exit;
  }
  ret = persistent_storage_handler->write(out, 1, cur_size, file);
  if (ret != cur_size)
  {
    LOG(LOG_ERR, "write failed");
    goto exit;
  }
exit:
  if (file)
  {
    persistent_storage_handler->close(file);
  }
  OICFree(out);
}

// Called with mutex_ held.
void Bridge::RDPublishTask::Run(Bridge *thiz)
{
  LOG(LOG_DEBUG, "[%p] thiz=%p", this, thiz);

  thiz->SetIntrospectionData(/* HF Info */"TITLE", "VERSION");
  ::RDPublish();
  thiz->rd_publish_task_ = NULL;
}

void Bridge::GetDeviceTableCB(void *ctx,
                              uint8_t dev_index,
                              uint8_t no_of_devices,
                              uint16_t *dev_ids,
                              uint8_t **dev_ipuis,
                              uint8_t **dev_emcs)
{
  Bridge *thiz = reinterpret_cast<Bridge *>(ctx);
  //std::lock_guard<std::mutex> lock(thiz->mutex_);
  LOG(LOG_TRACE, "DevIndex: %d, NoOfDevices: %d", dev_index, no_of_devices);

  if (thiz->sender_ == 0)
  {
    for (int i = 0; i < no_of_devices; ++i)
    {
      LOG(LOG_TRACE, "%d", dev_ids[i]);
    
      char piid[UUID_STRING_SIZE];
      if (GetProtocolIndependentId(dev_ipuis[i], dev_emcs[i], piid))
      {
        // TODO: Check if virtual (phase 2)
        bool is_virtual = false;

        switch (thiz->GetSeenState(piid))
        {
          case NOT_SEEN:
            if (is_virtual)
            {
              // TODO (phase 2)
            }
            else
            {
              thiz->exec_cb_(piid, /*dev_ids[i]*/dev_index + i + 1, thiz->secure_mode_->GetSecureMode(), is_virtual);
            }
            break;
          case SEEN_NATIVE:
            /* Do nothing */
            break;
          case SEEN_VIRTUAL:
            if (is_virtual)
            {
              /* Do nothing */
            }
            else
            {
              
              thiz->DestroyPiid(piid);
              thiz->exec_cb_(piid, /*dev_ids[i]*/dev_index + i + 1, thiz->secure_mode_->GetSecureMode(), is_virtual);
            }
            break;
        }
      }
      else
      {
        LOG(LOG_ERR, "Cannot retrieve piid. ipui: %p, emc: %p", dev_ipuis[i], dev_emcs[i]);
      }
    }

    if (no_of_devices == 5)
    {
      thiz->han_client_->get_device_table(dev_index + 5, 5, ctx);
    }
  }
  else
  {
    VirtualOcfDevice *device = new VirtualOcfDevice(dev_ids[0]);
    thiz->virtual_ocf_devices_.push_back(device);
    thiz->presence_.push_back(new HFPresence(dev_ids[0]));

    if (device->SetProperties(dev_ipuis[0], dev_emcs[0]) != OC_STACK_OK)
    {
      delete device;
    }

    VirtualResource *resource = thiz->CreateVirtualResource(dev_ids[0], "/example");
    if (resource)
    {
      thiz->virtual_resources_.push_back(resource);
    }

    thiz->han_state_ = RUNNING;
  }
}