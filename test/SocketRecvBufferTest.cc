#include "SocketRecvBuffer.h"

#include <limits>

#include <cppunit/extensions/HelperMacros.h>

#include "DlAbortEx.h"

namespace aria2 {

class SocketRecvBufferTest : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(SocketRecvBufferTest);
  CPPUNIT_TEST(testRejectOversizedDrain);
  CPPUNIT_TEST_SUITE_END();

public:
  void testRejectOversizedDrain();
};

CPPUNIT_TEST_SUITE_REGISTRATION(SocketRecvBufferTest);

void SocketRecvBufferTest::testRejectOversizedDrain()
{
  SocketRecvBuffer buffer(nullptr);
  buffer.drain(0);

  try {
    buffer.drain(std::numeric_limits<size_t>::max());
    CPPUNIT_FAIL("exception must be thrown");
  }
  catch (DlAbortEx&) {
    // success
  }

  CPPUNIT_ASSERT(buffer.bufferEmpty());
  CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(0), buffer.getBufferLength());
}

} // namespace aria2
