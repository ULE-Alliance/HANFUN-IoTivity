#include "han_client.h"

#include <cstdlib>

#include "log.h"

#include <thread>
#include <cstring>
#include <list>

#define CHECK_RETURN(x)                               \
   if ((status = x) < 0)                                  \
   {                                                 \
      print_han_error(status); \
      return status;                                      \
   }

struct Message
{
  char* service;
  char* command;
  char* parameters;
  //std::map<char*, char*> parameters;
  //std::list<Parameter> parameters;

  Message(char* service, char* command) :
    service(service), command(command), parameters(NULL)
  {}

  Message() :
    Message(NULL, NULL)
  {}

  /*void add_parameter(char* name, char* value)
  {
    parameters.insert(std::make_pair(name, value));
  }*/

  size_t pack(char* &buffer)
  {
    size_t service_size = service_pack_size();
    size_t command_size = command_pack_size();
    size_t parameters_size = parameters_pack_size();

    buffer = (char*) calloc(1, service_size + command_size + parameters_size + 2);

    pack_service(buffer);
    pack_command(buffer, service_size);
    pack_parameters(buffer, service_size + command_size);

    buffer[service_size + command_size + parameters_size] = '\r';
    buffer[service_size + command_size + parameters_size + 1] = '\n';
  
    return service_size + command_size + parameters_size + 2;
  }

  void unpack(char* buffer, size_t buffer_len)
  {
    uint8_t service_len = 0;
    uint8_t command_len = 0;
    char* lines = strtok(buffer, "\r\n");

    if (lines != NULL)
    {
      if (lines[0] == '[')
      {
        service_len = strlen(lines);
        service = (char*) calloc(1, service_len - 2);
        strncpy(service, lines + 1, service_len - 2);

        lines = strtok(NULL, "\r\n");
      }
      else
      {
        service = (char *)"HAN";
      }

      if (lines != NULL)
      {
        command_len = strlen(lines);
        command = (char*) calloc(1, command_len);
        strncpy(command, lines, command_len);

        lines = strtok(NULL, "\r\n");
      }

      if (lines != NULL)
      {
        parameters = (char *) calloc(1, buffer_len - service_len - command_len);
        strncpy(parameters, lines, strlen(lines));
        strcat(parameters, "\r\n");
        lines = strtok(NULL, "\r\n");

        while (lines != NULL)
        {
          strcat(parameters, lines);
          strcat(parameters, "\r\n");
          lines = strtok(NULL, "\r\n");
        }
      }
    }
  }

  private:
    size_t service_pack_size()
    {
      size_t size = 0;
      if (service != NULL)
      {
        size += strlen(service) + 4;
      }

      return size;
    }
    void pack_service(char* &buffer)
    {
      if (service_pack_size() > 0)
      {
        size_t service_size = strlen(service);
        buffer[0] = '[';
        strncpy(buffer + 1, service, service_size);
        buffer[service_size + 1] = ']';
        buffer[service_size + 2] = '\r';
        buffer[service_size + 3] = '\n';
      }
    }
    size_t command_pack_size()
    {
      return strlen(command) + 2;
    }
    void pack_command(char* &buffer, size_t offset)
    {
      size_t command_size = strlen(command);
      strncpy(buffer + offset, command, command_size);
      buffer[offset + command_size] = '\r';
      buffer[offset + command_size + 1] = '\n';
    }
    size_t parameters_pack_size()
    {
      size_t size = 0;
      if (parameters != NULL)
      {
        size = strlen(parameters);
      }
      return size;
    }
    void pack_parameters(char* &buffer, size_t offset)
    {
      if (parameters_pack_size() > 0)
      {
        strncpy(buffer + offset, parameters, strlen(parameters));
      }
    }
};

struct DeviceTableMessage
{
  uint8_t dev_index;
  uint8_t no_of_devices;
  uint16_t *dev_ids;
  uint8_t **dev_ipuis;
  uint8_t **dev_emcs;

  DeviceTableMessage()
    : dev_index(0), no_of_devices(0), dev_ids(NULL)
  {
    dev_ipuis = new uint8_t*[1];
    dev_emcs = new uint8_t*[1];
  }
  
  DeviceTableMessage(uint8_t dev_index, uint8_t no_of_devices)
    : dev_index(dev_index), no_of_devices(no_of_devices)
  {
    dev_ids = new uint16_t[no_of_devices];
    dev_ipuis = new uint8_t*[no_of_devices];
    dev_emcs = new uint8_t*[no_of_devices];
  }

  void unpack(char* buffer)
  {
    char *saveptr1, *saveptr2;
    char* lines = strtok_r(buffer, "\r\n", &saveptr1);

    if (lines != NULL)
    {
      if (0 == strncmp(" DEV_INDEX: ", lines, 12))
      {
        char* value = (char *) calloc(1, strlen(lines) - 12);
        strncpy(value, lines + 12, strlen(lines) - 12);
        dev_index = (uint8_t)atoi(value);
        delete value;

        lines = strtok_r(NULL, "\r\n", &saveptr1);

        if (0 == strncmp(" NO_OF_DEVICES: ", lines, 16))
        {
          value = (char *) calloc(1, strlen(lines) - 16);
          strncpy(value, lines + 16, strlen(lines) - 16);
          no_of_devices = (uint8_t)atoi(value);
          delete value;
        }

        lines = strtok_r(NULL, "\r\n", &saveptr1);

        if (no_of_devices > 0)
        {
          dev_ids = new uint16_t[no_of_devices];
          int i = 0;
          while (lines != NULL && i < no_of_devices)
          {
            if (0 == strncmp(" DEV_ID: ", lines, 9))
            {
              value = (char *) calloc(1, strlen(lines) - 9);
              strncpy(value, lines + 9, strlen(lines) - 9);
              dev_ids[i] = (uint16_t)atoi(value);
              delete value;
              lines = strtok_r(NULL, "\r\n", &saveptr1);
            }

            if (0 == strncmp(" DEV_IPUI: ", lines, 11))
            {
              value = (char *) calloc(1, strlen(lines) - 11);
              strncpy(value, lines + 11, strlen(lines) - 11);
              char *bytes = strtok_r(value, " ", &saveptr2);
              int j = 0;
              dev_ipuis[i] = new uint8_t[5];
              while (bytes != NULL && j < 5)
              {
                dev_ipuis[i][j] = (uint8_t)atoi(bytes);
                bytes = strtok_r(NULL, " ", &saveptr2);
                j++;
              }
              lines = strtok_r(NULL, "\r\n", &saveptr1);
            }

            if (0 == strncmp(" DEV_EMC: ", lines, 10))
            {
              value = (char *) calloc(1, strlen(lines) - 10);
              strncpy(value, lines + 11, strlen(lines) - 10);
              char *bytes = strtok_r(value, " ", &saveptr2);
              int j = 0;
              dev_emcs[i] = new uint8_t[2];
              while (bytes != NULL && j < 2)
              {
                dev_emcs[i][j] = (uint8_t)atoi(bytes);
                bytes = strtok_r(NULL, " ", &saveptr2);
                j++;
              }
              lines = strtok_r(NULL, "\r\n", &saveptr1);
            }

            i++;
          }
        }
      }
    }
  }
};

void print_han_error(int status)
{
   LOG(LOG_ERR, "%s - %s", uv_err_name(status), uv_strerror(status));
}

HanClient::HanClient(const char* ip, uint16_t port, uv_loop_t* loop)
  : ip_(ip), port_(port), loop_(loop), initialized_(false)
{}

void alloc_udp_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
  HAN_UNUSED(handle);
  buf->base = (char *) calloc(1, suggested_size);
  buf->len = suggested_size;
  LOG(LOG_TRACE, "calloc:%lu %p", buf->len, buf->base);
}

static void client_recv_cb(uv_udp_t *handle,
                           ssize_t nread,
                           const uv_buf_t *buf,
                           const struct sockaddr *addr,
                           unsigned flags)
{
  HAN_UNUSED(addr);
  HAN_UNUSED(flags);

  if (nread > 0)
  {
    LOG(LOG_TRACE, "\n%s", buf->base);

    Message msg;
    msg.unpack(buf->base, buf->len);
    if (0 == strcmp("INIT_RES", msg.command))
    {
      HanClient* thiz = reinterpret_cast<HanClient *>(handle->data);
      if (thiz)
      {
        thiz->set_initialized(true);
      }
      else
      {
        LOG(LOG_ERR, "HanClient context is null");
      }      
    }
    else if (0 == strcmp("DEV_TABLE", msg.command))
    {
      HanClient* thiz = reinterpret_cast<HanClient *>(handle->data);
      if (thiz)
      {
        DeviceTableMessage dtm;
        dtm.unpack(msg.parameters/*, sizeof(msg.parameters)*/);
        thiz->get_device_table_cb()(thiz->context_,
                                    dtm.dev_index,
                                    dtm.no_of_devices,
                                    dtm.dev_ids,
                                    dtm.dev_ipuis,
                                    dtm.dev_emcs);
      }
    }
    msg.~Message();
  }
  else if (nread < 0)
  {
    uv_udp_recv_stop(handle);
  }

  free(buf->base);
}

static void client_send_cb(uv_udp_send_t *req, int status)
{
  HAN_UNUSED(req);

  if (status < 0)
  {
    print_han_error(status);
  }
}

static void on_close(uv_handle_t *handle)
{
  LOG(LOG_TRACE, "Closing handle %p", handle);
}

int HanClient::send_init_message()
{
  Message msg(NULL, (char *)"INIT");
  msg.parameters = (char *)" VERSION: 1\r\n";
  char* message;
  size_t message_length = msg.pack(message);
  uv_buf_t init_msg = uv_buf_init(message, message_length);
  socket_.data = this;
  return uv_udp_send(&request_,
                     &socket_,
                     &init_msg,
                     1,
                     (const struct sockaddr *)&addr_,
                     client_send_cb);
}

int HanClient::start()
{
  int status = 0;
  LOG(LOG_TRACE, "[%p]", this);

  CHECK_RETURN(uv_ip4_addr(ip_, port_, &addr_));
  CHECK_RETURN(uv_udp_init(loop_, &socket_));
  CHECK_RETURN(uv_udp_recv_start(&socket_, alloc_udp_buffer, client_recv_cb));
  
  return send_init_message();
}

int HanClient::open_registration()
{
  Message msg(NULL, (char *)"OPEN_REG");
  msg.parameters = (char *)" TIME: 60\r\n";
  char* message;
  size_t message_length = msg.pack(message);
  uv_buf_t init_msg = uv_buf_init(message, message_length);
  return uv_udp_send(&request_,
                     &socket_,
                     &init_msg,
                     1,
                     (const struct sockaddr *)&addr_,
                     client_send_cb);
}

int HanClient::close_registration()
{
  Message msg(NULL, (char*)"CLOSE_REG");
  char* message;
  size_t message_length = msg.pack(message);
  uv_buf_t close_msg = uv_buf_init(message, message_length);
  return uv_udp_send(&request_,
                     &socket_,
                     &close_msg,
                     1,
                     (const struct sockaddr *)&addr_,
                     client_send_cb);
}

void on_walk(uv_handle_t* handle, void* arg)
{
  uv_close(handle, on_close);
}

int HanClient::get_device_table(uint8_t start_index, uint8_t no_of_devices, void *context)
{
  Message msg(NULL, (char *)"GET_DEV_TABLE");
  char *index = new char[3];
  sprintf(index, "%d", start_index);
  msg.parameters = new char[28 + strlen(index)];
  sprintf(msg.parameters, " DEV_INDEX: %s\r\n HOW_MANY: %d\r\n", index, no_of_devices);
  delete[] index;
  LOG(LOG_TRACE, "%s", msg.parameters);
  char* message;
  size_t message_length = msg.pack(message);
  uv_buf_t get_devices_msg = uv_buf_init(message, message_length);
  context_ = context;
  return uv_udp_send(&request_,
                     &socket_,
                     &get_devices_msg,
                     1,
                     (const struct sockaddr *)&addr_,
                     client_send_cb);
}

void HanClient::stop()
{
  LOG(LOG_TRACE, "[%p]", this);
  uv_udp_recv_stop(&socket_);
  uv_walk(loop_, on_walk, NULL);
}
