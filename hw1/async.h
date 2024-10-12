#ifndef _HW1_ASYNC_H
#define _HW1_ASYNC_H

#include <coroutine>
#include <exception>

namespace hw1 {
    namespace promise_impl {
        struct promise_base;

        template<class T>
        struct promise;
    }

    template<class Return>
    class async;
} // namespace hw1

// A wrapper template class for convenient, some-Python-style
// use of asynchronous functions.
// @tparam Return Type to be returned from the function. `void` is possible.
template<class Return>
class hw1::async {
public:
    using promise_type = promise_impl::promise<Return>;
    using handle_type = std::coroutine_handle<promise_type>;

    friend promise_type;

private:
    handle_type handle;

    async(handle_type hdl) : handle(hdl) {}
    async(const async&) = delete;
    async &operator=(const async&) = delete;

public:
    async(async &&coro) : handle(coro.handle) { coro.handle = nullptr; }
    async &operator=(async&&);
    inline ~async();

    bool resume(void) const;

    bool await_ready(void) const noexcept { return !handle || handle.done(); }
    void await_suspend(std::coroutine_handle<>) const noexcept {}
    Return await_resume(void) const noexcept {
        return handle.promise().result();
    }
}; // class hw1::async

struct hw1::promise_impl::promise_base {
    std::suspend_never initial_suspend(void) { return {}; }
    void unhandled_exception(void) {
        std::rethrow_exception(std::current_exception());
    }
    std::suspend_always final_suspend(void) noexcept { return {}; }
};

template<class T>
struct hw1::promise_impl::promise: hw1::promise_impl::promise_base {
    async<T> get_return_object(void) {
        return {async<T>::handle_type::from_promise(*this)};
    }
    void return_value(const T &val) { value = val; }
    T result(void) const noexcept { return value; }

private:
    T value;
};

template<class T>
struct hw1::promise_impl::promise<T&>: hw1::promise_impl::promise_base {
    async<T&> get_return_object(void) {
        return {async<T&>::handle_type::from_promise(*this)};
    }
    void return_value(T &val) { value = &val; }
    T &result(void) const noexcept { return *value; }

private:
    T *value;
};

template<>
struct hw1::promise_impl::promise<void>: hw1::promise_impl::promise_base {
    async<void> get_return_object(void) {
        return {async<void>::handle_type::from_promise(*this)};
    }
    void return_void(void) {}
    void result(void) const noexcept {}
};

template<class Return>
hw1::async<Return> &hw1::async<Return>::operator=(async &&coro) {
    if (handle) {
        handle.destroy();
    }
    handle = coro.handle;
    coro.handle = nullptr;
    return *this;
}

template<class Return>
hw1::async<Return>::~async() {
    if (handle) {
        handle.destroy();
    }
}

// Resumes function's execution.
// @returns `true` if the function was actually resumed.
template<class Return>
bool hw1::async<Return>::resume(void) const {
    bool ans = handle && !handle.done();
    if (ans) {
        handle.resume();
    }
    return ans;
}

#endif // _HW1_ASYNC_H
