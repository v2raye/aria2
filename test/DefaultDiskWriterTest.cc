#include "DefaultDiskWriter.h"
#include <limits>
#include <cppunit/extensions/HelperMacros.h>

#include "a2functional.h"
#include "DlAbortEx.h"

namespace aria2 {

class DefaultDiskWriterTest : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(DefaultDiskWriterTest);
  CPPUNIT_TEST(testSize);
  CPPUNIT_TEST(testRejectInvalidRanges);
  CPPUNIT_TEST_SUITE_END();

private:
public:
  void setUp() {}

  void testSize();
  void testRejectInvalidRanges();
};

CPPUNIT_TEST_SUITE_REGISTRATION(DefaultDiskWriterTest);

void DefaultDiskWriterTest::testSize()
{
  DefaultDiskWriter dw(A2_TEST_DIR "/4096chunk.txt");
  dw.enableReadOnly();
  dw.openExistingFile();
  CPPUNIT_ASSERT_EQUAL((int64_t)4_k, dw.size());
}

void DefaultDiskWriterTest::testRejectInvalidRanges()
{
  DefaultDiskWriter dw(
      A2_TEST_OUT_DIR "/aria2_DefaultDiskWriterTest_invalidRanges");
  dw.initAndOpenFile();
  unsigned char byte = 'x';

  try {
    dw.writeData(&byte, 1, -1);
    CPPUNIT_FAIL("exception must be thrown");
  }
  catch (DlAbortEx&) {
  }

  try {
    dw.writeData(&byte, 2, std::numeric_limits<int64_t>::max() - 1);
    CPPUNIT_FAIL("exception must be thrown");
  }
  catch (DlAbortEx&) {
  }

  try {
    dw.writeData(nullptr, 1, 0);
    CPPUNIT_FAIL("exception must be thrown");
  }
  catch (DlAbortEx&) {
  }

  try {
    dw.readData(&byte, 1, -1);
    CPPUNIT_FAIL("exception must be thrown");
  }
  catch (DlAbortEx&) {
  }

  try {
    dw.readData(nullptr, 1, 0);
    CPPUNIT_FAIL("exception must be thrown");
  }
  catch (DlAbortEx&) {
  }

  try {
    dw.truncate(-1);
    CPPUNIT_FAIL("exception must be thrown");
  }
  catch (DlAbortEx&) {
  }

  try {
    dw.allocate(std::numeric_limits<int64_t>::max(), 1, true);
    CPPUNIT_FAIL("exception must be thrown");
  }
  catch (DlAbortEx&) {
  }
}

} // namespace aria2
