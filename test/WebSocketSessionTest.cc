#include "WebSocketSession.h"

#include <string>

#include <cppunit/extensions/HelperMacros.h>

namespace aria2 {
namespace rpc {

class WebSocketSessionTest : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(WebSocketSessionTest);
  CPPUNIT_TEST(testOutboundQueueLimits);
  CPPUNIT_TEST_SUITE_END();

public:
  void testOutboundQueueLimits();
};

CPPUNIT_TEST_SUITE_REGISTRATION(WebSocketSessionTest);

void WebSocketSessionTest::testOutboundQueueLimits()
{
  CPPUNIT_ASSERT(!WebSocketSession::outboundQueueWouldOverflow(
      WebSocketSession::MAX_OUTBOUND_QUEUE_LENGTH, 0, 0));
  CPPUNIT_ASSERT(WebSocketSession::outboundQueueWouldOverflow(
      WebSocketSession::MAX_OUTBOUND_QUEUE_LENGTH, 0, 1));
  CPPUNIT_ASSERT(WebSocketSession::outboundQueueWouldOverflow(
      0, WebSocketSession::MAX_OUTBOUND_QUEUE_MESSAGES, 0));

  WebSocketSession session(std::shared_ptr<SocketCore>(), nullptr);

  session.addTextMessage(
      std::string(WebSocketSession::MAX_OUTBOUND_QUEUE_LENGTH + 1, 'x'), false);
  CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(0),
                       session.getQueuedMessageCount());

  for (size_t i = 0;
       i < WebSocketSession::MAX_OUTBOUND_QUEUE_MESSAGES + 1; ++i) {
    session.addTextMessage(std::string(), false);
  }
  CPPUNIT_ASSERT_EQUAL(WebSocketSession::MAX_OUTBOUND_QUEUE_MESSAGES,
                       session.getQueuedMessageCount());
}

} // namespace rpc
} // namespace aria2
