#include "introspection.h"

#include "log.h"
#include "oic_malloc.h"
#include "oic_string.h"
#include "ocstack.h"

#define OC_RSRVD_INTROSPECTION_URI_PATH "/introspection"

#define VERIFY_CBOR(err)                                          \
  if ((CborNoError != (err)) && (CborErrorOutOfMemory != (err)))  \
  {                                                               \
    goto exit;                                                    \
  }

static bool SetPropertiesSchema(OCRepPayload *parent, OCRepPayload *obj);

static int64_t Pair(CborEncoder *cbor, const char *key, const char *value)
{
  int64_t err = CborNoError;
  err |= cbor_encode_text_stringz(cbor, key);
  VERIFY_CBOR(err);
  err |= cbor_encode_text_stringz(cbor, value);
  VERIFY_CBOR(err);
exit:
  return err;
}

static int64_t Info(CborEncoder *cbor, const char *title, const char *version)
{
  int64_t err = CborNoError;
  CborEncoder info;
  err |= cbor_encode_text_stringz(cbor, "info");
  VERIFY_CBOR(err);
  err |= cbor_encoder_create_map(cbor, &info, CborIndefiniteLength);
  VERIFY_CBOR(err);
  err |= Pair(&info, "title", title);
  VERIFY_CBOR(err);
  err |= Pair(&info, "version", version);
  VERIFY_CBOR(err);
  err |= cbor_encoder_close_container(cbor, &info);
  VERIFY_CBOR(err);
exit:
  return err;
}

static int64_t Schemes(CborEncoder *cbor)
{
  int64_t err = CborNoError;
  CborEncoder schemes;
  err |= cbor_encode_text_stringz(cbor, "schemes");
  VERIFY_CBOR(err);
  err |= cbor_encoder_create_array(cbor, &schemes, 1);
  VERIFY_CBOR(err);
  err |= cbor_encode_text_stringz(&schemes, "http");
  VERIFY_CBOR(err);
  err |= cbor_encoder_close_container(cbor, &schemes);
  VERIFY_CBOR(err);
exit:
  return err;
}

static int64_t Consumes(CborEncoder *cbor)
{
  int64_t err = CborNoError;
  CborEncoder consumes;
  err |= cbor_encode_text_stringz(cbor, "consumes");
  VERIFY_CBOR(err);
  err |= cbor_encoder_create_array(cbor, &consumes, 1);
  VERIFY_CBOR(err);
  err |= cbor_encode_text_stringz(&consumes, "application/json");
  VERIFY_CBOR(err);
  err |= cbor_encoder_close_container(cbor, &consumes);
  VERIFY_CBOR(err);
exit:
  return err;
}

static int64_t Produces(CborEncoder *cbor)
{
  int64_t err = CborNoError;
  CborEncoder produces;
  err |= cbor_encode_text_stringz(cbor, "produces");
  VERIFY_CBOR(err);
  err |= cbor_encoder_create_array(cbor, &produces, 1);
  VERIFY_CBOR(err);
  err |= cbor_encode_text_stringz(&produces, "application/json");
  VERIFY_CBOR(err);
  err |= cbor_encoder_close_container(cbor, &produces);
  VERIFY_CBOR(err);
exit:
  return err;
}

static int64_t Paths(CborEncoder *cbor)
{
  OCStackResult result = OC_STACK_ERROR;
  int64_t err = CborNoError;
  CborEncoder paths;
  err |= cbor_encode_text_stringz(cbor, "paths");
  VERIFY_CBOR(err);
  err |= cbor_encoder_create_map(cbor, &paths, CborIndefiniteLength);
  VERIFY_CBOR(err);
  uint8_t nr;
  result = OCGetNumberOfResources(&nr);
  if (result != OC_STACK_OK)
  {
    err |= CborErrorInternalError;
    goto exit;
  }
  // TODO: Map paths
  err |= cbor_encoder_close_container(cbor, &paths);
  VERIFY_CBOR(err);
exit:
  return err;
}

static int64_t Definitions(CborEncoder *cbor)
{
  OCStackResult result = OC_STACK_ERROR;
  int64_t err = CborNoError;
  CborEncoder paths;
  err |= cbor_encode_text_stringz(cbor, "definitions");
  VERIFY_CBOR(err);
  err |= cbor_encoder_create_map(cbor, &paths, CborIndefiniteLength);
  VERIFY_CBOR(err);
  uint8_t nr;
  result = OCGetNumberOfResources(&nr);
  if (result != OC_STACK_OK)
  {
    err |= CborErrorInternalError;
    goto exit;
  }
  // TODO: Map definitions
  err |= cbor_encoder_close_container(cbor, &paths);
  VERIFY_CBOR(err);
exit:
  return err;
}

CborError Introspect(/* HF Data */const char *title, const char *version, uint8_t *out, size_t *out_size)
{
  int64_t err = CborNoError;
  CborEncoder encoder;
  cbor_encoder_init(&encoder, out, *out_size, 0);
  CborEncoder map;
  err |= cbor_encoder_create_map(&encoder, &map, CborIndefiniteLength);
  VERIFY_CBOR(err);
  err |= Pair(&map, "swagger", "2.0");
  VERIFY_CBOR(err);
  err |= Info(&map, title, version);
  VERIFY_CBOR(err);
  err |= Schemes(&map);
  VERIFY_CBOR(err);
  err |= Consumes(&map);
  VERIFY_CBOR(err);
  err |= Produces(&map);
  VERIFY_CBOR(err);
  err |= Paths(&map);
  VERIFY_CBOR(err);
  /*err |= Parameters(&map);
  VERIFY_CBOR(err);*/
  err |= Definitions(&map/*, bus, ajSoftwareVersion*/);
  VERIFY_CBOR(err);
  err |= cbor_encoder_close_container(&encoder, &map);
  VERIFY_CBOR(err);
  
exit:
  if (err == CborErrorOutOfMemory)
  {
    *out_size += cbor_encoder_get_extra_bytes_needed(&encoder);
  }
  else if (err == CborNoError)
  {
    *out_size = cbor_encoder_get_buffer_size(&encoder, out);
  }
  return (CborError)err;
}

static bool SetPropertiesSchema(OCRepPayload *property, OCRepPayloadPropType type, OCRepPayload *obj)
{
  OCRepPayload *child = NULL;
  bool success = false;

  switch (type)
  {
    case OCREP_PROP_NULL:
      success = false;
      break;
    case OCREP_PROP_INT:
      success = OCRepPayloadSetPropString(property, "type", "integer");
      break;
    case OCREP_PROP_DOUBLE:
      success = OCRepPayloadSetPropString(property, "type", "number");
      break;
    case OCREP_PROP_BOOL:
      success = OCRepPayloadSetPropString(property, "type", "boolean");
      break;
    case OCREP_PROP_STRING:
      success = OCRepPayloadSetPropString(property, "type", "string");
      break;
    case OCREP_PROP_BYTE_STRING:
      child = OCRepPayloadCreate();
      success = child &&
        OCRepPayloadSetPropString(child, "binaryEncoding", "base64") &&
        OCRepPayloadSetPropObjectAsOwner(property, "media", child) &&
        OCRepPayloadSetPropString(property, "type", "string");
      if (success)
      {
        child = NULL;
      }
      break;
    case OCREP_PROP_OBJECT:
      child = OCRepPayloadCreate();
      success = child &&
        SetPropertiesSchema(child, obj) &&
        OCRepPayloadSetPropObjectAsOwner(property, "properties", child) &&
        OCRepPayloadSetPropString(property, "type", "object");
      if (success)
      {
        child = NULL;
      }
      break;
    case OCREP_PROP_ARRAY:
      success = false;
      break;
  }
  if (!success)
  {
    goto exit;
  }
  success = true;

exit:
  OCRepPayloadDestroy(child);
  return success;
}

static bool SetPropertiesSchema(OCRepPayload *parent, OCRepPayload *obj)
{
  OCRepPayload *property = NULL;
  OCRepPayload *child = NULL;
  OCRepPayload *array = NULL;
  bool success = false;

  if (!obj)
  {
    success = true;
    goto exit;
  }
  for (OCRepPayloadValue *value = obj->values; value; value = value->next)
  {
    property = OCRepPayloadCreate();
    if (!property)
    {
      LOG(LOG_ERR, "Failed to create payload");
      success = false;
      goto exit;
    }
    switch (value->type)
    {
      case OCREP_PROP_NULL:
        success = false;
        break;
      case OCREP_PROP_INT:
        success = OCRepPayloadSetPropString(property, "type", "integer");
        break;
      case OCREP_PROP_DOUBLE:
        success = OCRepPayloadSetPropString(property, "type", "number");
        break;
      case OCREP_PROP_BOOL:
        success = OCRepPayloadSetPropString(property, "type", "boolean");
        break;
      case OCREP_PROP_STRING:
        success = OCRepPayloadSetPropString(property, "type", "string");
        break;
      case OCREP_PROP_BYTE_STRING:
        child = OCRepPayloadCreate();
        success = child &&
          OCRepPayloadSetPropString(child, "binaryEncoding", "base64") &&
          OCRepPayloadSetPropObjectAsOwner(property, "media", child) &&
          OCRepPayloadSetPropString(property, "type", "string");
        if (success)
        {
          child = NULL;
        }
        break;
      case OCREP_PROP_OBJECT:
        child = OCRepPayloadCreate();
        success = child &&
          SetPropertiesSchema(child, value->obj) &&
          OCRepPayloadSetPropObjectAsOwner(property, "properties", child) &&
          OCRepPayloadSetPropString(property, "type", "object");
        if (success)
        {
          child = NULL;
        }
        break;
      case OCREP_PROP_ARRAY:
        success = true;
        array = property;
        for (size_t i = 0; success && (i < MAX_REP_ARRAY_DEPTH) && value->arr.dimensions[i]; ++i)
        {
          child = OCRepPayloadCreate();
          success = child &&
            OCRepPayloadSetPropObjectAsOwner(array, "items", child) &&
            OCRepPayloadSetPropString(array, "type", "array");
          if (success)
          {
            array = child;
            child = NULL;
          }
        }
        if (success)
        {
          success = SetPropertiesSchema(array, value->arr.type, value->arr.objArray[0]);
        }
        break;
    }
    if (!success)
    {
      goto exit;
    }
    if (!OCRepPayloadSetPropObjectAsOwner(parent, value->name, property))
    {
      success = false;
      goto exit;
    }
    property = NULL;
  }
  success = true;

exit:
  OCRepPayloadDestroy(child);
  OCRepPayloadDestroy(property);
  return success;
}

OCRepPayload *IntrospectDefinition(OCRepPayload *payload, std::string resource_type,
  std::vector<std::string> &interfaces)
{
  OCRepPayload *definition = NULL;
  OCRepPayload *properties = NULL;
  OCRepPayload *rt = NULL;
  size_t resource_types_dim[MAX_REP_ARRAY_DEPTH] = { 0 };
  size_t dim_total;
  char **resource_types = NULL;
  std::vector<std::string>::iterator interface_it;
  OCRepPayload *itf = NULL;
  OCRepPayload *items = NULL;
  size_t itfs_dim[MAX_REP_ARRAY_DEPTH] = { 0 };
  char **itfs = NULL;
  definition = OCRepPayloadCreate();
  if (!definition)
  {
    LOG(LOG_ERR, "Failed to create payload");
    goto error;
  }
  if (!OCRepPayloadSetPropString(definition, "type", "object"))
  {
    goto error;
  }
  properties = OCRepPayloadCreate();
  if (!properties)
  {
    LOG(LOG_ERR, "Failed to create payload");
    goto error;
  }
  rt = OCRepPayloadCreate();
  if (!rt)
  {
    LOG(LOG_ERR, "Failed to create payload");
  }
  if (!OCRepPayloadSetPropBool(rt, "readOnly", true) ||
      !OCRepPayloadSetPropString(rt, "type", "array"))
  {
    goto error;  
  }
  resource_types_dim[0] = 1;
  dim_total = calcDimTotal(resource_types_dim);
  resource_types = (char **) OICCalloc(dim_total, sizeof(char *));
  if (!resource_types)
  {
    LOG(LOG_ERR, "Failed to allocate string array");
    goto error;
  }
  resource_types[0] = OICStrdup(resource_type.c_str());
  if (!OCRepPayloadSetStringArrayAsOwner(rt, "default", resource_types, resource_types_dim))
  {
    goto error;
  }
  resource_types = NULL;
  if (!OCRepPayloadSetPropObjectAsOwner(properties, "rt", rt))
  {
    goto error;
  }
  rt = NULL;
  itf = OCRepPayloadCreate();
  if (!itf)
  {
    LOG(LOG_ERR, "Failed to create payload");
    goto error;
  }
  if (!OCRepPayloadSetPropBool(itf, "readOnly", true) ||
    !OCRepPayloadSetPropString(itf, "type", "array"))
  {
    goto error;
  }
  items = OCRepPayloadCreate();
  if (!items)
  {
    LOG(LOG_ERR, "Failed to create payload");
    goto error;
  }
  if (!OCRepPayloadSetPropString(items, "type", "string"))
  {
    goto error;
  }
  itfs_dim[0] = interfaces.size();
  dim_total = calcDimTotal(itfs_dim);
  itfs = (char **) OICCalloc(dim_total, sizeof(char *));
  if (!itfs)
  {
    LOG(LOG_ERR, "Failed to allocate string array");
    goto error;
  }
  interface_it = interfaces.begin();
  for (size_t i = 0; i < dim_total; ++i, ++interface_it)
  {
    itfs[i] = OICStrdup(interface_it->c_str());
  }
  if (!OCRepPayloadSetStringArrayAsOwner(items, "enum", itfs, itfs_dim))
  {
    goto error;
  }
  itfs = NULL;
  if (!SetPropertiesSchema(properties, payload))
  {
    goto error;
  }
  if (!OCRepPayloadSetPropObjectAsOwner(definition, "properties", properties))
  {
    goto error;
  }
  properties = NULL;
  return definition;

error:
  OCRepPayloadDestroy(items);
  OCRepPayloadDestroy(itf);
  if (itfs)
  {
    dim_total = calcDimTotal(resource_types_dim);
    for (size_t i = 0; i < dim_total; ++i)
    {
      OICFree(resource_types[i]);
    }
    OICFree(resource_types);
  }
  OCRepPayloadDestroy(rt);
  OCRepPayloadDestroy(properties);
  OCRepPayloadDestroy(definition);
  return NULL;
}

OCRepPayload *IntrospectPath(std::vector<std::string> &resource_types,
  std::vector<std::string> &interfaces)
{
  OCRepPayload *path = NULL;
  OCRepPayload *method = NULL;
  size_t dim_total;
  size_t parameters_dim[MAX_REP_ARRAY_DEPTH] = { 0 };
  OCRepPayload **parameters = NULL;
  size_t itfs_dim[MAX_REP_ARRAY_DEPTH] = { 0 };
  char **itfs = NULL;
  OCRepPayload *responses = NULL;
  OCRepPayload *code = NULL;
  OCRepPayload *schema = NULL;
  size_t one_of_dim[MAX_REP_ARRAY_DEPTH] = { 0 };
  OCRepPayload **one_of = NULL;
  std::string ref;

  path = OCRepPayloadCreate();
  if (!path)
  {
    LOG(LOG_ERR, "Failed to create payload");
    goto error;
  }
  parameters_dim[0] = 2;
  dim_total = calcDimTotal(parameters_dim);
  parameters = (OCRepPayload **) OICCalloc(dim_total, sizeof(OCRepPayload *));
  if (!parameters)
  {
    LOG(LOG_ERR, "Failed to allocate object array");
    goto error;
  }
  parameters[0] = OCRepPayloadCreate();
  if (!parameters[0])
  {
    LOG(LOG_ERR, "Failed to create payload");
    goto error;
  }
  if (!OCRepPayloadSetPropString(parameters[0], "name", "if") ||
      !OCRepPayloadSetPropString(parameters[0], "in", "query") ||
      !OCRepPayloadSetPropString(parameters[0], "type", "string"))
  {
    goto error;
  }
  itfs = (char **) OICCalloc(interfaces.size(), sizeof(char *));
  if (!itfs)
  {
    LOG(LOG_ERR, "Failed to allocate string array");
    goto error;
  }
  itfs_dim[0] = 0;
  for (size_t i = 0; i < interfaces.size(); ++i)
  {
    /* Filter out read-only interfaces from post method */
    std::string &itf = interfaces[i];
    if (itf == "oic.if.ll" || itf == "oic.if.r" || itf == "oic.if.s")
    {
      continue;
    }
    itfs[itfs_dim[0]++] = OICStrdup(itf.c_str());
  }
  if (!OCRepPayloadSetStringArrayAsOwner(parameters[0], "enum", itfs, itfs_dim))
  {
    goto error;
  }
  itfs = NULL;
  parameters[1] = OCRepPayloadCreate();
  if (!parameters[1])
  {
    LOG(LOG_ERR, "Failed to create payload");
    goto error;
  }
  if (!OCRepPayloadSetPropString(parameters[1], "name", "body") ||
      !OCRepPayloadSetPropString(parameters[1], "in", "body"))
  {
    goto error;
  }
  schema = OCRepPayloadCreate();
  if (!schema)
  {
    LOG(LOG_ERR, "Failed to create payload");
    goto error;
  }
  one_of_dim[0] = resource_types.size();
  dim_total = calcDimTotal(one_of_dim);
  one_of = (OCRepPayload **) OICCalloc(dim_total, sizeof(OCRepPayload *));
  if (!one_of)
  {
    LOG(LOG_ERR, "Failed to allocate object array");
    goto error;
  }
  for (size_t i = 0; i < dim_total; ++i)
  {
    one_of[i] = OCRepPayloadCreate();
    if (!one_of[i])
    {
      LOG(LOG_ERR, "Failed to create payload");
      goto error;
    }
    ref = std::string("#/definitions/") + resource_types[i];
    if (!OCRepPayloadSetPropString(one_of[i], "$ref", ref.c_str()))
    {
      goto error;
    }
  }
  if (!OCRepPayloadSetPropObjectArrayAsOwner(schema, "oneOf", one_of, one_of_dim))
  {
    goto error;
  }
  one_of = NULL;
  // schema will be re-used in "responses" (so no ...AsOwner here)
  if (!OCRepPayloadSetPropObject(parameters[1], "schema", schema))
  {
    goto error;
  }
  // parameters will be re-used in "get" (so no ...AsOwner here)
  if (!OCRepPayloadSetPropObjectArray(method, "parameters", (const OCRepPayload **) parameters, parameters_dim))
  {
    goto error;
  }
  responses = OCRepPayloadCreate();
  if (!responses)
  {
    LOG(LOG_ERR, "Failed to create payload");
    goto error;
  }
  code = OCRepPayloadCreate();
  if (!code)
  {
    LOG(LOG_ERR, "Failed to create payload");
    goto error;
  }
  if (!OCRepPayloadSetPropString(code, "description", ""))
  {
    goto error;
  }
  if (!OCRepPayloadSetPropObjectAsOwner(code, "schema", schema))
  {
    goto error;
  }
  schema = NULL;
  if (!OCRepPayloadSetPropObjectAsOwner(responses, "200", code))
  {
    goto error;
  }
  code = NULL;
  // responses will be re-used in "get" (so no ...AsOwner here)
  if (!OCRepPayloadSetPropObject(method, "responses", responses))
  {
    goto error;
  }
  if (!OCRepPayloadSetPropObjectAsOwner(path, "post", method))
  {
    goto error;
  }
  method = NULL;
  method = OCRepPayloadCreate();
  if (!method)
  {
    LOG(LOG_ERR, "Failed to create payload");
    goto error;
  }
  itfs = (char **) OICCalloc(interfaces.size(), sizeof(char *));
  if (!itfs)
  {
    LOG(LOG_ERR, "Failed to allocate string array");
    goto error;
  }
  itfs_dim[0] = 0;
  for (size_t i = 0; i < interfaces.size(); ++i)
  {
    // All interfaces support get method
    itfs[itfs_dim[0]++] = OICStrdup(interfaces[i].c_str());
  }
  if (!OCRepPayloadSetStringArrayAsOwner(parameters[0], "enum", itfs, itfs_dim))
  {
    goto error;
  }
  itfs = NULL;
  parameters_dim[0] = 1; // only use "if" parameter
  if (!OCRepPayloadSetPropObjectArrayAsOwner(method, "parameters", parameters, parameters_dim))
  {
    goto error;
  }
  parameters = NULL;
  if (!OCRepPayloadSetPropObject(method, "responses", responses))
  {
    goto error;
  }
  responses = NULL;
  if (!OCRepPayloadSetPropObjectAsOwner(path, "get", method))
  {
    goto error;
  }
  method = NULL;
  return path;

error:
  if (one_of)
  {
    dim_total = calcDimTotal(one_of_dim);
    for (size_t i = 0; i < dim_total; ++i)
    {
      OCRepPayloadDestroy(one_of[i]);
    }
    OICFree(one_of);
  }
  OCRepPayloadDestroy(schema);
  OCRepPayloadDestroy(code);
  if (parameters)
  {
    dim_total = calcDimTotal(parameters_dim);
    for (size_t i = 0; i < dim_total; ++i)
    {
      OCRepPayloadDestroy(parameters[i]);
    }
    OICFree(parameters);
  }
  if (itfs)
  {
    dim_total = calcDimTotal(itfs_dim);
    for (size_t i = 0; i < dim_total; ++i)
    {
      OICFree(itfs[i]);
    }
    OICFree(itfs);
  }
  OCRepPayloadDestroy(responses);
  OCRepPayloadDestroy(method);
  OCRepPayloadDestroy(path);
  return NULL;
}