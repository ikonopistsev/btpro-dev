//#define _CRTDBG_MAP_ALLOC
//#include <stdlib.h>
//#include <crtdbg.h>

#include "btpro/queue.hpp"
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

int run()
{
    btpro::queue queue;
    queue.create();
    btpro::queue_ref q(queue);

    btpro::evh evh;
    evh.create(q, EV_TIMEOUT, call, nullptr);

    btpro::evs evs;
    auto l = [&](...) {
        MKREFSTR(lambda_str, "+lambda!");
        cout() << lambda_str << std::endl;
        q.loop_break();
    };
    evs.create(q, EV_TIMEOUT, l);

    evh.add(std::chrono::milliseconds(300));
    evs.add(std::chrono::milliseconds(500));

    // one shot
    auto lambda = [&](...) {
        MKREFSTR(lambda_str, "lambda");
        cout() << lambda_str << std::endl;
    };
    q.once(std::chrono::milliseconds(100), std::ref(lambda));

    q.once(std::chrono::milliseconds(110), [&](...) {
        MKREFSTR(fn_str, "fn");
        cout() << fn_str << std::endl;
        });

    MKREFSTR(run_str, "run");
    cout() << run_str << std::endl;

    q.dispatch();

    return 0;
}

int main(int, char**)
{
    btpro::startup();

    run();

    //_CrtDumpMemoryLeaks();

    return 0;
}
