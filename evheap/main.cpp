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

void call(evutil_socket_t, short, void *)
{
    cout() << "method!"sv << std::endl;
}

class proxy_test
{
    btpro::queue& queue_;

    using ev_type = btpro::evs::timer_fn<proxy_test>;
    ev_type evs_{ queue_, EV_TIMEOUT, 
        std::chrono::milliseconds(310), { &proxy_test::call, *this }};

    btpro::evh::timer evtf_{queue_, EV_TIMEOUT, 
        std::chrono::milliseconds(320), 
        std::bind(&proxy_test::call_bind, this) };

    btpro::timer_fun my_timer_ = []{
        cout() << "proxy ref!"sv << std::endl;
    };

    btpro::evs::timer evsf_{queue_, EV_TIMEOUT, 
        std::chrono::milliseconds(330), std::ref(my_timer_) };

    btpro::timer_fn<proxy_test> timer_{
        &proxy_test::do_timer, *this };

public:
    proxy_test(btpro::queue& queue)
        : queue_(queue)
    {   
        queue.once(std::chrono::milliseconds(400), timer_);
    }

    void call()
    {
        cout() << "proxy!"sv << std::endl;
    }

    void call_bind()
    {
        cout() << "bind!"sv << std::endl;
    }

    void do_timer()
    {
        cout() << "do_timer" << std::endl;
    }    
};

void test_event(btpro::queue& queue)
{
    static btpro::ev_stack stev1;
    stev1.create(queue, -1, EV_TIMEOUT, [](auto...){
        cout() << "stev1" << std::endl;
    }, nullptr);
    stev1.add(std::chrono::milliseconds(340));
    
    static btpro::ev_heap stev2(queue, -1, EV_TIMEOUT, [](auto...){
        cout() << "stev2" << std::endl;
    }, nullptr);
    stev2.add(std::chrono::milliseconds(350));

    static btpro::stack_event stev3(queue, -1, EV_TIMEOUT, [](auto...){
        cout() << "stev3" << std::endl;
    }, nullptr);
    auto tv = btpro::make_timeval(std::chrono::seconds(1));
    btpro::detail::check_result("event_add", event_add(stev3, &tv));
    //event_del(stev3);
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

int run()
{
    btpro::queue queue;
    // работает только с use_threads();
    btpro::queue thr_queue;
    std::thread thr([&]{
        // запускаем на 2 секунды вторую очередь
        thr_queue.loopexit(std::chrono::seconds(2));
        thr_queue.loop(EVLOOP_NO_EXIT_ON_EMPTY);
    });

    test_timer_requeue(queue, thr_queue);
    test_event(queue);
    test_queue_timer_ref(queue);

    btpro::evh::type evh;
    evh.create(queue, -1, EV_TIMEOUT, call, nullptr);

    btpro::evs::type evs;
    auto l = [&](...) {
        auto lambda_str = "+lambda!"sv;
        cout() << lambda_str << std::endl;
        queue.loop_break();
    };
    evs.create(queue, EV_TIMEOUT, l);

    evh.add(std::chrono::milliseconds(300));
    evs.add(std::chrono::milliseconds(1200));

    // one shot
    auto lambda = [&] {
        auto lambda_str = "lambda"sv;
        cout() << lambda_str << std::endl;
    };
    queue.once(std::chrono::milliseconds(100), std::ref(lambda));

    queue.once(std::chrono::milliseconds(110), []{
        cout() << "fn"sv << std::endl;
    });

    cout() << "run"sv << std::endl;

    proxy_test p(queue);

    queue.dispatch();

    thr.join();

    return 0;
}

int main()
{
    btpro::startup();
    btpro::use_threads();

    run();

    return 0;
}
