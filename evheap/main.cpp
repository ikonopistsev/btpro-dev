#include "btpro/evheap.hpp"
#include "btpro/evstack.hpp"
#include "btpro/evcore.hpp"
#include "btdef/date.hpp"

#include <iostream>
#include <signal.h>

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

        evh.destroy();
        evs.destroy();

        q.loop_break();
    };
    evs.create(q, EV_TIMEOUT, l);

    evh.add(std::chrono::milliseconds(250));
    evs.add(std::chrono::milliseconds(550));

    MKREFSTR(run_str, "run");
    cout() << run_str << std::endl;

    q.dispatch();

    return 0;
}
