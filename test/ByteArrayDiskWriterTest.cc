#include "ByteArrayDiskWriter.h"
#include <limits>
#include <string>
#include <cppunit/extensions/HelperMacros.h>

#include "DlAbortEx.h"

namespace aria2 {

class ByteArrayDiskWriterTest : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(ByteArrayDiskWriterTest);
  CPPUNIT_TEST(testWriteAndRead);
  CPPUNIT_TEST(testWriteAndRead2);
  CPPUNIT_TEST(testRejectInvalidRanges);
  CPPUNIT_TEST_SUITE_END();

private:
public:
  void setUp() {}

  void testWriteAndRead();
  void testWriteAndRead2();
  void testRejectInvalidRanges();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ByteArrayDiskWriterTest);

void ByteArrayDiskWriterTest::testWriteAndRead()
{
  ByteArrayDiskWriter bw;

  std::string msg1 = "Hello";
  bw.writeData((const unsigned char*)msg1.c_str(), msg1.size(), 0);
  // write at the end of stream
  std::string msg2 = " World";
  bw.writeData((const unsigned char*)msg2.c_str(), msg2.size(), 5);
  // write at the end of stream +1
  std::string msg3 = "!!";
  bw.writeData((const unsigned char*)msg3.c_str(), msg3.size(), 12);
  // write space at the 'hole'
  std::string msg4 = " ";
  bw.writeData((const unsigned char*)msg4.c_str(), msg4.size(), 11);

  char buf[100];
  int32_t c = bw.readData((unsigned char*)buf, sizeof(buf), 1);
  buf[c] = '\0';

  CPPUNIT_ASSERT_EQUAL(std::string("ello World !!"), std::string(buf));
  CPPUNIT_ASSERT_EQUAL((int64_t)14, bw.size());
}

void ByteArrayDiskWriterTest::testWriteAndRead2()
{
  ByteArrayDiskWriter bw;

  std::string msg1 = "Hello World";
  bw.writeData((const unsigned char*)msg1.c_str(), msg1.size(), 0);
  std::string msg2 = "From Mars";
  bw.writeData((const unsigned char*)msg2.c_str(), msg2.size(), 6);

  char buf[100];
  int32_t c = bw.readData((unsigned char*)buf, sizeof(buf), 0);
  buf[c] = '\0';

  CPPUNIT_ASSERT_EQUAL(std::string("Hello From Mars"), std::string(buf));
  CPPUNIT_ASSERT_EQUAL((int64_t)15, bw.size());
}

void ByteArrayDiskWriterTest::testRejectInvalidRanges()
{
  ByteArrayDiskWriter bw(8);
  bw.setString("data");
  unsigned char byte = 'x';

  try {
    bw.writeData(&byte, 1, -1);
    CPPUNIT_FAIL("exception must be thrown");
  }
  catch (DlAbortEx&) {
    // success
  }

  try {
    bw.writeData(&byte, std::numeric_limits<size_t>::max(), 0);
    CPPUNIT_FAIL("exception must be thrown");
  }
  catch (DlAbortEx&) {
    // success
  }

  try {
    bw.writeData(nullptr, 1, 0);
    CPPUNIT_FAIL("exception must be thrown");
  }
  catch (DlAbortEx&) {
    // success
  }

  try {
    bw.setString("too large");
    CPPUNIT_FAIL("exception must be thrown");
  }
  catch (DlAbortEx&) {
    // success
  }

  try {
    bw.readData(&byte, 1, -1);
    CPPUNIT_FAIL("exception must be thrown");
  }
  catch (DlAbortEx&) {
    // success
  }

  try {
    bw.readData(nullptr, 1, 0);
    CPPUNIT_FAIL("exception must be thrown");
  }
  catch (DlAbortEx&) {
    // success
  }

  CPPUNIT_ASSERT_EQUAL(std::string("data"), bw.getString());
  CPPUNIT_ASSERT_EQUAL((int64_t)4, bw.size());
}

} // namespace aria2
