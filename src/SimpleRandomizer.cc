/* <!-- copyright */
/*
 * aria2 - The high speed download utility
 *
 * Copyright (C) 2006 Tatsuhiro Tsujikawa
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
/* copyright --> */
#include "SimpleRandomizer.h"

#include <sys/types.h>
#include <unistd.h>
#include <algorithm>
#include <cstring>
#include <limits>

#ifdef __APPLE__
#  include <Security/SecRandom.h>
#endif // __APPLE__

#ifdef HAVE_LIBGNUTLS
#  include <gnutls/crypto.h>
#endif // HAVE_LIBGNUTLS

#ifdef HAVE_OPENSSL
#  include <openssl/err.h>
#  include <openssl/rand.h>
#endif // HAVE_OPENSSL

#include "DlAbortEx.h"
#include "fmt.h"

namespace aria2 {

std::unique_ptr<SimpleRandomizer> SimpleRandomizer::randomizer_;

const std::unique_ptr<SimpleRandomizer>& SimpleRandomizer::getInstance()
{
  if (!randomizer_) {
    randomizer_.reset(new SimpleRandomizer());
  }
  return randomizer_;
}

#ifdef __MINGW32__
SimpleRandomizer::SimpleRandomizer() : provider_(0)
{
  if (!::CryptAcquireContext(&provider_, 0, 0, PROV_RSA_FULL,
                             CRYPT_VERIFYCONTEXT | CRYPT_SILENT)) {
    throw DL_ABORT_EX(
        fmt("CryptAcquireContext failed. error=%lu", GetLastError()));
  }
}
#else  // !__MINGW32__
SimpleRandomizer::SimpleRandomizer() = default;
#endif // !__MINGW32__

SimpleRandomizer::~SimpleRandomizer()
{
#ifdef __MINGW32__
  if (provider_) {
    CryptReleaseContext(provider_, 0);
  }
#endif
}

long int SimpleRandomizer::getRandomNumber(long int to)
{
  if (to <= 0) {
    throw DL_ABORT_EX("Random number upper bound must be positive.");
  }
  return std::uniform_int_distribution<long int>(0, to - 1)(*this);
}

void SimpleRandomizer::getRandomBytes(unsigned char* buf, size_t len)
{
  if (len == 0) {
    return;
  }
  if (!buf) {
    throw DL_ABORT_EX("Random byte output buffer is null.");
  }

#ifdef __MINGW32__
  size_t offset = 0;
  while (offset < len) {
    const auto chunk = static_cast<DWORD>(std::min(
        len - offset,
        static_cast<size_t>(std::numeric_limits<DWORD>::max())));
    if (!CryptGenRandom(provider_, chunk,
                        reinterpret_cast<BYTE*>(buf + offset))) {
      throw DL_ABORT_EX(fmt("CryptGenRandom failed. error=%lu", GetLastError()));
    }
    offset += chunk;
  }
#elif defined(__APPLE__)
  const auto rv = SecRandomCopyBytes(kSecRandomDefault, len, buf);
  if (rv != errSecSuccess) {
    throw DL_ABORT_EX(fmt("SecRandomCopyBytes failed. error=%d", rv));
  }
#elif defined(HAVE_LIBGNUTLS)
  const auto rv = gnutls_rnd(GNUTLS_RND_RANDOM, buf, len);
  if (rv != 0) {
    const char* reason = gnutls_strerror(rv);
    throw DL_ABORT_EX(fmt("gnutls_rnd failed. error=%d, cause:%s", rv,
                          reason ? reason : "unknown error"));
  }
#elif defined(HAVE_OPENSSL)
  size_t offset = 0;
  while (offset < len) {
    const auto chunk = static_cast<int>(std::min(
        len - offset,
        static_cast<size_t>(std::numeric_limits<int>::max())));
    if (RAND_bytes(buf + offset, chunk) != 1) {
      const auto error = ERR_get_error();
      throw DL_ABORT_EX(
          fmt("RAND_bytes failed. cause:%s",
              error ? ERR_error_string(error, nullptr) : "unknown error"));
    }
    offset += chunk;
  }
#else
  constexpr static size_t blocklen = 256;
  auto iter = len / blocklen;
  auto p = buf;

  for (size_t i = 0; i < iter; ++i) {
    auto rv = getentropy(p, blocklen);
    if (rv != 0) {
      throw DL_ABORT_EX(
          fmt("getentropy failed. cause:%s", strerror(errno)));
    }

    p += blocklen;
  }

  auto rem = len - iter * blocklen;
  if (rem == 0) {
    return;
  }

  auto rv = getentropy(p, rem);
  if (rv != 0) {
    throw DL_ABORT_EX(fmt("getentropy failed. cause:%s", strerror(errno)));
  }
#endif // !__MINGW32__ && !__APPLE__ && !HAVE_OPENSSL && !HAVE_LIBGNUTLS
}

} // namespace aria2
