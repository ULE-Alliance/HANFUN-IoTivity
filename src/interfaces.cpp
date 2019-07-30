#include "interfaces.h"

#include "device_configuration_resource.h"
#include "octypes.h"
#include "platform_configuration_resource.h"
#include <string.h>

bool TranslateResourceType(const char *name)
{
  const char *do_not_translate[] = {
    "oic.d.bridge",
    OC_RSRVD_RESOURCE_TYPE_COLLECTION, OC_RSRVD_RESOURCE_TYPE_INTROSPECTION,
    OC_RSRVD_RESOURCE_TYPE_RD, OC_RSRVD_RESOURCE_TYPE_RDPUBLISH, OC_RSRVD_RESOURCE_TYPE_RES,
    "oic.r.hanfunobject", "oic.r.acl", "oic.r.acl2", "oic.r.amacl", "oic.r.cred", "oic.r.crl",
    "oic.r.csr", "oic.r.doxm", "oic.r.pstat", "oic.r.roles", "oic.r.securemode" 
  };
  for (size_t i = 0; i < sizeof(do_not_translate)/sizeof(do_not_translate[0]); ++i)
  {
    if (!strcmp(do_not_translate[i], name))
    {
      return false;
    }
  }
  const char *do_deep_translation[] = {
    OC_RSRVD_RESOURCE_TYPE_DEVICE, OC_RSRVD_RESOURCE_TYPE_DEVICE_CONFIGURATION,
    OC_RSRVD_RESOURCE_TYPE_MAINTENANCE, OC_RSRVD_RESOURCE_TYPE_PLATFORM,
    OC_RSRVD_RESOURCE_TYPE_PLATFORM_CONFIGURATION
  };
  for (size_t i = 0; i < sizeof(do_deep_translation)/sizeof(do_deep_translation[0]); ++i)
  {
    if (!strcmp(do_deep_translation[i], name))
    {
      return true;
    }
  }
  //return !IsResourceTypeInWellDefinedSet(name);
  return true;
}