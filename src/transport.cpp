#ifndef NDEBUG
#define HF_LOG_LEVEL HF_LOG_LEVEL_TRACE
#endif

#include <iostream>
#include <iomanip>

#include <cassert>
#include <cstdlib>
#include <cstdint>

#include "uv.h"

#include "hanfun.h"

#include "log.h"

#include "transport.h"

#define NONE_MSG    0xFFFF
#define HELLO_MSG   0x0101     
#define DATA_MSG    0x0201    

#define CHECK_STATUS()                              \
  if (status != 0)                                  \
  {                                                 \
    print_error(status);                            \
    exit(-1);                                       \
  }

struct Message
{
  uint16_t nbytes;       
  uint16_t primitive;    
  HF::Common::ByteArray data;         

  Message(uint16_t primitive = NONE_MSG):
      primitive(primitive)
  {}

  Message(uint16_t primitive, HF::Common::ByteArray &data):
      primitive(primitive), data(data)
  {}

  static constexpr uint16_t min_size = sizeof(nbytes) + sizeof(primitive);

  uint16_t size() const
  {
    return min_size + data.size();
  }

  uint16_t pack(HF::Common::ByteArray &array, uint16_t offset = 0) const
  {
    HF_SERIALIZABLE_CHECK(array, offset, size());

    uint16_t start = offset;

    uint16_t temp  = (uint16_t) (sizeof(uint16_t) + data.size());

    offset += array.write(offset, temp);

    offset += array.write(offset, primitive);

    std::copy(data.begin(), data.end(), array.begin() + offset);

    return offset - start;
  }

  uint16_t unpack(HF::Common::ByteArray &array, uint16_t offset = 0)
  {
    HF_SERIALIZABLE_CHECK(array, offset, min_size);

    uint16_t start = offset;

    offset += array.read(offset, nbytes);

    offset += array.read(offset, primitive);

    uint16_t data_size = nbytes - sizeof(primitive);

    data = HF::Common::ByteArray(data_size);

    auto begin = array.begin();

    begin += offset;

    auto end = begin + data_size;

    std::copy(begin, end, data.begin());

    return offset - start;
  }
};

struct HelloMessage
{
  uint8_t core;
  uint8_t profiles;
  uint8_t interfaces;
  
  HF::UID::UID uid;
  
  HelloMessage():
    core(HF::CORE_VERSION), profiles(HF::PROFILES_VERSION), interfaces(HF::INTERFACES_VERSION)
  {}
  
  static constexpr uint16_t min_size = 3 * sizeof(uint8_t);
  
  uint16_t size() const
  {
    return min_size + uid.size();
  }
  
  uint16_t pack(HF::Common::ByteArray &array, uint16_t offset = 0) const
  {
    HF_SERIALIZABLE_CHECK(array, offset, size());
    
    uint16_t start = offset;
    
    offset += array.write(offset, core);
    offset += array.write(offset, profiles);
    offset += array.write(offset, interfaces);
    
    offset += uid.pack(array, offset);
    
    return offset - start;
  }
  
  uint16_t unpack(HF::Common::ByteArray &array, uint16_t offset = 0)
  {
    HF_SERIALIZABLE_CHECK(array, offset, min_size);
    
    uint16_t start = offset;
    
    offset += array.read(offset, core);
    offset += array.read(offset, profiles);
    offset += array.read(offset, interfaces);
    
    uid.unpack(array, offset);
    
    return offset - start;
  }
};

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
  UNUSED(handle);
  buf->base = (char *) malloc(suggested_size);
  buf->len = suggested_size;
}

void print_error(int status)
{
  LOG(LOG_ERR, "%s - %s", uv_err_name(status), uv_strerror(status));
}

static void handle_message(Link *link, Message &msg);

static void on_close(uv_handle_t *handle)
{
  LOG(LOG_INFO, "Connection closed!");

  UNUSED(handle);
}

static void on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
  Link *link = (Link *) stream->data;

  Transport *transport = link->transport();

  if (nread < 0)
  {
    LOG(LOG_WARN, "Could not read from stream. Stream closed!");

    transport->remove(link);

    uv_close((uv_handle_t *) stream, on_close);
  }
  else if (nread > 0)
  {
    HF::Common::ByteArray payload((uint8_t *) buf->base, nread);

    Message msg;
    msg.unpack(payload);

    handle_message(link, msg);
  }

  free(buf->base);
}

static void on_write(uv_write_t *req, int status)
{
  CHECK_STATUS();

  assert(req->type == UV_WRITE);

  /* Free the read/write buffer and the request */
  free(req);
}

static void send_message(uv_stream_t *stream, Message &msg)
{
  HF::Common::ByteArray payload(msg.size());
  msg.pack(payload);

  uv_write_t *req = (uv_write_t *) calloc(1, sizeof(uv_write_t));
  uv_buf_t buf    = uv_buf_init((char *) payload.data(), payload.size());

  uv_write(req, (uv_stream_t *) stream, &buf, 1 /*nbufs*/, on_write);
}

static void send_hello(uv_stream_t *stream, Transport *transport)
{
  HelloMessage hello;
  
  hello.uid = transport->uid();
  
  Message msg(HELLO_MSG);
  
  msg.data = HF::Common::ByteArray(hello.size());
  
  hello.pack(msg.data);
  
  send_message(stream, msg);
}

static void handle_message(Link *link, Message &msg)
{
  Transport *transport = link->transport();

  switch (msg.primitive)
  {
    case HELLO_MSG:
    {
      HelloMessage hello;
      
      hello.unpack(msg.data);
      
      LOG(LOG_DEBUG, "%d", hello.uid);
      
      ((Link *) link)->uid(hello.uid.raw()->clone());
      
      transport->add(link);
      
      break;
    }
    case DATA_MSG:
    {
      transport->receive(link, msg.data);
      break;
    }
    default:
      break;
  }
}

static void on_connect(uv_connect_t *conn, int status)
{
  CHECK_STATUS();

  uv_stream_t *stream = conn->handle;

  uv_read_start(stream, (uv_alloc_cb) alloc_buffer, on_read);

  if (uv_is_writable(stream) && uv_is_readable(stream))
  {
    LOG(LOG_INFO, "Connected!");

    Transport *transport = ((Transport *) stream->data);
    Link *link     = new Link(transport, stream);

    stream->data = link;
    
    send_hello(stream, transport);
  }
  else
  {
    LOG(LOG_ERR, "Cannot read/write to stream!");
    uv_close((uv_handle_t *) conn->handle, NULL);
  }

  free(conn);
}

void Transport::initialize()
{
  LOG(LOG_TRACE, "[%p]", this);

  uv_tcp_init(uv_default_loop(), &socket_);

  uv_tcp_keepalive(&socket_, 1, 60);

  socket_.data = this;

  uv_connect_t *connect = (uv_connect_t *) calloc(1, sizeof(uv_connect_t));

  struct sockaddr_in dest;
  uv_ip4_addr("127.0.0.1", 8000, &dest);

  uv_tcp_connect(connect, &socket_, (const struct sockaddr *) &dest, on_connect);
}

void Transport::destroy()
{
  LOG(LOG_TRACE, "[%p]", this);
}

void Link::send(HF::Common::ByteArray &array)
{
  LOG(LOG_TRACE, "[%p]", this);

  Message msg(DATA_MSG);

  msg.data = array;

  send_message((uv_stream_t *) stream_, msg);
}

#if HF_GROUP_SUPPORT

HF::Common::Result HF::Transport::Group::create(Endpoint &ep, uint16_t group)
{
  UNUSED(ep);
  UNUSED(group);

  LOG(LOG_WARN, "Not implemented!");

  return HF::Common::Result::FAIL_SUPPORT;
}

HF::Common::Result HF::Transport::Group::add(Endpoint &ep, uint16_t group, uint16_t device)
{
  UNUSED(ep);
  UNUSED(group);
  UNUSED(device);

  LOG(LOG_WARN, "Not implemented!");

  return HF::Common::Result::FAIL_SUPPORT;
}

void HF::Transport::Group::remove(Endpoint &ep, uint16_t group, uint16_t device)
{
  UNUSED(ep);
  UNUSED(group);
  UNUSED(device);

  LOG(LOG_WARN, "Not implemented!");
}

void HF::Transport::Group::remove(Endpoint &ep, uint16_t group)
{
  UNUSED(ep);
  UNUSED(group);

  LOG(LOG_WARN, "Not implemented!");
}

#endif
