#include "GZipEncoder.h"

#include <cppunit/extensions/HelperMacros.h>

#include "GZipDecoder.h"
#include "RecoverableException.h"
#include "util.h"

namespace aria2 {

class GZipEncoderTest : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(GZipEncoderTest);
  CPPUNIT_TEST(testEncode);
  CPPUNIT_TEST(testLargeInput);
  CPPUNIT_TEST(testStateTransitions);
  CPPUNIT_TEST_SUITE_END();

public:
  void testEncode();
  void testLargeInput();
  void testStateTransitions();
};

CPPUNIT_TEST_SUITE_REGISTRATION(GZipEncoderTest);

void GZipEncoderTest::testEncode()
{
  GZipEncoder encoder;
  encoder.init();

  std::vector<std::string> inputs;
  inputs.push_back("Hello World");
  inputs.push_back("9223372036854775807");
  inputs.push_back("Fox");

  encoder << inputs[0];
  encoder << (int64_t)9223372036854775807LL;
  encoder << inputs[2].c_str();

  std::string gzippedData = encoder.str();

  GZipDecoder decoder;
  decoder.init();
  std::string gunzippedData =
      decoder.decode(reinterpret_cast<const unsigned char*>(gzippedData.data()),
                     gzippedData.size());
  CPPUNIT_ASSERT(decoder.finished());
  CPPUNIT_ASSERT_EQUAL(strjoin(inputs.begin(), inputs.end(), ""),
                       gunzippedData);
}

void GZipEncoderTest::testLargeInput()
{
  std::string input(256_k, '\0');
  uint32_t state = 1;
  for (auto& c : input) {
    state = state * 1103515245 + 12345;
    c = static_cast<char>(state >> 24);
  }

  GZipEncoder encoder;
  encoder.init();
  encoder.write(input.data(), input.size());
  std::string gzippedData = encoder.str();

  GZipDecoder decoder;
  decoder.init();
  std::string decoded = decoder.decode(
      reinterpret_cast<const unsigned char*>(gzippedData.data()),
      gzippedData.size());
  CPPUNIT_ASSERT(decoder.finished());
  CPPUNIT_ASSERT_EQUAL(input, decoded);
}

void GZipEncoderTest::testStateTransitions()
{
  GZipEncoder encoder;
  try {
    encoder << "not initialized";
    CPPUNIT_FAIL("Encoding without init() must fail.");
  }
  catch (RecoverableException&) {
    // success
  }

  encoder.init();
  encoder << "first";
  const std::string first = encoder.str();
  CPPUNIT_ASSERT_EQUAL(first, encoder.str());
  try {
    encoder << "after finish";
    CPPUNIT_FAIL("Encoding after str() must fail.");
  }
  catch (RecoverableException&) {
    // success
  }

  encoder.init();
  encoder << "second";
  const std::string second = encoder.str();
  GZipDecoder decoder;
  decoder.init();
  CPPUNIT_ASSERT_EQUAL(
      std::string("second"),
      decoder.decode(reinterpret_cast<const unsigned char*>(second.data()),
                     second.size()));
}

} // namespace aria2
