#include "AuthConfigFactory.h"

#include <cppunit/extensions/HelperMacros.h>

#include "Netrc.h"
#include "prefs.h"
#include "Request.h"
#include "AuthConfig.h"
#include "Option.h"

namespace aria2 {

class AuthConfigFactoryTest : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(AuthConfigFactoryTest);
  CPPUNIT_TEST(testCreateAuthConfig_http);
  CPPUNIT_TEST(testCreateAuthConfig_httpNoChallenge);
  CPPUNIT_TEST(testDoNotForwardHttpAuthOnCrossOriginRedirect);
  CPPUNIT_TEST(testCreateAuthConfig_ftp);
  CPPUNIT_TEST(testUpdateBasicCred);
  CPPUNIT_TEST_SUITE_END();

public:
  void testCreateAuthConfig_http();
  void testCreateAuthConfig_httpNoChallenge();
  void testDoNotForwardHttpAuthOnCrossOriginRedirect();
  void testCreateAuthConfig_ftp();
  void testUpdateBasicCred();
};

CPPUNIT_TEST_SUITE_REGISTRATION(AuthConfigFactoryTest);

void AuthConfigFactoryTest::testCreateAuthConfig_http()
{
  std::shared_ptr<Request> req(new Request());
  req->setUri("http://localhost/download/aria2-1.0.0.tar.bz2");

  Option option;
  option.put(PREF_NO_NETRC, A2_V_FALSE);
  option.put(PREF_HTTP_AUTH_CHALLENGE, A2_V_TRUE);

  AuthConfigFactory factory;

  // without auth info
  CPPUNIT_ASSERT(!factory.createAuthConfig(req, &option));

  // with Netrc
  auto netrc = make_unique<Netrc>();
  netrc->addAuthenticator(make_unique<Authenticator>(
      "localhost", "localhostuser", "localhostpass", "localhostacct"));
  netrc->addAuthenticator(make_unique<DefaultAuthenticator>(
      "default", "defaultpassword", "defaultaccount"));
  factory.setNetrc(std::move(netrc));

  // not activated
  CPPUNIT_ASSERT(!factory.createAuthConfig(req, &option));

  CPPUNIT_ASSERT(factory.activateBasicCred("localhost", 80, "/", &option));

  CPPUNIT_ASSERT_EQUAL(std::string("localhostuser:localhostpass"),
                       factory.createAuthConfig(req, &option)->getAuthText());

  // See default token in netrc is ignored.
  req->setUri("http://mirror/");

  CPPUNIT_ASSERT(!factory.activateBasicCred("mirror", 80, "/", &option));

  CPPUNIT_ASSERT(!factory.createAuthConfig(req, &option));

  // with Netrc + user defined
  option.put(PREF_HTTP_USER, "userDefinedUser");
  option.put(PREF_HTTP_PASSWD, "userDefinedPassword");

  CPPUNIT_ASSERT(!factory.createAuthConfig(req, &option));

  CPPUNIT_ASSERT(factory.activateBasicCred("mirror", 80, "/", &option));

  CPPUNIT_ASSERT_EQUAL(std::string("userDefinedUser:userDefinedPassword"),
                       factory.createAuthConfig(req, &option)->getAuthText());

  // username and password in URI
  req->setUri(
      "http://aria2user:aria2password@localhost/download/aria2-1.0.0.tar.bz2");
  CPPUNIT_ASSERT_EQUAL(std::string("aria2user:aria2password"),
                       factory.createAuthConfig(req, &option)->getAuthText());
}

void AuthConfigFactoryTest::testCreateAuthConfig_httpNoChallenge()
{
  std::shared_ptr<Request> req(new Request());
  req->setUri("http://localhost/download/aria2-1.0.0.tar.bz2");

  Option option;
  option.put(PREF_NO_NETRC, A2_V_FALSE);

  AuthConfigFactory factory;

  // without auth info
  CPPUNIT_ASSERT(!factory.createAuthConfig(req, &option));

  // with Netrc
  auto netrc = make_unique<Netrc>();
  netrc->addAuthenticator(make_unique<Authenticator>(
      "localhost", "localhostuser", "localhostpass", "localhostacct"));
  netrc->addAuthenticator(make_unique<DefaultAuthenticator>(
      "default", "defaultpassword", "defaultaccount"));
  factory.setNetrc(std::move(netrc));

  // not activated
  CPPUNIT_ASSERT_EQUAL(std::string("localhostuser:localhostpass"),
                       factory.createAuthConfig(req, &option)->getAuthText());

  // See default token in netrc is ignored.
  req->setUri("http://mirror/");

  CPPUNIT_ASSERT(!factory.createAuthConfig(req, &option));

  // with Netrc + user defined
  option.put(PREF_HTTP_USER, "userDefinedUser");
  option.put(PREF_HTTP_PASSWD, "userDefinedPassword");

  CPPUNIT_ASSERT_EQUAL(std::string("userDefinedUser:userDefinedPassword"),
                       factory.createAuthConfig(req, &option)->getAuthText());

  // username and password in URI
  req->setUri(
      "http://aria2user:aria2password@localhost/download/aria2-1.0.0.tar.bz2");
  CPPUNIT_ASSERT_EQUAL(std::string("aria2user:aria2password"),
                       factory.createAuthConfig(req, &option)->getAuthText());
}

void AuthConfigFactoryTest::testDoNotForwardHttpAuthOnCrossOriginRedirect()
{
  Option option;
  option.put(PREF_NO_NETRC, A2_V_TRUE);
  option.put(PREF_HTTP_AUTH_CHALLENGE, A2_V_FALSE);
  option.put(PREF_HTTP_USER, "globalUser");
  option.put(PREF_HTTP_PASSWD, "globalPassword");

  AuthConfigFactory factory;

  auto sameOrigin = std::make_shared<Request>();
  CPPUNIT_ASSERT(sameOrigin->setUri("http://localhost/download/file"));
  CPPUNIT_ASSERT_EQUAL(
      std::string("globalUser:globalPassword"),
      factory.createAuthConfig(sameOrigin, &option)->getAuthText());
  CPPUNIT_ASSERT(sameOrigin->redirectUri("/mirror/file"));
  CPPUNIT_ASSERT(!sameOrigin->isCrossOriginRedirect());
  CPPUNIT_ASSERT_EQUAL(
      std::string("globalUser:globalPassword"),
      factory.createAuthConfig(sameOrigin, &option)->getAuthText());

  auto sameOriginDifferentCase = std::make_shared<Request>();
  CPPUNIT_ASSERT(
      sameOriginDifferentCase->setUri("http://localhost/download/file"));
  CPPUNIT_ASSERT(sameOriginDifferentCase->redirectUri(
      "http://LOCALHOST/mirror/file"));
  CPPUNIT_ASSERT(!sameOriginDifferentCase->isCrossOriginRedirect());
  CPPUNIT_ASSERT_EQUAL(
      std::string("globalUser:globalPassword"),
      factory.createAuthConfig(sameOriginDifferentCase, &option)->getAuthText());

  auto differentHost = std::make_shared<Request>();
  CPPUNIT_ASSERT(differentHost->setUri("http://localhost/download/file"));
  CPPUNIT_ASSERT(differentHost->redirectUri("http://mirror/download/file"));
  CPPUNIT_ASSERT(differentHost->isCrossOriginRedirect());
  CPPUNIT_ASSERT(!factory.createAuthConfig(differentHost, &option));

  auto differentScheme = std::make_shared<Request>();
  CPPUNIT_ASSERT(differentScheme->setUri("http://localhost/download/file"));
  CPPUNIT_ASSERT(
      differentScheme->redirectUri("https://localhost/download/file"));
  CPPUNIT_ASSERT(differentScheme->isCrossOriginRedirect());
  CPPUNIT_ASSERT(!factory.createAuthConfig(differentScheme, &option));

  auto differentPort = std::make_shared<Request>();
  CPPUNIT_ASSERT(differentPort->setUri("http://localhost/download/file"));
  CPPUNIT_ASSERT(
      differentPort->redirectUri("http://localhost:8080/download/file"));
  CPPUNIT_ASSERT(differentPort->isCrossOriginRedirect());
  CPPUNIT_ASSERT(!factory.createAuthConfig(differentPort, &option));

  // Credentials explicitly supplied by the redirect target are safe to use.
  auto explicitCred = std::make_shared<Request>();
  CPPUNIT_ASSERT(explicitCred->setUri("http://localhost/download/file"));
  CPPUNIT_ASSERT(explicitCred->redirectUri(
      "http://redirectUser:redirectPassword@mirror/download/file"));
  CPPUNIT_ASSERT_EQUAL(
      std::string("redirectUser:redirectPassword"),
      factory.createAuthConfig(explicitCred, &option)->getAuthText());

  // Challenge-based authentication must not reactivate global credentials on
  // a cross-origin redirect either.
  option.put(PREF_HTTP_AUTH_CHALLENGE, A2_V_TRUE);
  CPPUNIT_ASSERT(!factory.activateBasicCred(
      differentHost->getHost(), differentHost->getPort(),
      differentHost->getDir(), &option,
      !differentHost->isCrossOriginRedirect()));

  // A host-specific netrc entry for the redirect target remains available.
  option.put(PREF_NO_NETRC, A2_V_FALSE);
  option.put(PREF_HTTP_AUTH_CHALLENGE, A2_V_FALSE);
  auto netrc = make_unique<Netrc>();
  netrc->addAuthenticator(make_unique<Authenticator>(
      "mirror", "mirrorUser", "mirrorPassword", "mirrorAccount"));
  factory.setNetrc(std::move(netrc));
  CPPUNIT_ASSERT_EQUAL(
      std::string("mirrorUser:mirrorPassword"),
      factory.createAuthConfig(differentHost, &option)->getAuthText());
}

void AuthConfigFactoryTest::testCreateAuthConfig_ftp()
{
  std::shared_ptr<Request> req(new Request());
  req->setUri("ftp://localhost/download/aria2-1.0.0.tar.bz2");

  Option option;
  option.put(PREF_NO_NETRC, A2_V_FALSE);

  AuthConfigFactory factory;

  // without auth info
  CPPUNIT_ASSERT_EQUAL(std::string("anonymous:ARIA2USER@"),
                       factory.createAuthConfig(req, &option)->getAuthText());

  // with Netrc
  auto netrc = make_unique<Netrc>();
  netrc->addAuthenticator(make_unique<DefaultAuthenticator>(
      "default", "defaultpassword", "defaultaccount"));
  factory.setNetrc(std::move(netrc));
  CPPUNIT_ASSERT_EQUAL(std::string("default:defaultpassword"),
                       factory.createAuthConfig(req, &option)->getAuthText());

  // disable Netrc
  option.put(PREF_NO_NETRC, A2_V_TRUE);
  CPPUNIT_ASSERT_EQUAL(std::string("anonymous:ARIA2USER@"),
                       factory.createAuthConfig(req, &option)->getAuthText());

  // with Netrc + user defined
  option.put(PREF_NO_NETRC, A2_V_FALSE);
  option.put(PREF_FTP_USER, "userDefinedUser");
  option.put(PREF_FTP_PASSWD, "userDefinedPassword");
  CPPUNIT_ASSERT_EQUAL(std::string("userDefinedUser:userDefinedPassword"),
                       factory.createAuthConfig(req, &option)->getAuthText());

  // username and password in URI
  req->setUri(
      "ftp://aria2user:aria2password@localhost/download/aria2-1.0.0.tar.bz2");
  CPPUNIT_ASSERT_EQUAL(std::string("aria2user:aria2password"),
                       factory.createAuthConfig(req, &option)->getAuthText());

  // username in URI, but no password. We have DefaultAuthenticator
  // but username is not aria2user
  req->setUri("ftp://aria2user@localhost/download/aria2-1.0.0.tar.bz2");
  CPPUNIT_ASSERT_EQUAL(std::string("aria2user:userDefinedPassword"),
                       factory.createAuthConfig(req, &option)->getAuthText());

  // Recreate netrc with entry for user aria2user
  netrc.reset(new Netrc());
  netrc->addAuthenticator(make_unique<Authenticator>("localhost", "aria2user",
                                                     "netrcpass", "netrcacct"));
  factory.setNetrc(std::move(netrc));
  // This time, we can find same username "aria2user" in netrc, so the
  // password "netrcpass" is used, instead of "userDefinedPassword"
  CPPUNIT_ASSERT_EQUAL(std::string("aria2user:netrcpass"),
                       factory.createAuthConfig(req, &option)->getAuthText());
  // No netrc entry for host mirror, so "userDefinedPassword" is used.
  req->setUri("ftp://aria2user@mirror/download/aria2-1.0.0.tar.bz2");
  CPPUNIT_ASSERT_EQUAL(std::string("aria2user:userDefinedPassword"),
                       factory.createAuthConfig(req, &option)->getAuthText());
}

namespace {
std::unique_ptr<BasicCred>
createBasicCred(const std::string& user, const std::string& password,
                const std::string& host, uint16_t port, const std::string& path,
                bool activated = false)
{
  return make_unique<BasicCred>(user, password, host, port, path, activated);
}
} // namespace

void AuthConfigFactoryTest::testUpdateBasicCred()
{
  Option option;
  option.put(PREF_NO_NETRC, A2_V_FALSE);
  option.put(PREF_HTTP_AUTH_CHALLENGE, A2_V_TRUE);

  AuthConfigFactory factory;

  factory.updateBasicCred(
      createBasicCred("myname", "mypass", "localhost", 80, "/", true));
  factory.updateBasicCred(
      createBasicCred("price", "j38jdc", "localhost", 80, "/download", true));
  factory.updateBasicCred(createBasicCred("soap", "planB", "localhost", 80,
                                          "/download/beta", true));
  factory.updateBasicCred(
      createBasicCred("alice", "ium8", "localhost", 80, "/documents", true));
  factory.updateBasicCred(
      createBasicCred("jack", "jackx", "mirror", 80, "/doc", true));

  std::shared_ptr<Request> req(new Request());
  req->setUri("http://localhost/download/v2.6/Changelog");
  CPPUNIT_ASSERT_EQUAL(std::string("price:j38jdc"),
                       factory.createAuthConfig(req, &option)->getAuthText());

  req->setUri("http://localhost/download/beta/v2.7/Changelog");
  CPPUNIT_ASSERT_EQUAL(std::string("soap:planB"),
                       factory.createAuthConfig(req, &option)->getAuthText());

  req->setUri("http://localhost/documents/reference.html");
  CPPUNIT_ASSERT_EQUAL(std::string("alice:ium8"),
                       factory.createAuthConfig(req, &option)->getAuthText());

  req->setUri("http://localhost/documents2/manual.html");
  CPPUNIT_ASSERT_EQUAL(std::string("myname:mypass"),
                       factory.createAuthConfig(req, &option)->getAuthText());

  req->setUri("http://localhost/doc/readme.txt");
  CPPUNIT_ASSERT_EQUAL(std::string("myname:mypass"),
                       factory.createAuthConfig(req, &option)->getAuthText());

  req->setUri("http://localhost:8080/doc/readme.txt");
  CPPUNIT_ASSERT(!factory.createAuthConfig(req, &option));

  req->setUri("http://local/");
  CPPUNIT_ASSERT(!factory.createAuthConfig(req, &option));

  req->setUri("http://mirror/");
  CPPUNIT_ASSERT(!factory.createAuthConfig(req, &option));
}

} // namespace aria2
