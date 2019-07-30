#include "security.h"

#include "log.h"
#include "plugin.h"

#include "ocprovisioningmanager.h"
#include "pinoxmcommon.h"

// TODO: Add HF Security implementation

bool OCSecurity::Init()
{
#if __WITH_DTLS__
  OCStackResult result = SetDisplayPinWithContextCB(OCSecurity::DisplayPinCB, this);
  if (result != OC_STACK_OK)
  {
    LOG(LOG_ERR, "SetDisplayPinWithContextCB() - %d", result);
    return false;
  }
  SetClosePinDisplayCB(OCSecurity::ClosePinDisplayCB);
  result = SetRandomPinPolicy(OXM_RANDOM_PIN_DEFAULT_SIZE,
          (OicSecPinType_t) OXM_RANDOM_PIN_DEFAULT_PIN_TYPE);
  if (result != OC_STACK_OK)
  {
    LOG(LOG_ERR, "SetRandomPinPolicy() - %d", result);
    return false;
  }
#endif
  return true;
}

void OCSecurity::Reset()
{
#if __WITH_DTLS__
  LOG(LOG_DEBUG, "");
  OCStackResult result = OCResetSVRDB();
  if (result != OC_STACK_OK)
  {
    LOG(LOG_ERR, "OCResetSVRDB() - %d", result);
  }
#endif
}

void OCSecurity::DisplayPinCB(char *pin, size_t pin_size, void *context)
{
  LOG(LOG_INFO, "pin=%s,pinSize=%d,context=%p", pin, pin_size, context);
}

void OCSecurity::ClosePinDisplayCB()
{
  LOG(LOG_INFO, "");
}
