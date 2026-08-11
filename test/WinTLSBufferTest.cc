#include "WinTLSBuffer.h"

#include <limits>

#include <cppunit/extensions/HelperMacros.h>

namespace aria2 {
namespace wintls {

class WinTLSBufferTest : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(WinTLSBufferTest);
  CPPUNIT_TEST(testBounds);
  CPPUNIT_TEST_SUITE_END();

public:
  void testBounds();
};

CPPUNIT_TEST_SUITE_REGISTRATION(WinTLSBufferTest);

void WinTLSBufferTest::testBounds()
{
  Buffer buffer;
  CPPUNIT_ASSERT(buffer.write("abc", 3));
  CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(3), buffer.size());

  CPPUNIT_ASSERT(!buffer.eat(4));
  CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(3), buffer.size());
  CPPUNIT_ASSERT(!buffer.advance(buffer.free() + 1));
  CPPUNIT_ASSERT(!buffer.write(nullptr, 1));
  CPPUNIT_ASSERT(!buffer.write(
      "x", std::numeric_limits<size_t>::max()));

  CPPUNIT_ASSERT(buffer.eat(3));
  CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(0), buffer.size());
}

} // namespace wintls
} // namespace aria2
