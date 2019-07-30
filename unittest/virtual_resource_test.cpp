#include "hanfun.h"
#include "ocstack.h"
#include "transport.h"
#include "virtual_resource.h"
#include <thread>

typedef HF::Devices::Node::Unit0<HF::Core::DeviceInformation::Server,
                                 HF::Core::DeviceManagement::Client,
                                 HF::Core::AttributeReporting::Server,
                                 HF::Core::Time::Server,
                                 HF::Core::BatchProgramManagement::DefaultServer,
                                 HF::Core::Scheduling::Event::DefaultServer,
                                 HF::Core::Scheduling::Weekly::DefaultServer
                                > NodeUnit0;

class TestNode : public HF::Devices::Node::Abstract<NodeUnit0>
{
  public:
    void connected(HF::Transport::Link *link)
    {
      (void) link;
    }

    void disconnected(HF::Transport::Link *link)
    {
      (void) link;
    }

    void receive(HF::Protocol::Packet &packet, HF::Common::ByteArray &payload, uint16_t offset)
    {
      (void) packet;
      (void) payload;
      (void) offset;
    }
};

static void CreateCB(void *context)
{
  (void) context;
}

int main(int, char **)
{
  if (OC_STACK_OK != OCInit2(OC_SERVER, OC_DEFAULT_FLAGS, OC_DEFAULT_FLAGS, OC_ADAPTER_IP))
  {
    exit(EXIT_FAILURE);
  }
  
  Transport transport;
  transport.initialize();

  TestNode *test_node = new TestNode();
  transport.add(test_node);
  transport.uid(new HF::UID::URI("hf://node.test.com/test"));

  VirtualResource *resource_ = VirtualResource::Create(1, "/example", CreateCB, NULL);

  for (;;)
  {
    OCProcess();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  delete resource_;
  
  transport.destroy();

  if (OC_STACK_OK != OCStop())
  {
    exit(EXIT_FAILURE);
  }
  exit(EXIT_SUCCESS);
}