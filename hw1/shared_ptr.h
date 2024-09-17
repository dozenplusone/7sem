#ifndef _HW1_SHARED_PTR_H
#define _HW1_SHARED_PTR_H

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
}; // class hw1::shared_ptr

#endif // _HW1_SHARED_PTR_H
