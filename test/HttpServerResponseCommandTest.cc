#include "HttpServerResponseCommand.h"

#include <cppunit/extensions/HelperMacros.h>

#include "DownloadEngine.h"
#include "EventPoll.h"
#include "SocketCore.h"

namespace aria2 {

namespace {
class RecordingEventPoll : public EventPoll {
public:
  size_t writeChecks = 0;

  void poll(const struct timeval&) CXX11_OVERRIDE {}

  bool addEvents(sock_t, Command*, EventType events) CXX11_OVERRIDE
  {
    if (events & EVENT_WRITE) {
      ++writeChecks;
    }
    return true;
  }

  bool deleteEvents(sock_t, Command*, EventType events) CXX11_OVERRIDE
  {
    if (events & EVENT_WRITE) {
      CPPUNIT_ASSERT(writeChecks > 0);
      --writeChecks;
    }
    return true;
  }

#ifdef ENABLE_ASYNC_DNS
  bool addNameResolver(const std::shared_ptr<AsyncNameResolver>&,
                       Command*) CXX11_OVERRIDE
  {
    return true;
  }

  bool deleteNameResolver(const std::shared_ptr<AsyncNameResolver>&,
                          Command*) CXX11_OVERRIDE
  {
    return true;
  }
#endif // ENABLE_ASYNC_DNS
};
} // namespace

class HttpServerResponseCommandTest : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(HttpServerResponseCommandTest);
  CPPUNIT_TEST(testDisableSocketCheck);
  CPPUNIT_TEST_SUITE_END();

public:
  void testDisableSocketCheck();
};

CPPUNIT_TEST_SUITE_REGISTRATION(HttpServerResponseCommandTest);

void HttpServerResponseCommandTest::testDisableSocketCheck()
{
  auto eventPoll = make_unique<RecordingEventPoll>();
  auto eventPollPtr = eventPoll.get();
  DownloadEngine engine(std::move(eventPoll));
  auto socket = std::make_shared<SocketCore>();

  {
    HttpServerResponseCommand command(1, nullptr, &engine, socket);
    CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), eventPollPtr->writeChecks);

    command.disableSocketCheck();
    CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(0), eventPollPtr->writeChecks);
  }

  // The destructor must not try to unregister the write event a second time.
  CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(0), eventPollPtr->writeChecks);
}

} // namespace aria2
