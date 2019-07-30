#include "bridge.h"
#include "log.h"
#include "plugin.h"

#include "ocstack.h"
#include "rd_client.h"
#include "rd_server.h"
#include "uv.h"
#include <chrono>
#include <signal.h>
#include <sstream>
#include <stdlib.h>
#include <thread>
#include <unistd.h>

static volatile sig_atomic_t kQuitFlag = false;
static volatile sig_atomic_t kResetSecurityFlag = false;
static const char *kPersistentStoragePrefix = "HanFunBridge_";
static const char *kUidPrefix = "hf://node.bridge.com/";
static const char *kUuid = NULL;
static uint16_t kSenderAddress = 0;
static const char *kResourceDirectoryDi = NULL;
#if __WITH_DTLS__
static bool kSecureMode = true;
#else
static bool kSecureMode = false;
#endif

class OC
{
  public:
    bool Start()
    {
      thread_ = std::thread(OC::Process);
      return true;
    }
    void Stop()
    {
      if (kSenderAddress != 0)
      {
        bool done = false;
        OCCallbackData callback_data;
        callback_data.cb = OC::RDDeleteCB;
        callback_data.context = &done;
        callback_data.cd = NULL;
        // Delete RD resource from Resource Directory
        OCStackResult result = OCRDDelete(NULL, kResourceDirectory.c_str(), CT_DEFAULT, NULL, 0, &callback_data, OC_HIGH_QOS);
        while (!done)
        {
          result = OCProcess();
          if (result != OC_STACK_OK)
          {
            break;
          }

          std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
      }
      else
      {
        // Stop Resource Directory and remove all published.
        OCRDStop();
      }
      // Stop OC stack.
      OCStop();
      thread_.join();
    }
  private:
    std::thread thread_;
    static void Process()
    {
      while (!kQuitFlag)
      {
        // Allows low-level processing of stack services.
        // It has to be called in main loop of OC client or server.
        OCStackResult result = OCProcess();
        if (result != OC_STACK_OK)
        {
          fprintf(stderr, "OCProcess - %d\n", result);
          break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      }
    }
    static OCStackApplicationResult RDDeleteCB(void *context, OCDoHandle handle, OCClientResponse *response)
    {
      UNUSED(handle);

      bool *done = (bool *) context;
      LOG(LOG_DEBUG,"response=%p,response->result=%d", response, response ? response->result : 0);
      *done = true;
      return OC_STACK_DELETE_TRANSACTION;
    }
};

class HanFun
{
  public:
    bool Start()
    {
      thread_ = std::thread(HanFun::Process);
      return true;
    }
    
    void Stop()
    {
      thread_.detach();
      uv_stop(uv_default_loop());
      //thread_.join();
    }
  private:
    std::thread thread_;
    static void Process()
    {
      /* execute all tasks in queue */
      uv_run(uv_default_loop(), UV_RUN_DEFAULT);
    }
};

// Callback for Ctrl+C (interrupt) signal
//
// @param sig
//
static void SigIntCB(int sig)
{
  UNUSED(sig);
  kQuitFlag = true;
}

// Callback for user defined signal. It is used to reset OCF Security.
//
// @param sig
//
static void SigUsr1CB(int sig)
{
  UNUSED(sig);
  kResetSecurityFlag = true;
}

static std::string GetFilename(const char *uuid, const char *suffix)
{
  std::string path = kPersistentStoragePrefix;
  if (uuid)
  {
    path += std::string(uuid) + "_";
  }
  if (suffix)
  {
    path += suffix;
  }
  return path;
}

// Callback for OCF persistent storage opening
//
// @param suffix
// @param mode
//
static FILE *PSOpenCB(const char *suffix, const char *mode)
{
  std::string path = GetFilename(kUuid, suffix);
  return fopen(path.c_str(), mode);
}

// Callback for Resource Directory discovery by Plugins
//
// @param context
// @param handle
// @param response
//
static OCStackApplicationResult DiscoverResourceDirectoryCB(void *context, OCDoHandle handle, OCClientResponse *response)
{
  UNUSED(context);
  UNUSED(handle);
  
  if (!response || !response->payload || response->result != OC_STACK_OK)
  {
    return OC_STACK_KEEP_TRANSACTION;
  }
  if (response->payload->type != PAYLOAD_TYPE_DISCOVERY)
  {
    return OC_STACK_KEEP_TRANSACTION;
  }
  OCDiscoveryPayload *payload = (OCDiscoveryPayload *) response->payload;
  if (kResourceDirectoryDi && !strcmp(payload->sid, kResourceDirectoryDi))
  {
    std::ostringstream oss;
    if (response->devAddr.flags & OC_IP_USE_V6)
    {
      oss << "[";
      for (size_t i = 0; response->devAddr.addr[i]; ++i)
      {
        if (response->devAddr.addr[i] != '%')
        {
          oss << response->devAddr.addr[i];
        }
        else
        {
          oss << "%25";
        }
      }
      oss << "]";
    }
    else
    {
      oss << response->devAddr.addr;
    }
    oss << ":" << response->devAddr.port;
    kResourceDirectory = oss.str();
    return OC_STACK_DELETE_TRANSACTION;
  }
  else
  {
    return OC_STACK_KEEP_TRANSACTION;
  }
}

// Callback for Plugin execution by PluginManager
//
// @param uuid
// @param sender
// @param secure_mode
// @param is_virtual
//
static void ExecCB(const char *uuid, uint16_t sender, bool secure_mode, bool is_virtual)
{
  printf("exec --ps %s --uuid %s --sender %u --rd %s --secureMode %s %s\n", kPersistentStoragePrefix, uuid,
          sender, OCGetServerInstanceIDString(), secure_mode ? "true" : "false",
          is_virtual ? "--virtual" : "");
  fflush(stdout);
}

// Callback for Plugin kill by PluginManager
//
// @param uuid
//
static void KillCB(const char *uuid)
{
  printf("kill --uuid %s\n", uuid);
  fflush(stdout);
}

// Callback to check if a device is native or virtual
//
// @param uuid
//
static Bridge::SeenState GetSeenStateCB(const char *uuid)
{
  std::string seen_path = GetFilename(uuid, "seen.state");
  FILE *fp = fopen(seen_path.c_str(), "r");
  if (!fp)
  {
    return Bridge::NOT_SEEN;
  }
  char state_str[16];
  fscanf(fp, "%s", state_str);
  fclose(fp);
  if (!strcmp("virtual", state_str))
  {
    return Bridge::SEEN_VIRTUAL;
  }
  else
  {
    return Bridge::SEEN_NATIVE;
  }
}

static void DisconnectedCB()
{
  LOG(LOG_TRACE, "DisconnectedCB");
  kQuitFlag = true;
}

int main(int argc, char **argv)
{
  int ret = EXIT_FAILURE;
  Bridge *bridge = NULL;
  OC *oc = NULL;
  HanFun *hf = NULL;
  std::string db_filename;
  OCStackResult result;
  OCPersistentStorage ps_handler = { PSOpenCB, fread, fwrite, fclose, unlink };
  
  int protocols = 0;
  bool is_virtual = false;
  // Parameters used for each Plugin process call
  if (argc > 1)
  {
    for (int i = 1; i < argc; ++i)
    {
      if (!strcmp(argv[i], "--ps") && (i < (argc - 1)))
      {
        kPersistentStoragePrefix = argv[++i];
      }
      else if (!strcmp(argv[i], "--hf"))
      {
        protocols |= Bridge::HF;
      }
      else if (!strcmp(argv[i], "--oc"))
      {
        protocols |= Bridge::OC;
      }
      else if (!strcmp(argv[i], "--uuid") && (i < (argc -1)))
      {
        kUuid = argv[++i];
      }
      else if (!strcmp(argv[i], "--sender") && (i < (argc - 1)))
      {
        char *endptr;
        kSenderAddress = strtol(argv[++i], &endptr, 10);
      }
      else if (!strcmp(argv[i], "--rd") && (i < (argc - 1)))
      {
        kResourceDirectoryDi = argv[++i];
      }
      else if (!strcmp(argv[i], "--virtual"))
      {
        is_virtual = true;
      }
      else if (!strcmp(argv[i], "--secureMode") && (i < (argc - 1)))
      {
        char *mode = argv[++i];
        if (!strcmp(mode, "false"))
        {
          kSecureMode = false;
        }
        else if (!strcmp(mode, "true"))
        {
          kSecureMode = true;
        }
      }
    }
  }
  // uuid, sender and rd must be supplied together. When they are, hf and oc are ignored
  if (protocols == 0)
  {
    protocols = Bridge::HF | Bridge::OC;
  }
  // If a Plugin is being executed, store its state (native or virtual)
  std::string seen_path = GetFilename(kUuid, "seen.state");
  if (kSenderAddress != 0)
  {
    FILE *fp = fopen(seen_path.c_str(), "w");
    fprintf(fp, "%s", is_virtual ? "virtual" : "native");
    fclose(fp);
  }
  
  // signal handling
  signal(SIGINT, SigIntCB);
#ifdef SIGUSR1
  signal(SIGUSR1, SigUsr1CB);
#endif
  
  result = OCRegisterPersistentStorageHandler(&ps_handler);
  if (result != OC_STACK_OK)
  {
    fprintf(stderr, "OCRegisterPersistentStorageHandler - %d\n", result);
    goto exit;
  }
  db_filename = GetFilename(NULL, "RD.db");
  result = OCRDDatabaseSetStorageFilename(db_filename.c_str());
  if (result != OC_STACK_OK)
  {
    fprintf(stderr, "OCRDDatabaseSetStorageFilename - %d\n", result);
    goto exit;
  }
  result = OCInit1(OC_CLIENT_SERVER, OC_DEFAULT_FLAGS, OC_DEFAULT_FLAGS);
  if (result != OC_STACK_OK)
  {
    fprintf(stderr, "OCInit1 - %d\n", result);
    goto exit;
  }
  if (kSenderAddress != 0)
  {
    result = OCStopMulticastServer();
    if (result != OC_STACK_OK)
    {
      fprintf(stderr, "OCStopMulticastServer - %d\n", result);
      goto exit;
    }
    OCDoHandle handle;
    OCCallbackData callback_data;
    callback_data.cb = DiscoverResourceDirectoryCB;
    callback_data.context = NULL;
    callback_data.cd = NULL;
    result = OCDoResource(&handle, OC_REST_DISCOVER, "/oic/res?rt=oic.wk.rd", NULL, 0, CT_DEFAULT, OC_HIGH_QOS, &callback_data, NULL, 0);
    if (result != OC_STACK_OK)
    {
      fprintf(stderr, "DoResource(OC_REST_DISCOVER) - %d\n", result);
      goto exit;
    }
    while (!kQuitFlag && kResourceDirectory.empty())
    {
      result = OCProcess();
      if (result != OC_STACK_OK)
      {
        fprintf(stderr, "OCProcess - %d\n", result);
        goto exit;
      }
    }
  }
  else
  {
    result = OCRDStart();
    if (result != OC_STACK_OK)
    {
      fprintf(stderr, "OCRDStart() - %d\n", result);
      goto exit;
    }
  }
  
  if (kUuid && (kSenderAddress != 0))
  {
    bridge = new Bridge(kUidPrefix, kSenderAddress);
    bridge->SetDisconnectedCB(DisconnectedCB);
  }
  else
  {
    bridge = new Bridge(kUidPrefix, (Bridge::Protocol) protocols);
    bridge->SetProcessCB(ExecCB, KillCB, GetSeenStateCB);
  }
  bridge->SetDeviceName("HAN-FUN Bridge");
  bridge->SetManufacturerName("DEKRA Testing and Certification, S.A.U.");
  bridge->SetSecureMode(kSecureMode);
  if (!bridge->Start())
  {
    goto exit;
  }
  // Start OCF thread for processing
  oc = new OC();
  if (!oc->Start())
  {
    goto exit;
  }
  // Start HAN-FUN thread for processing
  hf = new HanFun();
  if (!hf->Start())
  {
    goto exit;
  }
  while (!kQuitFlag)
  {
    if (kResetSecurityFlag)
    {
      kResetSecurityFlag = false;
      bridge->ResetSecurity();
    }
    if (!bridge->Process())
    {
      goto exit;
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
    
  ret = EXIT_SUCCESS;
  
exit:
  
  if (bridge)
  {
    bridge->Stop();
    delete bridge;
  }
  if (hf)
  {
    hf->Stop();
    delete hf;
  }
  if (oc)
  {
    oc->Stop();
    if (kSenderAddress != 0)
    {
      remove(seen_path.c_str());
    }
    delete oc;
  }
  
  return ret;
}
