#include "WrDiskCacheEntry.h"

#include <limits>

#include <cstring>

#include <cppunit/extensions/HelperMacros.h>

#include "TestUtil.h"
#include "DirectDiskAdaptor.h"
#include "ByteArrayDiskWriter.h"

namespace aria2 {

class WrDiskCacheEntryTest : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(WrDiskCacheEntryTest);
  CPPUNIT_TEST(testWriteToDisk);
  CPPUNIT_TEST(testAppend);
  CPPUNIT_TEST(testRejectInvalidCell);
  CPPUNIT_TEST(testClear);
  CPPUNIT_TEST_SUITE_END();

  std::shared_ptr<DirectDiskAdaptor> adaptor_;
  ByteArrayDiskWriter* writer_;

public:
  void setUp()
  {
    adaptor_ = std::make_shared<DirectDiskAdaptor>();
    auto dw = make_unique<ByteArrayDiskWriter>();
    writer_ = dw.get();
    adaptor_->setDiskWriter(std::move(dw));
  }

  void testWriteToDisk();
  void testAppend();
  void testRejectInvalidCell();
  void testClear();
};

CPPUNIT_TEST_SUITE_REGISTRATION(WrDiskCacheEntryTest);

void WrDiskCacheEntryTest::testWriteToDisk()
{
  WrDiskCacheEntry e(adaptor_);
  e.cacheData(createDataCell(0, "??01234567", 2));
  e.cacheData(createDataCell(8, "890"));
  e.writeToDisk();
  CPPUNIT_ASSERT_EQUAL((size_t)0, e.getSize());
  CPPUNIT_ASSERT_EQUAL(std::string("01234567890"), writer_->getString());
}

void WrDiskCacheEntryTest::testAppend()
{
  WrDiskCacheEntry e(adaptor_);
  auto cell = new WrDiskCacheEntry::DataCell{};
  cell->goff = 0;
  size_t capacity = 6;
  size_t offset = 2;
  cell->data = new unsigned char[offset + capacity];
  memcpy(cell->data, "??foo", 3);
  cell->offset = offset;
  cell->len = 3;
  cell->capacity = capacity;
  e.cacheData(cell);
  CPPUNIT_ASSERT_EQUAL((size_t)3,
                       e.append(3, (const unsigned char*)"barbaz", 6));
  CPPUNIT_ASSERT_EQUAL((size_t)6, cell->len);
  CPPUNIT_ASSERT_EQUAL((size_t)6, e.getSize());

  CPPUNIT_ASSERT_EQUAL((size_t)0, e.append(7, (const unsigned char*)"FOO", 3));
  CPPUNIT_ASSERT_EQUAL((size_t)0, e.append(6, nullptr, 1));
}

void WrDiskCacheEntryTest::testRejectInvalidCell()
{
  WrDiskCacheEntry e(adaptor_);

  auto cell = new WrDiskCacheEntry::DataCell{};
  cell->goff = std::numeric_limits<int64_t>::max();
  cell->data = new unsigned char[1];
  cell->offset = 0;
  cell->len = 1;
  cell->capacity = 1;
  CPPUNIT_ASSERT(!e.cacheData(cell));
  delete[] cell->data;
  delete cell;

  cell = new WrDiskCacheEntry::DataCell{};
  cell->goff = 0;
  cell->data = new unsigned char[1];
  cell->offset = 0;
  cell->len = 2;
  cell->capacity = 1;
  CPPUNIT_ASSERT(!e.cacheData(cell));
  delete[] cell->data;
  delete cell;
}

void WrDiskCacheEntryTest::testClear()
{
  WrDiskCacheEntry e(adaptor_);
  e.cacheData(createDataCell(0, "foo"));
  e.clear();
  CPPUNIT_ASSERT_EQUAL((size_t)0, e.getSize());
  CPPUNIT_ASSERT_EQUAL(std::string(), writer_->getString());
}

} // namespace aria2
