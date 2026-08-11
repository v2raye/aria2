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
#include "GZipDecodingStreamFilter.h"

#include <limits>

#include "fmt.h"
#include "DlAbortEx.h"

namespace aria2 {

const std::string GZipDecodingStreamFilter::NAME("GZipDecodingStreamFilter");

GZipDecodingStreamFilter::GZipDecodingStreamFilter(
    std::unique_ptr<StreamFilter> delegate)
    : StreamFilter{std::move(delegate)},
      strm_{nullptr},
      finished_{false},
      bytesProcessed_{0}
{
}

GZipDecodingStreamFilter::~GZipDecodingStreamFilter() { release(); }

void GZipDecodingStreamFilter::init()
{
  auto candidate = make_unique<z_stream>();
  *candidate = {};

  // initialize z_stream with gzip/zlib format auto detection enabled.
  const int rv = inflateInit2(candidate.get(), 47);
  if (rv != Z_OK) {
    const char* reason = candidate->msg ? candidate->msg : zError(rv);
    throw DL_ABORT_EX(
        fmt("Initializing z_stream failed. code=%d, cause:%s", rv,
            reason ? reason : "unknown error"));
  }

  release();
  strm_ = candidate.release();
}

void GZipDecodingStreamFilter::release()
{
  if (strm_) {
    inflateEnd(strm_);
    delete strm_;
    strm_ = nullptr;
  }
  finished_ = false;
  bytesProcessed_ = 0;
}

ssize_t
GZipDecodingStreamFilter::transform(const std::shared_ptr<BinaryStream>& out,
                                    const std::shared_ptr<Segment>& segment,
                                    const unsigned char* inbuf, size_t inlen)
{
  bytesProcessed_ = 0;
  ssize_t outlen = 0;
  if (!strm_) {
    throw DL_ABORT_EX("GZipDecodingStreamFilter is not initialized.");
  }
  if (!getDelegate()) {
    throw DL_ABORT_EX("GZipDecodingStreamFilter has no delegate.");
  }
  if (finished_ || inlen == 0) {
    return outlen;
  }
  if (!inbuf) {
    throw DL_ABORT_EX("GZipDecodingStreamFilter received a null input buffer.");
  }

  size_t inputOffset = 0;
  unsigned char outbuf[OUTBUF_LENGTH];
  while (inputOffset < inlen && !finished_) {
    const auto chunk = static_cast<uInt>(std::min(
        inlen - inputOffset,
        static_cast<size_t>(std::numeric_limits<uInt>::max())));
    strm_->avail_in = chunk;
    strm_->next_in = const_cast<unsigned char*>(inbuf + inputOffset);

    do {
      const auto availInBefore = strm_->avail_in;
      strm_->avail_out = OUTBUF_LENGTH;
      strm_->next_out = outbuf;

      const int rv = ::inflate(strm_, Z_NO_FLUSH);
      const size_t consumed = availInBefore - strm_->avail_in;
      const size_t produced = OUTBUF_LENGTH - strm_->avail_out;
      inputOffset += consumed;
      bytesProcessed_ = inputOffset;

      if (produced > 0) {
        const auto written =
            getDelegate()->transform(out, segment, outbuf, produced);
        if (written < 0 || static_cast<size_t>(written) != produced) {
          throw DL_ABORT_EX(
              "GZipDecodingStreamFilter delegate did not consume all data.");
        }
        if (written > std::numeric_limits<ssize_t>::max() - outlen) {
          throw DL_ABORT_EX("GZipDecodingStreamFilter output size overflow.");
        }
        outlen += written;
      }

      if (rv == Z_STREAM_END) {
        finished_ = true;
        break;
      }
      if (rv != Z_OK && rv != Z_BUF_ERROR) {
        const char* reason = strm_->msg ? strm_->msg : zError(rv);
        throw DL_ABORT_EX(fmt("libz::inflate() failed. code=%d, cause:%s", rv,
                              reason ? reason : "unknown error"));
      }
      if (consumed == 0 && produced == 0) {
        if (rv == Z_BUF_ERROR && strm_->avail_in == 0) {
          break;
        }
        throw DL_ABORT_EX("libz::inflate() made no progress.");
      }
    } while (strm_->avail_in > 0 || strm_->avail_out == 0);
  }
  return outlen;
}

bool GZipDecodingStreamFilter::finished()
{
  return finished_ && getDelegate() && getDelegate()->finished();
}

const std::string& GZipDecodingStreamFilter::getName() const { return NAME; }

} // namespace aria2
