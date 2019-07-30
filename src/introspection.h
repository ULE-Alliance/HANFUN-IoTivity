#ifndef _INTROSPECTION_H
#define _INTROSPECTION_H

#include "cbor.h"
#include "ocpayload.h"
#include <string>
#include <vector>

class Device;
class VirtualHfDevice;

/*
 * Creates CBOR-encoded introspection data from the supplied ***HF Data***
 * 
 * @param[in] title
 * @param[in] version
 * @param[out] out
 * @param[out] out_size
 * @return
 */
CborError Introspect(/* HF Data */const char *title, const char *version, uint8_t *out, size_t *out_size);

/*
 * Creates an introspection definition object from a GET request payload.
 * 
 * @param[in] payload       a response payload to create the schema from
 * @param[in] resource_type the resource type of payload
 * @param[in] interfaces    the interfaces implemented by the resource
 */
OCRepPayload *IntrospectDefinition(OCRepPayload *payload, std::string resource_type,
  std::vector<std::string> &interfaces);

/*
 * Creates an introspection path object from the supplied resource types and interfaces.
 * 
 * @param[in] resource_types  the resource types implemented by the resource
 * @param[in] interfaces      the interfaces implemented by the resource
 */ 
OCRepPayload *IntrospectPath(std::vector<std::string> &resource_types,
  std::vector<std::string> &interfaces);

/*
 * @param[in] device        the device the OC introspection data is from
 * @param[in] payload       the OC introspection data
 */
bool ParseIntrospectionPayload(Device *device, const OCRepPayload *payload);

#endif
