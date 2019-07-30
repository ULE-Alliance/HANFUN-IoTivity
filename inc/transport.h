#ifndef _TRANSPORT_H
#define _TRANSPORT_H

#include <forward_list>

#include "uv.h"

#include "hanfun.h"

class Transport: public HF::Devices::Node::Transport
{
  protected:

    uv_tcp_t socket_;

  public:

    virtual ~Transport() {}

    void initialize();

    void destroy();
};

class Link: public HF::Transport::AbstractLink
{
  protected:

    Transport *transport_;

    uv_stream_s *stream_;

  public:

    Link(Transport *transport, uv_stream_s *stream):
      HF::Transport::AbstractLink(), transport_(transport), stream_(stream)
    {
      stream_->data = this;
    }

    virtual ~Link()
    {}

    void send(HF::Common::ByteArray &array);

    Transport *transport() const
    {
      return transport_;
    }
};

#endif
