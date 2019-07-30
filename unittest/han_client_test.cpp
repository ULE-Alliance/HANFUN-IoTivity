#include "unit_test.h"

#include "han_client.h"
#include "log.h"

#include <thread>

template <class T>
class HanClientTestBase : public T
{
  protected:
    virtual ~HanClientTestBase() {}
    virtual void SetUp()
    {
      HFOCSetUp::SetUpStack();
    }
    virtual void TearDown()
    {
      HFOCSetUp::TearDownStack();
    }
};

class HanClientTest : public HanClientTestBase<::testing::Test>
{
  public:
    virtual ~HanClientTest() {}
  protected:
    static void SetUpTestCase() {
      loop_ = uv_default_loop();
      client_ = new HanClient("127.0.0.1",
                              (uint16_t)3490,
                              loop_);

      ASSERT_EQ(0, client_->start());

      std::thread thread([]() {
        uv_run(loop_, UV_RUN_DEFAULT);
      });
      thread.detach();
    }
    static void TearDownTestCase() {
      client_->stop();
      delete client_;
    }

    static uv_loop_t* loop_;
    static HanClient* client_;
};

uv_loop_t* HanClientTest::loop_ = NULL;
HanClient* HanClientTest::client_ = NULL;

TEST_F(HanClientTest, SendInit)
{
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_TRUE(client_->is_initialized());
}

TEST_F(HanClientTest, OpenRegistration)
{
  EXPECT_EQ(0, client_->open_registration());
}

TEST_F(HanClientTest, CloseRegistration)
{
  EXPECT_EQ(0, client_->close_registration());
}