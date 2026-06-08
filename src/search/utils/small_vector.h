#ifndef SEARCH_UTILS_SMALL_VECTOR_H
#define SEARCH_UTILS_SMALL_VECTOR_H

#include <cstddef>
#include <iterator>
#include <new>
#include <utility>

namespace utils {

/*
  A minimal small-buffer-optimized vector.

  Stores up to N elements inline (in a buffer embedded in the object, so no
  heap allocation) and spills to a heap buffer only when it grows past N. This
  is a self-contained, dependency-free stand-in for the subset of
  std::vector/boost::container::small_vector API that the search component uses
  (the project must not depend on boost). Element types used here (int, Term)
  are small and trivially copyable, but the implementation is correct for any
  copyable/movable T: it placement-news and explicitly destroys elements.

  Value semantics match std::vector<T>: same elements, same order, element-wise
  operator==, and iteration via raw pointers (so utils::hash_range / TupleHash
  produce identical hashes to the previous std::vector<int> tuples).
*/
template <typename T, std::size_t N>
class small_vector {
public:
    using value_type = T;
    using size_type = std::size_t;
    using iterator = T *;
    using const_iterator = const T *;

    small_vector() noexcept : data_(inline_storage()), size_(0), capacity_(N) {}

    template <typename InputIt>
    small_vector(InputIt first, InputIt last)
        : data_(inline_storage()), size_(0), capacity_(N) {
        for (; first != last; ++first) {
            emplace_back(*first);
        }
    }

    small_vector(const small_vector &other)
        : data_(inline_storage()), size_(0), capacity_(N) {
        reserve(other.size_);
        for (size_type i = 0; i < other.size_; ++i) {
            ::new (static_cast<void *>(data_ + i)) T(other.data_[i]);
        }
        size_ = other.size_;
    }

    small_vector(small_vector &&other) noexcept
        : data_(inline_storage()), size_(0), capacity_(N) {
        move_from(other);
    }

    small_vector &operator=(const small_vector &other) {
        if (this != &other) {
            clear();
            reserve(other.size_);
            for (size_type i = 0; i < other.size_; ++i) {
                ::new (static_cast<void *>(data_ + i)) T(other.data_[i]);
            }
            size_ = other.size_;
        }
        return *this;
    }

    small_vector &operator=(small_vector &&other) noexcept {
        if (this != &other) {
            destroy_and_free();
            data_ = inline_storage();
            size_ = 0;
            capacity_ = N;
            move_from(other);
        }
        return *this;
    }

    ~small_vector() { destroy_and_free(); }

    void reserve(size_type n) {
        if (n <= capacity_) return;
        size_type new_cap = capacity_ * 2;
        if (new_cap < n) new_cap = n;
        T *new_data = static_cast<T *>(::operator new(new_cap * sizeof(T)));
        for (size_type i = 0; i < size_; ++i) {
            ::new (static_cast<void *>(new_data + i)) T(std::move(data_[i]));
            data_[i].~T();
        }
        if (!is_inline()) ::operator delete(data_);
        data_ = new_data;
        capacity_ = new_cap;
    }

    void push_back(const T &v) { emplace_back(v); }
    void push_back(T &&v) { emplace_back(std::move(v)); }

    template <typename... Args>
    T &emplace_back(Args &&... args) {
        if (size_ == capacity_) reserve(size_ + 1);
        T *p = data_ + size_;
        ::new (static_cast<void *>(p)) T(std::forward<Args>(args)...);
        ++size_;
        return *p;
    }

    // Range insert. The search code only ever appends at end(); the general
    // (mid-vector) case is implemented for completeness/safety.
    template <typename InputIt>
    iterator insert(const_iterator pos, InputIt first, InputIt last) {
        size_type idx = static_cast<size_type>(pos - data_);
        if (idx == size_) {
            for (; first != last; ++first) emplace_back(*first);
            return data_ + idx;
        }
        small_vector tail(data_ + idx, data_ + size_);
        for (size_type i = size_; i > idx; --i) data_[i - 1].~T();
        size_ = idx;
        for (; first != last; ++first) emplace_back(*first);
        for (size_type i = 0; i < tail.size_; ++i) {
            emplace_back(std::move(tail.data_[i]));
        }
        return data_ + idx;
    }

    void resize(size_type n) {
        if (n < size_) {
            for (size_type i = n; i < size_; ++i) data_[i].~T();
            size_ = n;
        } else if (n > size_) {
            reserve(n);
            for (size_type i = size_; i < n; ++i) {
                ::new (static_cast<void *>(data_ + i)) T();
            }
            size_ = n;
        }
    }

    void clear() {
        for (size_type i = 0; i < size_; ++i) data_[i].~T();
        size_ = 0;
    }

    T &operator[](size_type i) { return data_[i]; }
    const T &operator[](size_type i) const { return data_[i]; }

    T &back() { return data_[size_ - 1]; }
    const T &back() const { return data_[size_ - 1]; }

    iterator begin() { return data_; }
    iterator end() { return data_ + size_; }
    const_iterator begin() const { return data_; }
    const_iterator end() const { return data_ + size_; }

    size_type size() const { return size_; }
    bool empty() const { return size_ == 0; }

    bool operator==(const small_vector &o) const {
        if (size_ != o.size_) return false;
        for (size_type i = 0; i < size_; ++i) {
            if (!(data_[i] == o.data_[i])) return false;
        }
        return true;
    }
    bool operator!=(const small_vector &o) const { return !(*this == o); }

private:
    T *inline_storage() { return reinterpret_cast<T *>(buf_); }
    bool is_inline() const {
        return data_ == reinterpret_cast<const T *>(buf_);
    }

    void destroy_and_free() {
        for (size_type i = 0; i < size_; ++i) data_[i].~T();
        if (!is_inline()) ::operator delete(data_);
        size_ = 0;
    }

    // Take ownership of `other`'s elements. Leaves `other` empty (but valid).
    void move_from(small_vector &other) {
        if (!other.is_inline()) {
            // Source is heap-allocated: steal the buffer outright.
            data_ = other.data_;
            size_ = other.size_;
            capacity_ = other.capacity_;
            other.data_ = other.inline_storage();
            other.size_ = 0;
            other.capacity_ = N;
        } else {
            // Source is inline: its storage is part of `other`, so we must
            // move-construct each element into our own (inline) buffer.
            for (size_type i = 0; i < other.size_; ++i) {
                ::new (static_cast<void *>(data_ + i)) T(std::move(other.data_[i]));
            }
            size_ = other.size_;
            other.clear();
        }
    }

    T *data_;
    size_type size_;
    size_type capacity_;
    alignas(T) unsigned char buf_[N * sizeof(T)];
};

}  // namespace utils

#endif  // SEARCH_UTILS_SMALL_VECTOR_H
