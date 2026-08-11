#ifndef D_WIN_TLS_BUFFER_H
#define D_WIN_TLS_BUFFER_H

#include <cstring>
#include <limits>
#include <vector>

namespace aria2 {
namespace wintls {

class Buffer {
private:
  size_t off_;
  size_t free_;
  size_t cap_;
  std::vector<char> buf_;

public:
  Buffer() : off_(0), free_(0), cap_(0) {}

  size_t size() const { return off_; }

  size_t free() const { return free_; }

  bool resize(size_t len)
  {
    if (cap_ >= len) {
      return true;
    }
    try {
      buf_.resize(len);
    }
    catch (...) {
      return false;
    }
    cap_ = buf_.size();
    free_ = cap_ - off_;
    return true;
  }

  char* data() { return buf_.data(); }

  char* end() { return buf_.data() + off_; }

  bool eat(size_t len)
  {
    if (len > off_) {
      return false;
    }
    off_ -= len;
    if (off_) {
      memmove(buf_.data(), buf_.data() + len, off_);
    }
    free_ = cap_ - off_;
    return true;
  }

  void clear()
  {
    off_ = 0;
    free_ = cap_;
  }

  bool advance(size_t len)
  {
    if (len > free_) {
      return false;
    }
    off_ += len;
    free_ -= len;
    return true;
  }

  bool write(const void* data, size_t len)
  {
    if (!len) {
      return true;
    }
    if (!data || len > std::numeric_limits<size_t>::max() - off_ ||
        !resize(off_ + len)) {
      return false;
    }
    memcpy(end(), data, len);
    return advance(len);
  }
};

} // namespace wintls
} // namespace aria2

#endif // D_WIN_TLS_BUFFER_H
