#ifndef _SECURITY_H
#define _SECURITY_H

#include "cacommon.h"

// TODO: Add HF Security class

class OCSecurity
{
  public:
    bool Init();
    void Reset();
    
  private:
    static void DisplayPinCB(char *pin, size_t pin_size, void *context);
    static void ClosePinDisplayCB();
};

#endif
