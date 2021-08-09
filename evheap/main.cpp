//#define _CRTDBG_MAP_ALLOC
//#include <stdlib.h>
//#include <crtdbg.h>

#include "btpro/queue.hpp"
#include "btpro/evcore.hpp"
#include "btdef/date.hpp"
#include "btpro/buffer.hpp"

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

using namespace std::literals;

void call(evutil_socket_t, short, void *)
{
    auto method_str = "method!"sv;
    cout() << method_str << std::endl;
}

class proxy_test
{
    btpro::queue& queue_;
    btpro::evtfn<proxy_test> evh_{ *this, &proxy_test::call };
    btpro::evs ev_{};
public:
    proxy_test(btpro::queue& queue)
        : queue_(queue)
    {
        ev_.create(queue_, EV_TIMEOUT, evh_);
        ev_.add(std::chrono::milliseconds(450));
    }

    void call()
    {
        auto proxy_str = "proxy!"sv;
        cout() << proxy_str << std::endl;
    }
};

int run()
{
    btpro::buffer buf;
    buf.append(std::string("text"));
    btpro::buffer bv;
    bv.append("51"sv);
    btpro::buffer bt;
    bt.append(btdef::text("123"));
    {
        btpro::buffer_ref ref(buf);
        btpro::buffer_ref btr(bt);
        std::cout << "ref: " << ref.str() << std::endl;
        ref.remove(btr);
        std::cout << "btr: " << btr.str() << std::endl;
        std::cout << "buf: " << buf.str() << std::endl;
    }

    auto t = btdef::date::now().json_text();
    std::cout << t << std::endl;
    btdef::date d(t);
    std::cout << d.json_text() << std::endl;

    btpro::queue queue;
    queue.create();

    btpro::evh evh;
    evh.create(queue, EV_TIMEOUT, call, nullptr);

    btpro::evs evs;
    auto l = [&](...) {
        auto lambda_str = "+lambda!"sv;
        cout() << lambda_str << std::endl;
        queue.loop_break();
    };
    evs.create(queue, EV_TIMEOUT, l);

    evh.add(std::chrono::milliseconds(300));
    evs.add(std::chrono::milliseconds(500));


    //ev.set(ev::call(q1, EV_TIMEOUT, l), ev::call(q2, l2));

    // one shot
    auto lambda = [&](...) {
        auto lambda_str = "lambda"sv;
        cout() << lambda_str << std::endl;
    };
    queue.once(std::chrono::milliseconds(100), std::ref(lambda));

    queue.once(std::chrono::milliseconds(110), [&](...) {
        auto fn_str = "fn"sv;
        cout() << fn_str << std::endl;
        });

    auto run_str = "run"sv;
    cout() << run_str << std::endl;

    proxy_test p(queue);

    queue.dispatch();

    return 0;
}

int main(int, char**)
{
    btpro::startup();

    run();

    //_CrtDumpMemoryLeaks();

    return 0;
}
