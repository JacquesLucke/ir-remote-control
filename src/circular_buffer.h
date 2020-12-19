#pragma once

#include <cassert>

template <typename T, int Capacity> class CircularBuffer {
private:
  T data_[Capacity];
  int current_ = 0;
  bool is_full_ = false;

public:
  CircularBuffer() = default;
  ~CircularBuffer() = default;

  void push(T value) {
    data_[current_] = value;
    current_++;
    if (current_ == Capacity) {
      is_full_ = true;
      current_ = 0;
    }
  }

  int size() const {
    if (is_full_) {
      return Capacity;
    }
    return current_;
  }

  const T &get_least_recently_added(const int index) const {
    assert(index < this->size());
    const int real_index = (current_ - 1 - index + Capacity) % Capacity;
    return data_[real_index];
  }
};
