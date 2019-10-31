#include "btpro/evheap.hpp"
#include "btpro/evstack.hpp"
#include "btpro/evcore.hpp"
#include "btdef/date.hpp"

#include <iostream>
#include <signal.h>
#include <cassert>

std::ostream& output(std::ostream& os)
{
    auto log_time = btdef::date::log_time_text();
    os << log_time << ' ';
    return os;
}

inline std::ostream& cerr()
{
    return output(std::cerr);
}

inline std::ostream& cout()
{
    return output(std::cout);
}

#define MKREFSTR(x, y) \
    static const auto x = btref::mkstr(std::cref(y))

void call(evutil_socket_t, short, void *)
{
    MKREFSTR(method_str, "method!");
    cout() << method_str << std::endl;
}

template<class T>
struct fun
{
    static inline void call(evutil_socket_t, short, void *arg_fn) noexcept
    {
        assert(arg_fn);
        auto fn = static_cast<T*>(arg_fn);
        try
        {
            (*fn)();
        } catch (...)
        {   }
        delete fn;
    }
};

template<class T, class F>
void run_once(btpro::queue& q, T timeout, F fn)
{
    auto arg_fn = new std::function<void()>(fn);
    q.once(timeout, fun<std::function<void()>>::call, arg_fn);
}

int main(int, char**)
{
    btpro::startup();

    btpro::queue q;

    btpro::evcore<btpro::evheap> evh;
    evh.create(q, EV_TIMEOUT, call, nullptr);

    btpro::evcore<btpro::evstack> evs;
    auto l = [&](...){
        MKREFSTR(lambda_str, "+lambda!");
        cout() << lambda_str << std::endl;
        q.loop_break();
    };
    evs.create(q, EV_TIMEOUT, l);

    evh.add(std::chrono::milliseconds(300));
    evs.add(std::chrono::milliseconds(500));

    run_once(q, std::chrono::milliseconds(400), []{
        MKREFSTR(fun_str, "+fun!");
        cout() << fun_str << std::endl;
    });

    // one shot
    auto rshot = [&](...){
        MKREFSTR(rshot_str, "rshot");
        cout() << rshot_str << std::endl;
    };
    q.once(std::chrono::milliseconds(100), std::ref(rshot));

    q.once(EV_PERSIST, std::chrono::milliseconds(110), [&](...){
        MKREFSTR(lambdashot_str, "lambda");
        cout() << lambdashot_str << std::endl;
    });

    MKREFSTR(run_str, "run");
    cout() << run_str << std::endl;

    q.dispatch();

    return 0;
}
