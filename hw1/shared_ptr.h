#ifndef _HW1_SHARED_PTR_H
#define _HW1_SHARED_PTR_H

#include <cstddef>

namespace hw1 {
    template<class T>
    class shared_ptr;
} // namespace hw1

// A smart pointer with reference-counted copy semantics.
template<class T>
class hw1::shared_ptr {
public:
    typedef T element_type;
    typedef unsigned long long counter_type;

private:
    element_type *ptr;              // Contained pointer.
    counter_type *refcount;         // Reference counter.

    void acquire(void);
    void release(void);

public:
    constexpr shared_ptr(std::nullptr_t = nullptr) noexcept;
    inline explicit shared_ptr(element_type*);
    inline shared_ptr(const shared_ptr&) noexcept;
    inline shared_ptr(shared_ptr&&) noexcept;

    inline ~shared_ptr();

    shared_ptr &operator=(const shared_ptr&) noexcept;
    shared_ptr &operator=(shared_ptr&&) noexcept;

    inline counter_type use_count(void) const noexcept;
    inline element_type *get(void) const noexcept;
    inline element_type &operator*(void) const;
    inline element_type *operator->(void) const;
}; // class hw1::shared_ptr

// Increment reference counter or create one.
template<class T>
void hw1::shared_ptr<T>::acquire(void) {
    if (ptr) {
        if (!refcount) {
            refcount = new counter_type();
        }
        ++*refcount;
    }
}

// Decrement reference counter and, if reaching zero, free data.
template<class T>
void hw1::shared_ptr<T>::release(void) {
    if (!(refcount && --*refcount)) {
        delete refcount;
        delete ptr;
    }
    ptr = nullptr;
    refcount = nullptr;
}

template<class T>
constexpr hw1::shared_ptr<T>::shared_ptr(std::nullptr_t) noexcept
    : ptr(nullptr)
    , refcount(nullptr)
{}

template<class T>
hw1::shared_ptr<T>::shared_ptr(element_type *p)
    : ptr(p)
    , refcount(nullptr)
{
    acquire();
}

template<class T>
hw1::shared_ptr<T>::shared_ptr(const shared_ptr &obj) noexcept
    : ptr(obj.ptr)
    , refcount(obj.refcount)
{
    acquire();
}

template<class T>
hw1::shared_ptr<T>::shared_ptr(shared_ptr &&obj) noexcept
    : ptr(obj.ptr)
    , refcount(obj.refcount)
{
    obj.ptr = nullptr;
    obj.refcount = nullptr;
}

template<class T>
hw1::shared_ptr<T>::~shared_ptr() {
    release();
}

template<class T>
hw1::shared_ptr<T> &hw1::shared_ptr<T>::operator=(
    const shared_ptr &obj
) noexcept {
    if (ptr != obj.ptr) {
        release();
        ptr = obj.ptr;
        refcount = obj.refcount;
        acquire();
    }
    return *this;
}

template<class T>
hw1::shared_ptr<T> &hw1::shared_ptr<T>::operator=(shared_ptr &&obj) noexcept {
    release();
    ptr = obj.ptr;
    refcount = obj.refcount;
    obj.ptr = nullptr;
    obj.refcount = nullptr;
    return *this;
}

// If `*this` owns a pointer, return the number of owners, otherwise zero.
template<class T>
hw1::shared_ptr<T>::counter_type hw1::shared_ptr<T>::use_count(
    void
) const noexcept {
    return refcount ? *refcount : counter_type();
}

// Return the stored pointer.
template<class T>
hw1::shared_ptr<T>::element_type *hw1::shared_ptr<T>::get(
    void
) const noexcept {
    return ptr;
}

template<class T>
hw1::shared_ptr<T>::element_type &hw1::shared_ptr<T>::operator*(void) const {
    return *ptr;
}

template<class T>
hw1::shared_ptr<T>::element_type *hw1::shared_ptr<T>::operator->(void) const {
    return ptr;
}

#endif // _HW1_SHARED_PTR_H
