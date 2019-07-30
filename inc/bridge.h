#ifndef _BRIDGE_H
#define _BRIDGE_H

#include "hanfun.h"
#include "han_client.h"
#include "ocpayload.h"
#include "octypes.h"
#include "uv.h"
#include <condition_variable>
#include <list>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <vector>

class OCSecurity;
class Presence;
class SecureModeResource;
class RegistrationResource;
class VirtualOcfDevice;
class VirtualResource;

class Bridge
{
  public:
    enum Protocol
    {
      HF = (1 << 0),
      OC = (1 << 1),
    };
    
    Bridge(const std::string &base_uri, Protocol protocols);
    Bridge(const std::string &base_uri, uint16_t sender);
    virtual ~Bridge();
    
    typedef void (*ExecCB)(const char *piid, uint16_t sender, bool secure_mode, bool is_virtual);
    typedef void (*KillCB)(const char *piid);
    typedef enum { NOT_SEEN = 0, SEEN_NATIVE, SEEN_VIRTUAL } SeenState;
    typedef SeenState (*GetSeenStateCB)(const char *piid);
    typedef void (*DisconnectedCB)();

    void SetProcessCB(ExecCB exec_cb, KillCB kill_cb, GetSeenStateCB seen_state_cb)
    {
      exec_cb_ = exec_cb;
      kill_cb_ = kill_cb;
      seen_state_cb_ = seen_state_cb;
    }
    void SetDisconnectedCB(DisconnectedCB cb)
    {
      disconnected_cb_ = cb;
    }
    void SetDeviceName(const char *device_name)
    {
      device_name_ = device_name;
    }
    void SetManufacturerName(const char* manufacturer_name)
    {
      manufacturer_name_ = manufacturer_name;
    }
    void SetSecureMode(bool secure_mode);
    
    bool Start();
    bool Stop();
    void ResetSecurity();
    bool Process();
    
  private:
  
    struct DiscoverContext;
    struct Task
    {
      time_t tick;
      Task(time_t tick) : tick(tick) {}
      virtual ~Task() {}
      virtual void Run(Bridge *thiz) = 0;
    };
    struct DiscoverTask : public Task {
      std::string piid;
      OCRepPayload *payload;
      DiscoverContext *context;
      DiscoverTask(time_t tick, const char *piid, OCRepPayload *payload,
        DiscoverContext *context) : Task(tick), piid(piid),
        payload(OCRepPayloadClone(payload)), context(context) {}
      virtual ~DiscoverTask() { OCRepPayloadDestroy(payload); }
      virtual void Run(Bridge *thiz);
    };
    struct RDPublishTask : public Task {
      RDPublishTask(time_t tick) : Task(tick) {}
      virtual ~RDPublishTask() {}
      virtual void Run(Bridge *thiz);
    };
  
    static const time_t OCF_DISCOVER_PERIOD_SECS = 5;
    static const time_t HF_DISCOVER_PERIOD_SECS = 30;
  
    ExecCB exec_cb_;
    KillCB kill_cb_;
    GetSeenStateCB seen_state_cb_;
    DisconnectedCB disconnected_cb_;
    
    std::mutex mutex_;
    std::condition_variable cond_;
    Protocol protocols_;
    enum { CREATED, STARTED, RUNNING } han_state_;
    uint16_t sender_;
    HanClient *han_client_;
    OCSecurity *oc_security_;
    OCDoHandle discover_handle_;
    time_t discover_next_tick_;
    std::vector<Presence *> presence_;
    std::vector<VirtualOcfDevice *> virtual_ocf_devices_;
    std::vector<VirtualResource *> virtual_resources_;
    std::map<OCDoHandle, DiscoverContext *> discovered_;
    SecureModeResource *secure_mode_;
    RegistrationResource *registration_;
    std::list<Task *> tasks_;
    RDPublishTask *rd_publish_task_;
    size_t pending_;
    std::string device_name_;
    std::string manufacturer_name_;
    time_t get_devices_next_tick_;
    
    static void RDPublish(void *context);
    void SetIntrospectionData(/* HF Data */const char *title, const char *version);
    void Destroy(const char *id);
    void Destroy(uint16_t id);
    VirtualResource *CreateVirtualResource(uint16_t address, const char *path);
    
    static OCStackApplicationResult DiscoverCB(void *context, OCDoHandle handle, OCClientResponse *response);
    static OCStackApplicationResult GetCollectionCB(void *context, OCDoHandle handle, OCClientResponse *response);
    static OCStackApplicationResult GetPlatformCB(void *context, OCDoHandle handle, OCClientResponse *response);
    static OCStackApplicationResult GetPlatformConfigurationCB(void *context, OCDoHandle handle, OCClientResponse *response);
    static OCStackApplicationResult GetDeviceCB(void *context, OCDoHandle handle, OCClientResponse *response);
    static OCStackApplicationResult GetDeviceConfigurationCB(void *context, OCDoHandle handle, OCClientResponse *response);
    static OCStackApplicationResult GetCB(void *ctx, OCDoHandle handle, OCClientResponse *response);
    static OCStackApplicationResult GetIntrospectionCB(void *ctx, OCDoHandle handle, OCClientResponse *response);
    static OCStackApplicationResult GetIntrospectionDataCB(void *ctx, OCDoHandle handle, OCClientResponse *response);
    
    static void GetDeviceTableCB(void* ctx,
                                 uint8_t dev_index,
                                 uint8_t no_of_devices,
                                 uint16_t *dev_ids,
                                 uint8_t **dev_ipuis,
                                 uint8_t **dev_emcs);

    SeenState GetSeenState(const char *piid);
    void DestroyPiid(const char *piid);
    
    bool IsSelf(const OCDiscoveryPayload *payload);
    bool HasSeenBefore(const OCDiscoveryPayload *payload);
    bool IsSecure(const OCResourcePayload *resource);
    bool IsVirtual(HF::Attributes::List *attributes);
    bool HasTranslatableResource(OCDiscoveryPayload *payload);
    void UpdatePresenceStatus(const OCDiscoveryPayload *payload);
    void GetContextAndRepPayload(OCDoHandle handle, OCClientResponse *response, DiscoverContext **context, OCRepPayload **payload);
    bool ParseIntrospectionPayload(DiscoverContext *context, OCRepPayload *payload);
    OCStackResult GetIntrospection(DiscoverContext *context);
    OCStackResult GetCollection(DiscoverContext *context);
    OCStackResult GetPlatformConfiguration(DiscoverContext *context);
    OCStackResult ContinueDiscovery(DiscoverContext *context, const char *uri, const std::vector<OCDevAddr> &addrs, OCClientResponseHandler cb);
    OCStackResult ContinueDiscovery(DiscoverContext *context, const char *uri, OCDevAddr *addr, OCClientResponseHandler cb);
    
    OCStackResult DoResource(OCDoHandle *handle, OCMethod method, const char *uri, OCDevAddr *addr, OCClientResponseHandler cb);
    OCStackResult DoResource(OCDoHandle *handle, OCMethod method, const char *uri, const std::vector<OCDevAddr> &addrs, OCClientResponseHandler cb);
};

#endif
