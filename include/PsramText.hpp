#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <new>

class String;

struct PsramTextMemoryStats {
  size_t allocatedBytes = 0; // requested bytes, including reference headers
  size_t blocks = 0;
  uint32_t allocationFailures = 0;
};
const PsramTextMemoryStats &psramTextMemoryStats();
void recordPsramTextAllocationFailure();
void *allocatePsramTextBytes(size_t bytes);
void releasePsramTextBytes(void *memory, size_t bytes);

// Immutable, reference-counted text. Persistent records are network-task-owned;
// their buffers never use the internal heap. Record copies are cheap, allowing
// updates to be committed atomically if all new text allocations succeeded.
class PsramText {
 public:
  PsramText() = default;
  PsramText(const char *text) { assign(text); }
  PsramText(const String &text);
  PsramText(const PsramText &other) : block_(other.block_) { retain(); }
  PsramText(PsramText &&other) noexcept : block_(other.block_) { other.block_ = nullptr; }
  ~PsramText() { release(); }

  PsramText &operator=(const PsramText &other) {
    if (this != &other) { release(); block_ = other.block_; retain(); }
    return *this;
  }
  PsramText &operator=(PsramText &&other) noexcept {
    if (this != &other) { release(); block_ = other.block_; other.block_ = nullptr; }
    return *this;
  }
  PsramText &operator=(const char *text) { assign(text); return *this; }
  PsramText &operator=(const String &text);
  operator String() const;

  const char *c_str() const { return block_ ? reinterpret_cast<const char *>(block_ + 1) : ""; }
  size_t length() const { return block_ ? block_->length : 0; }
  bool isEmpty() const { return !length(); }
  bool operator==(const PsramText &other) const {
    return length() == other.length() && std::memcmp(c_str(), other.c_str(), length()) == 0;
  }
  bool operator!=(const PsramText &other) const { return !(*this == other); }
  bool operator==(const char *other) const { return equals(other); }
  bool operator!=(const char *other) const { return !equals(other); }
  bool operator==(const String &other) const;
  bool operator!=(const String &other) const;

  bool assign(const char *text) { return assign(text, text ? std::strlen(text) : 0); }
  bool assign(const char *text, size_t size) {
    if (!text || !size) { release(); return true; }
    if (length() == size && std::memcmp(c_str(), text, size) == 0) return true;
    if (size > std::numeric_limits<size_t>::max() - sizeof(Block) - 1) {
      recordPsramTextAllocationFailure();
      return false;
    }
    const size_t bytes = sizeof(Block) + size + 1;
    void *memory = allocatePsramTextBytes(bytes);
    if (!memory) return false; // preserve old text; caller can discard staged record
    auto *next = new (memory) Block{1, size};
    char *destination = reinterpret_cast<char *>(next + 1);
    std::memcpy(destination, text, size);
    destination[size] = '\0';
    release();
    block_ = next;
    return true;
  }

 private:
  struct Block { uint32_t references; size_t length; };
  Block *block_ = nullptr;
  void retain() { if (block_) ++block_->references; }
  void release() {
    if (block_ && --block_->references == 0)
      releasePsramTextBytes(block_, sizeof(Block) + block_->length + 1);
    block_ = nullptr;
  }
  bool equals(const char *other) const { return std::strcmp(c_str(), other ? other : "") == 0; }
};

class PsramTextTransaction {
 public:
  PsramTextTransaction() : failures_(psramTextMemoryStats().allocationFailures) {}
  bool succeeded() const { return failures_ == psramTextMemoryStats().allocationFailures; }
 private:
  uint32_t failures_;
};
