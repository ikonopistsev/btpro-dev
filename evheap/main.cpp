//#define _CRTDBG_MAP_ALLOC
//#include <stdlib.h>
//#include <crtdbg.h>

#include "btpro/queue.hpp"
#include "btpro/thread.hpp"
#include "btpro/evcore.hpp"
#include "btdef/date.hpp"
#include "btpro/buffer.hpp"
#include "btpro/dns.hpp"
#include "btpro/ev.hpp"

#include <thread>
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

// void call(evutil_socket_t, short, void *)
// {
//     auto method_str = "method!"sv;
//     cout() << method_str << std::endl;
// }



class proxy_test
{
    btpro::queue& queue_;

    using ev_type = btpro::evs::timer_fn<proxy_test>;
    ev_type evs_{ queue_, EV_TIMEOUT, 
        std::chrono::milliseconds(350), { &proxy_test::call, *this }};

    btpro::evh::timer evtf_{queue_, EV_TIMEOUT, 
        std::chrono::milliseconds(300), 
        std::bind(&proxy_test::call_bind, this) };

public:
    proxy_test(btpro::queue& queue)
        : queue_(queue)
    {   }

    void call()
    {
        cout() << "proxy!"sv << std::endl;
    }

    void call_bind()
    {
        cout() << "bind!"sv << std::endl;
    }
};

void test_stack_event(btpro::queue& queue)
{
    timeval tv{1, 0};
    btpro::ev_stack stev1;
    stev1.create(queue, -1, EV_TIMEOUT, [](auto...){
        cout() << "stev1" << std::endl;
    }, nullptr);
    stev1.add(tv);
    
    btpro::ev_heap stev2(queue, -1, EV_TIMEOUT, [](auto...){
        cout() << "stev2" << std::endl;
    }, nullptr);
    stev2.add(tv);

    btpro::stack_event stev3(queue, -1, EV_TIMEOUT, [](auto...){
        cout() << "stev3" << std::endl;
    }, nullptr);
    btpro::detail::check_result("event_add",
            event_add(stev3, &tv));
    event_del(stev3);
}

void test_heap_event(btpro::queue& queue)
{
    timeval tv{1, 0};
    btpro::heap_event hev1;

    btpro::heap_event hev2(queue, -1, EV_TIMEOUT, [](auto...){}, nullptr);
    btpro::detail::check_result("event_add",
            event_add(hev2, &tv));
}

struct timer_with_fn
{
    btpro::timer_fn<timer_with_fn> timer_{
        &timer_with_fn::do_timer, *this };

    void run(btpro::queue& queue)
    {
        queue.once(timer_);
    }

    void do_timer()
    {
        cout() << "do_timer" << std::endl;
    }
};

void test_timer_with_fn(timer_with_fn& t, btpro::queue& queue)
{
    t.run(queue);
}

void test_timer_requeue(btpro::queue& q1, btpro::queue& q2)
{
    q1.once(q2, std::chrono::milliseconds(20), []{
        cout() << "timer requeue" << std::endl;
    });

    q1.once(q2, btpro::socket(), EV_TIMEOUT, std::chrono::milliseconds(30), 
        [](auto...){
            cout() << "socket requeue" << std::endl;
        });
}

void test_queue_timer_ref(btpro::queue& queue)
{
    static auto f = []{
        cout() << "static ref!" << std::endl;
    };
    // function reference
    queue.once(std::chrono::milliseconds(110), std::ref(f));
}

void test_ev_core_timer(btpro::queue& queue)
{
    static auto f = []{
        cout() << "core timer!" << std::endl;
    };

    //btpro::ev_heap e{queue, EV_TIMEOUT, std::chrono::milliseconds(234), std::ref(f)};
    //e.create_then_add(queue, EV_TIMEOUT, std::chrono::milliseconds(234), f);
}

int run()
{
    // работает только с use_threads();
    btpro::queue thr_queue;
    std::thread thr([&]{
        // запускаем на 2 секунды вторую очередь
        thr_queue.loopexit(std::chrono::seconds(2));
        thr_queue.loop(EVLOOP_NO_EXIT_ON_EMPTY);
    });

    btpro::queue queue;
    //btpro::dns dns(queue);

    timer_with_fn t;

    test_queue_timer_ref(queue);
    test_ev_core_timer(queue);
    test_stack_event(queue);
    test_heap_event(queue);
    test_timer_with_fn(t, queue);
    test_timer_requeue(queue, thr_queue);
    queue.dispatch();

    // btpro::evh evh;
    // evh.create(queue, EV_TIMEOUT, call, nullptr);

    // btpro::evs evs;
    // auto l = [&](...) {
    //     auto lambda_str = "+lambda!"sv;
    //     cout() << lambda_str << std::endl;
    //     queue.loop_break();
    // };
    // evs.create(queue, EV_TIMEOUT, l);

    // evh.add(std::chrono::milliseconds(300));
    // evs.add(std::chrono::milliseconds(500));


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

    thr.join();

    return 0;
}

int main(int, char**)
{
    btpro::startup();
    btpro::use_threads();

    run();

    //_CrtDumpMemoryLeaks();

    return 0;
}
