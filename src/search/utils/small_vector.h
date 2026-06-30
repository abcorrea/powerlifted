#ifndef UTILS_SMALL_VECTOR_H
#define UTILS_SMALL_VECTOR_H

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <utility>

namespace utils {

/*
 * Minimal small-buffer-optimized vector for trivially-copyable element types.
 *
 * Keeps up to N elements inline (no heap allocation), spilling to the heap only
 * beyond that. Vendored in-tree (the way parallel_hashmap is) so the planner
 * keeps depending only on the C++ standard library -- this is NOT an external
 * dependency and NOT a general std::vector drop-in: it is deliberately tiny and
 * restricted to trivially-copyable T (so growth is a plain memcpy and no element
 * destructors are needed).
 *
 * It is used as the storage of datalog::Arguments, which is copied once per
 * produced ground fact in the weighted grounder's hot loop. Typical planning
 * atoms have a small arity, so the inline buffer removes that per-fact heap
 * allocation entirely.
 *
 * operator== compares element by element with T's own operator==, so a container
 * of Arguments hashes and compares exactly as the previous std::vector<Term>
 * storage did.
 */
template <typename T, std::size_t N>
class small_vector {
    static_assert(std::is_trivially_copyable<T>::value,
                  "small_vector only supports trivially-copyable element types");

    T *data_;
    std::uint32_t size_;
    std::uint32_t capacity_;
    alignas(T) unsigned char inline_storage_[N * sizeof(T)];

    T *inline_ptr() { return reinterpret_cast<T *>(inline_storage_); }
    const T *inline_ptr() const { return reinterpret_cast<const T *>(inline_storage_); }
    bool is_inline() const { return data_ == inline_ptr(); }

    void grow(std::uint32_t min_capacity) {
        std::uint32_t new_capacity = capacity_ * 2;
        if (new_capacity < min_capacity) new_capacity = min_capacity;
        T *new_data = static_cast<T *>(::operator new(new_capacity * sizeof(T)));
        std::memcpy(new_data, data_, size_ * sizeof(T));
        if (!is_inline()) ::operator delete(data_);
        data_ = new_data;
        capacity_ = new_capacity;
    }

    void assign_from(const T *src, std::uint32_t n) {
        if (n > capacity_) grow(n);
        std::memcpy(data_, src, n * sizeof(T));
        size_ = n;
    }

public:
    using value_type = T;
    using iterator = T *;
    using const_iterator = const T *;

    small_vector() : data_(inline_ptr()), size_(0), capacity_(N) {}

    small_vector(const small_vector &o) : data_(inline_ptr()), size_(0), capacity_(N) {
        assign_from(o.data_, o.size_);
    }

    small_vector(small_vector &&o) noexcept : data_(inline_ptr()), size_(0), capacity_(N) {
        steal(o);
    }

    template <typename InputIt>
    small_vector(InputIt first, InputIt last) : data_(inline_ptr()), size_(0), capacity_(N) {
        for (; first != last; ++first) push_back(*first);
    }

    small_vector &operator=(const small_vector &o) {
        if (this != &o) {
            size_ = 0;
            assign_from(o.data_, o.size_);
        }
        return *this;
    }

    small_vector &operator=(small_vector &&o) noexcept {
        if (this != &o) {
            if (!is_inline()) ::operator delete(data_);
            data_ = inline_ptr();
            size_ = 0;
            capacity_ = N;
            steal(o);
        }
        return *this;
    }

    ~small_vector() {
        if (!is_inline()) ::operator delete(data_);
    }

    void reserve(std::size_t n) {
        if (n > capacity_) grow(static_cast<std::uint32_t>(n));
    }

    void push_back(const T &v) {
        if (size_ == capacity_) grow(size_ + 1);
        data_[size_++] = v;
    }

    template <typename... Args>
    void emplace_back(Args &&... args) {
        if (size_ == capacity_) grow(size_ + 1);
        data_[size_++] = T(std::forward<Args>(args)...);
    }

    T &operator[](std::size_t i) { return data_[i]; }
    const T &operator[](std::size_t i) const { return data_[i]; }

    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    iterator begin() { return data_; }
    iterator end() { return data_ + size_; }
    const_iterator begin() const { return data_; }
    const_iterator end() const { return data_ + size_; }

    bool operator==(const small_vector &o) const {
        if (size_ != o.size_) return false;
        for (std::uint32_t i = 0; i < size_; ++i) {
            if (!(data_[i] == o.data_[i])) return false;
        }
        return true;
    }
    bool operator!=(const small_vector &o) const { return !(*this == o); }

private:
    // Take ownership of o's contents, leaving o empty. `this` must currently be
    // empty and inline.
    void steal(small_vector &o) {
        if (o.is_inline()) {
            std::memcpy(inline_ptr(), o.data_, o.size_ * sizeof(T));
            size_ = o.size_;
        } else {
            data_ = o.data_;
            size_ = o.size_;
            capacity_ = o.capacity_;
        }
        o.data_ = o.inline_ptr();
        o.size_ = 0;
        o.capacity_ = N;
    }
};

}  // namespace utils

#endif  // UTILS_SMALL_VECTOR_H
