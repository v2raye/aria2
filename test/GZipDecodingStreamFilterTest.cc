#include "GZipDecodingStreamFilter.h"

#include <cassert>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <vector>

#include <cppunit/extensions/HelperMacros.h>

#include "Exception.h"
#include "util.h"
#include "Segment.h"
#include "ByteArrayDiskWriter.h"
#include "SinkStreamFilter.h"
#include "MockSegment.h"
#include "MessageDigest.h"

namespace aria2 {

class GZipDecodingStreamFilterTest : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(GZipDecodingStreamFilterTest);
  CPPUNIT_TEST(testTransform);
  CPPUNIT_TEST(testLargeInputAndReinitialize);
  CPPUNIT_TEST(testInvalidStateAndCorruptInput);
  CPPUNIT_TEST_SUITE_END();

  class MockSegment2 : public MockSegment {
  private:
    int64_t positionToWrite_;

  public:
    MockSegment2() : positionToWrite_(0) {}

    virtual void updateWrittenLength(int64_t bytes) CXX11_OVERRIDE
    {
      positionToWrite_ += bytes;
    }

    virtual int64_t getPositionToWrite() const CXX11_OVERRIDE
    {
      return positionToWrite_;
    }
  };

  std::unique_ptr<GZipDecodingStreamFilter> filter_;
  std::shared_ptr<ByteArrayDiskWriter> writer_;
  std::shared_ptr<MockSegment2> segment_;

public:
  void setUp()
  {
    writer_ = std::make_shared<ByteArrayDiskWriter>();
    auto sinkFilter = make_unique<SinkStreamFilter>();
    sinkFilter->init();
    filter_ = make_unique<GZipDecodingStreamFilter>(std::move(sinkFilter));
    filter_->init();
    segment_ = std::make_shared<MockSegment2>();
  }

  void testTransform();
  void testLargeInputAndReinitialize();
  void testInvalidStateAndCorruptInput();
};

CPPUNIT_TEST_SUITE_REGISTRATION(GZipDecodingStreamFilterTest);

void GZipDecodingStreamFilterTest::testTransform()
{
  unsigned char buf[4_k];
  std::ifstream in(A2_TEST_DIR "/gzip_decode_test.gz", std::ios::binary);
  while (in) {
    in.read(reinterpret_cast<char*>(buf), sizeof(buf));
    filter_->transform(writer_, segment_, buf, in.gcount());
  }
  CPPUNIT_ASSERT(filter_->finished());
  std::string data = writer_->getString();
  std::shared_ptr<MessageDigest> sha1(MessageDigest::sha1());
  sha1->update(data.data(), data.size());
  CPPUNIT_ASSERT_EQUAL(std::string("8b577b33c0411b2be9d4fa74c7402d54a8d21f96"),
                       util::toHex(sha1->digest()));
}

void GZipDecodingStreamFilterTest::testLargeInputAndReinitialize()
{
  std::string input;
  input.reserve(256_k);
  for (size_t i = 0; i < 256_k; ++i) {
    input += static_cast<char>((i * 31 + i / 7) & 0xff);
  }

  uLongf compressedLength = compressBound(input.size());
  std::vector<unsigned char> compressed(compressedLength);
  CPPUNIT_ASSERT_EQUAL(
      Z_OK, compress2(compressed.data(), &compressedLength,
                      reinterpret_cast<const unsigned char*>(input.data()),
                      input.size(), Z_BEST_SPEED));
  compressed.resize(compressedLength);

  size_t offset = 0;
  while (offset < compressed.size()) {
    const size_t length = std::min<size_t>(7, compressed.size() - offset);
    filter_->transform(writer_, segment_, compressed.data() + offset, length);
    const size_t processed = filter_->getBytesProcessed();
    CPPUNIT_ASSERT(processed > 0);
    CPPUNIT_ASSERT(processed <= length);
    offset += processed;
  }
  CPPUNIT_ASSERT(filter_->finished());
  CPPUNIT_ASSERT_EQUAL(input, writer_->getString());

  filter_->init();
  writer_->setString("");
  segment_ = std::make_shared<MockSegment2>();
  CPPUNIT_ASSERT_EQUAL(
      static_cast<ssize_t>(input.size()),
      filter_->transform(writer_, segment_, compressed.data(),
                         compressed.size()));
  CPPUNIT_ASSERT(filter_->finished());
  CPPUNIT_ASSERT_EQUAL(input, writer_->getString());
}

void GZipDecodingStreamFilterTest::testInvalidStateAndCorruptInput()
{
  filter_->release();
  const unsigned char input[] = {0x00};
  try {
    filter_->transform(writer_, segment_, input, sizeof(input));
    CPPUNIT_FAIL("exception must be thrown");
  }
  catch (DlAbortEx&) {
    // success
  }

  filter_->init();
  const unsigned char corrupt[] = {'n', 'o', 't', ' ', 'z', 'l', 'i', 'b'};
  try {
    filter_->transform(writer_, segment_, corrupt, sizeof(corrupt));
    CPPUNIT_FAIL("exception must be thrown");
  }
  catch (DlAbortEx&) {
    // success
  }
}

} // namespace aria2
