#ifndef _PLUGIN_H
#define _PLUGIN_H

#include "octypes.h"
#include <string>

#ifndef UNUSED
#define UNUSED(x) (void)x;
#endif

// Global Resource Directory address
extern std::string kResourceDirectory;

// Publish RD resources to Resource Directory.
OCStackResult RDPublish();

#endif // _PLUGIN_H
