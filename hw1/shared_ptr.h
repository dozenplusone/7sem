#ifndef _HW1_SHARED_PTR_H
#define _HW1_SHARED_PTR_H

#include <compare>
#include <cstddef>
#include <type_traits>

namespace hw1 {
    template<class From, class To>
    concept pointer_compatible =
        std::is_convertible_v<From*, To*> ||
        std::is_bounded_array_v<From> &&
        std::is_unbounded_array_v<To> &&
        std::is_same_v<
            std::remove_extent_t<From>,
            std::remove_cv_t<std::remove_extent_t<To>>
        >;

    template<class T>
    class shared_ptr;
} // namespace hw1

// A smart pointer with reference-counted copy semantics.
template<class T>
class hw1::shared_ptr {
public:
    typedef std::remove_extent_t<T> element_type;
    typedef unsigned long long counter_type;

    template<class C>
    friend class shared_ptr;

private:
    element_type *ptr;              // Contained pointer.
    counter_type *refcount;         // Reference counter.

    void acquire(void);
    void release(void);

public:
    constexpr shared_ptr(std::nullptr_t = nullptr) noexcept;

    template<class C>
    explicit shared_ptr(C*)
    requires pointer_compatible<C, element_type>;

    inline shared_ptr(const shared_ptr&) noexcept;

    template<pointer_compatible<T> C>
    inline shared_ptr(const shared_ptr<C>&) noexcept;

    inline shared_ptr(shared_ptr&&) noexcept;

    template<pointer_compatible<T> C>
    inline shared_ptr(shared_ptr<C>&&) noexcept;

    inline ~shared_ptr();

    shared_ptr &operator=(const shared_ptr&) noexcept;

    template<pointer_compatible<T> C>
    shared_ptr &operator=(const shared_ptr<C>&) noexcept;

    shared_ptr &operator=(shared_ptr&&) noexcept;

    template<pointer_compatible<T> C>
    shared_ptr &operator=(shared_ptr<C>&&) noexcept;

    inline counter_type use_count(void) const noexcept;
    inline element_type *get(void) const noexcept;
    inline element_type &operator*(void) const;

    inline element_type *operator->(void) const
    requires std::is_class_v<T> || std::is_union_v<T>;

    inline element_type &operator[](std::ptrdiff_t) const
    requires std::is_array_v<T>;

    inline void reset(std::nullptr_t = nullptr) noexcept;

    template<class C>
    void reset(C *p)
    requires pointer_compatible<C, element_type>;

    void swap(shared_ptr&) noexcept;

    inline operator bool(void) const noexcept;

    template<class C>
    inline bool operator==(const shared_ptr<C>&) const noexcept;

    inline bool operator==(std::nullptr_t) const noexcept;

    template<class C>
    inline std::strong_ordering operator<=>(const shared_ptr<C>&)
    const noexcept;

    inline std::strong_ordering operator<=>(std::nullptr_t) const noexcept;
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
        if constexpr (std::is_array_v<T>) {
            delete[] ptr;
        } else {
            delete ptr;
        }
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
template<class C>
hw1::shared_ptr<T>::shared_ptr(C *p)
requires pointer_compatible<C, element_type>
    : ptr(static_cast<element_type*>(p))
    , refcount(nullptr)
{
    try {
        acquire();
    } catch (...) {
        release();
        throw;
    }
}

template<class T>
hw1::shared_ptr<T>::shared_ptr(const shared_ptr &obj) noexcept
    : ptr(obj.ptr)
    , refcount(obj.refcount)
{
    acquire();
}

template<class T>
template<hw1::pointer_compatible<T> C>
hw1::shared_ptr<T>::shared_ptr(const shared_ptr<C> &obj) noexcept
    : ptr(static_cast<element_type*>(obj.ptr))
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
template<hw1::pointer_compatible<T> C>
hw1::shared_ptr<T>::shared_ptr(shared_ptr<C> &&obj) noexcept
    : ptr(static_cast<element_type*>(obj.ptr))
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
) noexcept
{
    if (ptr != obj.ptr) {
        release();
        ptr = obj.ptr;
        refcount = obj.refcount;
        acquire();
    }
    return *this;
}

template<class T>
template<hw1::pointer_compatible<T> C>
hw1::shared_ptr<T> &hw1::shared_ptr<T>::operator=(
    const shared_ptr<C> &obj
) noexcept
{
    element_type *cast_obj_ptr = static_cast<element_type*>(obj.ptr);
    if (ptr != cast_obj_ptr) {
        release();
        ptr = cast_obj_ptr;
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

template<class T>
template<hw1::pointer_compatible<T> C>
hw1::shared_ptr<T> &hw1::shared_ptr<T>::operator=(shared_ptr<C> &&obj) noexcept
{
    release();
    ptr = static_cast<element_type*>(obj.ptr);
    refcount = obj.refcount;
    obj.ptr = nullptr;
    obj.refcount = nullptr;
    return *this;
}

// If `*this` owns a pointer, return the number of owners, otherwise zero.
template<class T>
hw1::shared_ptr<T>::counter_type hw1::shared_ptr<T>::use_count(
    void
) const noexcept
{
    return refcount ? *refcount : counter_type();
}

// Return the stored pointer.
template<class T>
hw1::shared_ptr<T>::element_type *hw1::shared_ptr<T>::get(
    void
) const noexcept
{
    return ptr;
}

template<class T>
hw1::shared_ptr<T>::element_type &hw1::shared_ptr<T>::operator*(void) const {
    return *ptr;
}

template<class T>
hw1::shared_ptr<T>::element_type *hw1::shared_ptr<T>::operator->(void) const
requires std::is_class_v<T> || std::is_union_v<T>
{
    return ptr;
}

template<class T>
hw1::shared_ptr<T>::element_type &hw1::shared_ptr<T>::operator[](
    std::ptrdiff_t idx
) const
requires std::is_array_v<T>
{
    return ptr[idx];
}

// Release the ownership of the managed object, if any.
template<class T>
void hw1::shared_ptr<T>::reset(std::nullptr_t) noexcept {
    release();
}

// Replace the managed object with another object.
template<class T>
template<class C>
void hw1::shared_ptr<T>::reset(C *p)
requires pointer_compatible<C, element_type>
{
    release();
    ptr = static_cast<element_type*>(p);
    try {
        acquire();
    } catch (...) {
        release();
        throw;
    }
}

// Exchange the stored pointer values and the ownerships.
template<class T>
void hw1::shared_ptr<T>::swap(shared_ptr &obj) noexcept {
    element_type *ptr_tmp = ptr;
    ptr = obj.ptr;
    obj.ptr = ptr_tmp;

    counter_type *refcount_tmp = refcount;
    refcount = obj.refcount;
    obj.refcount = refcount_tmp;
}

template<class T>
hw1::shared_ptr<T>::operator bool(void) const noexcept {
    return !!ptr;
}

template<class T>
template<class C>
bool hw1::shared_ptr<T>::operator==(const shared_ptr<C> &obj) const noexcept {
    return ptr == obj.ptr;
}

template<class T>
bool hw1::shared_ptr<T>::operator==(std::nullptr_t) const noexcept {
    return !ptr;
}

template<class T>
template<class C>
std::strong_ordering hw1::shared_ptr<T>::operator<=>(
    const shared_ptr<C> &obj
) const noexcept
{
    return std::compare_three_way{}(ptr, obj.ptr);
}

template<class T>
std::strong_ordering hw1::shared_ptr<T>::operator<=>(
    std::nullptr_t
) const noexcept
{
    return ptr ? std::strong_ordering::greater : std::strong_ordering::equal;
}

#endif // _HW1_SHARED_PTR_H
