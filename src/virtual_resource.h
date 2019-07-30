#ifndef _VIRTUALRESOURCE_H
#define _VIRTUALRESOURCE_H

#include "ocstack.h"
#include <mutex>

class VirtualResource
{
  public:
    typedef void (*CreateCB)(void *context);
    static VirtualResource *Create(uint16_t address, const char *path, CreateCB create_callback, void *create_context);
    virtual ~VirtualResource();

    uint16_t address();

  protected:
    std::mutex mutex_;
    uint16_t address_;
    CreateCB create_callback_;
    void *create_context_;

    VirtualResource(uint16_t address, const char *path, CreateCB create_callback, void *create_context);

  private:
    OCResourceHandle handle_;

    OCStackResult Create();
};

#endif // _VIRTUALRESOURCE_H