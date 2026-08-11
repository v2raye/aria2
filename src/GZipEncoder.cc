/* <!-- copyright */
/*
 * aria2 - The high speed download utility
 *
 * Copyright (C) 2010 Tatsuhiro Tsujikawa
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
#include "GZipEncoder.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>

#include "fmt.h"
#include "DlAbortEx.h"
#include "util.h"

namespace aria2 {

GZipEncoder::GZipEncoder() : strm_(nullptr), finished_(false) {}

GZipEncoder::~GZipEncoder() { release(); }

void GZipEncoder::init()
{
  release();
  internalBuf_.clear();
  auto stream = make_unique<z_stream>();
  memset(stream.get(), 0, sizeof(z_stream));

  int rv = deflateInit2(stream.get(), Z_DEFAULT_COMPRESSION, Z_DEFLATED, 31, 9,
                        Z_DEFAULT_STRATEGY);
  if (rv != Z_OK) {
    throw DL_ABORT_EX(
        fmt("Initializing z_stream failed. cause:%s", zError(rv)));
  }
  strm_ = stream.release();
}

void GZipEncoder::release()
{
  if (strm_) {
    deflateEnd(strm_);
    delete strm_;
    strm_ = nullptr;
  }
  finished_ = false;
}

std::string GZipEncoder::encode(const unsigned char* in, size_t length,
                                int flush)
{
  if (!strm_) {
    throw DL_ABORT_EX("GZipEncoder is not initialized.");
  }
  if (finished_) {
    throw DL_ABORT_EX("GZipEncoder is already finished.");
  }
  if (length != 0 && !in) {
    throw DL_ABORT_EX("GZipEncoder input is null.");
  }
  if (length == 0 && flush == Z_NO_FLUSH) {
    return std::string();
  }

  std::string out;
  std::array<unsigned char, 4_k> outbuf;
  size_t remaining = length;
  const unsigned char* next = in;

  for (;;) {
    if (strm_->avail_in == 0 && remaining != 0) {
      const auto chunk = static_cast<uInt>(std::min(
          remaining,
          static_cast<size_t>(std::numeric_limits<uInt>::max())));
      strm_->avail_in = chunk;
      strm_->next_in = const_cast<unsigned char*>(next);
      next += chunk;
      remaining -= chunk;
    }

    const int currentFlush = remaining == 0 ? flush : Z_NO_FLUSH;
    const uInt availInBefore = strm_->avail_in;
    strm_->avail_out = outbuf.size();
    strm_->next_out = outbuf.data();
    int ret = ::deflate(strm_, currentFlush);
    if (ret != Z_OK && ret != Z_STREAM_END) {
      throw DL_ABORT_EX(fmt("libz::deflate() failed. cause:%s",
                            strm_->msg ? strm_->msg : zError(ret)));
    }
    if (strm_->avail_in > availInBefore ||
        strm_->avail_out > outbuf.size()) {
      throw DL_ABORT_EX("libz::deflate() returned invalid buffer lengths.");
    }

    const size_t consumed = availInBefore - strm_->avail_in;
    const size_t produced = outbuf.size() - strm_->avail_out;
    out.append(reinterpret_cast<const char*>(outbuf.data()), produced);

    if (ret == Z_STREAM_END) {
      finished_ = true;
      break;
    }
    if (remaining == 0 && strm_->avail_in == 0 &&
        currentFlush == Z_NO_FLUSH && strm_->avail_out != 0) {
      break;
    }
    if (consumed == 0 && produced == 0) {
      throw DL_ABORT_EX("libz::deflate() made no progress.");
    }
  }
  return out;
}

std::string GZipEncoder::str()
{
  if (!finished_) {
    internalBuf_ += encode(nullptr, 0, Z_FINISH);
  }
  return internalBuf_;
}

GZipEncoder& GZipEncoder::operator<<(const char* s)
{
  internalBuf_ += encode(reinterpret_cast<const unsigned char*>(s), strlen(s));
  return *this;
}

GZipEncoder& GZipEncoder::operator<<(const std::string& s)
{
  internalBuf_ +=
      encode(reinterpret_cast<const unsigned char*>(s.data()), s.size());
  return *this;
}

GZipEncoder& GZipEncoder::operator<<(int64_t i)
{
  std::string s = util::itos(i);
  (*this) << s;
  return *this;
}

GZipEncoder& GZipEncoder::write(const char* s, size_t length)
{
  internalBuf_ += encode(reinterpret_cast<const unsigned char*>(s), length);
  return *this;
}

} // namespace aria2
