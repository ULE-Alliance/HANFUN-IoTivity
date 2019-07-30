#include "virtual_resource.h"

#include "log.h"

VirtualResource *VirtualResource::Create(uint16_t address, const char *path, CreateCB create_callback, void *create_context)
{
  VirtualResource *resource = new VirtualResource(address, path, create_callback, create_context);
  OCStackResult result = resource->Create();
  if (result != OC_STACK_OK)
  {
    delete resource;
    resource = NULL;
  }
  return resource;
}

VirtualResource::VirtualResource(uint16_t address, const char *path, CreateCB create_callback, void *create_context)
  : create_callback_(create_callback), create_context_(create_context)
{
  (void) path;
  LOG(LOG_DEBUG, "[%p]", this);
}

VirtualResource::~VirtualResource()
{
  LOG(LOG_DEBUG, "[%p]", this);

  OCResourceHandle handle;
  while ((handle = OCGetResourceHandleFromCollection(handle_, 0)))
  {
    OCUnBindResource(handle_, handle);
    OCDeleteResource(handle);
  }
}

uint16_t VirtualResource::address()
{
  return address_;
}

OCStackResult VirtualResource::Create()
{
  std::lock_guard<std::mutex> lock(mutex_);

  create_callback_(create_context_);

  return OC_STACK_OK;
}
