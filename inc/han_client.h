#ifndef _HANCLIENT_H
#define _HANCLIENT_H

#include "uv.h"

#ifndef HAN_UNUSED
#define HAN_UNUSED(x) (void)x;
#endif

typedef void (*han_cb)(void* context);
typedef void (*han_device_table_cb)(void *context,
                                    uint8_t dev_index,
                                    uint8_t no_of_devices,
                                    uint16_t *dev_ids,
                                    uint8_t **dev_ipuis,
                                    uint8_t **dev_emcs);

typedef struct HANCallbackData
{
  void* context;
  han_cb cb;

} HANCallbackData;

class HanClient
{
  protected:
    uv_udp_t socket_;

  public:
    HanClient(const char* ip, uint16_t port, uv_loop_t* loop);
    int start();
    int open_registration();
    int close_registration();
    int get_device_table(uint8_t start_index, uint8_t no_of_devices, void *ctx);
    void stop();

    bool is_initialized()
    {
      return initialized_;
    }
    void set_initialized(bool initialized)
    {
      initialized_ = true;
    }
    han_device_table_cb get_device_table_cb()
    {
      return device_table_cb_;
    }
    void set_device_table_cb(han_device_table_cb callback)
    {
      device_table_cb_ = callback;
    }

    void *context_;

  private:
    int send_init_message();

    const char* ip_;
    uint16_t port_;
    uv_loop_t* loop_;
    bool initialized_;

    uv_udp_send_t request_;
    struct sockaddr_in addr_;

    han_device_table_cb device_table_cb_;
};

#endif // _HANCLIENT_H
