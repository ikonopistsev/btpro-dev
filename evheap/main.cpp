#include "btpro/ev.hpp"
#include "btpro/queue.hpp"
#include "btpro/thread.hpp"
#include "btdef/date.hpp"

#include <thread>
#include <iostream>
#include <signal.h>
#include <cassert>
#include <array>
#include <charconv>

std::ostream& output(std::ostream& os)
{
    auto log_time = btdef::date::log_time();
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

class proxy_test
{
    btpro::queue& queue_;

    btpro::timer_fn<proxy_test> timer_{
        &proxy_test::do_timer, *this };

    btpro::socket_fn<proxy_test> socket_{
        &proxy_test::do_socket, *this };

    btpro::generic_fn<proxy_test> generic_{
        &proxy_test::do_generic, *this };

    btpro::timer_fun timer_fun_{[]{
        cout() << "+timer fun" << std::endl;
    }};
    btpro::ev_heap evh_{queue_, -1, EV_TIMEOUT, 
        std::chrono::milliseconds(570), timer_fun_};

    btpro::timer_fn<proxy_test> timer_fn_{
        &proxy_test::do_fn, *this };
    btpro::ev_stack evs_{};

    btpro::evh::socket_fn<proxy_test> socket_fn_{queue_, btpro::socket(), 
        EV_TIMEOUT, std::chrono::milliseconds(600), {&proxy_test::do_fn, *this}};

    btpro::evh::timer just_timer_{queue_, EV_TIMEOUT,
        std::chrono::milliseconds(650), []{
            cout() << "just timer!" << std::endl;
        }};

public:
    proxy_test(btpro::queue& queue)
        : queue_(queue)
    {   
        evs_.create(queue_, timer_fn_);
        evs_.add(std::chrono::milliseconds(550));

        queue.once(std::chrono::milliseconds(400), timer_);
        queue.once(std::chrono::milliseconds(450), socket_);
        queue.once(std::chrono::milliseconds(500), generic_);

    }

    void do_timer()
    {
        cout() << "do_timer" << std::endl;
    }    

    void do_socket(btpro::socket, short)
    {
        cout() << "do_socket" << std::endl;
    }   

    void do_generic(evutil_socket_t, short)
    {
        cout() << "do_generic" << std::endl;
    }   

    void do_fn()
    {
        cout() << "+timer fn" << std::endl;
    }    

    void do_fn(btpro::socket, short)
    {
        cout() << "+socket fn" << std::endl;
    }    
};

void test_queue(btpro::queue& queue)
{
    queue.once(std::chrono::milliseconds(150), [](btpro::socket, short){
        cout() << "socket lambda" << std::endl;
    });

    static btpro::timer_fun f1 = []{
        cout() << "static fun" << std::endl;
    };
    queue.once(std::chrono::milliseconds(200), f1);

    btpro::timer_fun f2 = []{
        cout() << "move timer" << std::endl;
    };
    queue.once(std::chrono::milliseconds(250), std::move(f2));    

    auto f3 = [](evutil_socket_t, short) {
        cout() << "generic lambda" << std::endl;
    };
    queue.once(std::chrono::milliseconds(300), f3);
}

void test_requeue(btpro::queue& q1, btpro::queue& q2)
{
    q1.once(q2, std::chrono::milliseconds(450), 
        []{
            cout() << "timer requeue"sv << std::endl;
    });

    q1.once(q2, btpro::socket(), EV_TIMEOUT, 
        std::chrono::milliseconds(450), 
            [](btpro::socket, short){
                cout() << "socket requeue"sv << std::endl;
    });

    q1.once(q2, -1, EV_TIMEOUT, 
        std::chrono::milliseconds(500), 
            [](evutil_socket_t, short){
                cout() << "generic requeue"sv << std::endl;
    });
}

int run()
{
    btpro::queue queue;
    btpro::queue thr_queue;
    std::thread thr([&]{
        thr_queue.loopexit(std::chrono::seconds(2));
        thr_queue.loop(EVLOOP_NO_EXIT_ON_EMPTY);
    });

    test_requeue(queue, thr_queue);
    test_queue(queue);
    proxy_test p(queue);

    btpro::timer_fun fh = []{
        cout() << "ev_heap timer!"sv << std::endl;
    };
    btpro::ev_heap evh(queue, -1, EV_TIMEOUT, 
        std::chrono::milliseconds(600), fh);

    btpro::timer_fun fs = []{
        cout() << "ev_stack timer!"sv << std::endl;
    };
    btpro::ev_stack evs(queue, -1, EV_TIMEOUT, 
        std::chrono::milliseconds(650), fs);

    cout() << "run"sv << std::endl;

    queue.dispatch();

    cout() << "join"sv << std::endl;
    thr.join();

    return 0;
}

void atoiperf() 
{
    constexpr std::array<std::string_view, 12> arr = {
        "124345536"sv , "65878456745"sv, "934535667856"sv, 
        "124345536"sv , "65878456745"sv, "934535667856"sv,
        "124345536"sv , "65878456745"sv, "934535667856"sv, 
        "124345536"sv , "65878456745"sv, "99999999999999999"sv 
    };


    std::size_t count = 10000000;
    std::int64_t n;

    auto b = btdef::date::now();
    for (std::size_t i = 0; i < count; ++i) {
        for (auto& text : std::as_const(arr)) {
            n = btdef::conv::antou(text.data(), text.size());
        }
    }
    cout() << "btdef::conv::antou: " << n << ' ' << btdef::date::diff_abs(b, btdef::date::now()) << std::endl;

    b = btdef::date::now();
    for (std::size_t i = 0; i < count; ++i) {
        for (auto& text : arr) {
            std::from_chars(text.data(), text.data() + text.size(), n);
        }
    }
    cout() << "from_chars: " << n << ' ' << btdef::date::diff_abs(b, btdef::date::now()) << std::endl;
}

int main()
{
    btpro::startup();
    btpro::use_threads();
    atoiperf();
    run();

    return 0;
}
