#include "btpro/evcore.hpp"
#include "btpro/evstack.hpp"
#include "btpro/uri.hpp"
#include "btdef/date.hpp"
#include "btpro/tcp/bevsock.hpp"
#include "btpro/tcp/bevfn.hpp"
#include "btdef/ref.hpp"

#include <iostream>
#include <list>
#include <string_view>

#ifndef _WIN32
#include <signal.h>
#endif // _WIN32

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

template <class F>
inline std::ostream& cout(F fn)
{
    auto text = fn();
    return std::endl(output(std::cout)
        .write(text.data(), static_cast<std::streamsize>(text.size())));
}

template <class F>
inline std::ostream& cerr(F fn)
{
    auto text = fn();
    return std::endl(output(std::cerr)
        .write(text.data(), static_cast<std::streamsize>(text.size())));
}

#define MKREFSTR(x, y) \
    static const auto x = btref::mkstr(std::cref(y))

btpro::queue create_queue()
{
    MKREFSTR(libevent_str, "libevent-");
    cout() << libevent_str << btpro::queue::version() << ' ' << '-' << ' ';

    btpro::config conf;
    for (auto& i : conf.supported_methods())
        std::cout << i << ' ';
    std::endl(std::cout);

#ifndef _WIN32
    conf.require_features(EV_FEATURE_ET|EV_FEATURE_O1);
#endif //
    return btpro::queue(conf);
}

class queue_core;

template<class T, bool R>
class basic_queue;

template<class T>
class basic_queue<T, true>
    : public queue_core
{
public:
    basic_queue() = default;

    void create();
};

template<>
class basic_queue<false>
{
public:
    basic_queue() = default;

    void create();
};

template<>
basic_queue<bool>::create()
{

}

class peer
{
public:
    typedef btpro::tcp::bevfn<peer> bevfn_type;
    btpro::queue& queue_;
    btpro::dns& dns_;
    bevfn_type bev_;
    bool ready_{true};

    void on_event(short ev, btpro::tcp::bev&)
    {
        if (ev)
        {
            cout() << "peer ev=" << ev << std::endl;
        }
    }

    void on_ready(btpro::tcp::bev&)
    {
        //cout() << "peer ready" << std::endl;
        //ready_ = true;
    }

    void on_recv_send_ask(btpro::tcp::bev& that)
    {
        std::string text;
        auto input = that.input();
        input.drain(text);
        cout() << std::endl << text << std::endl;
    }

    void on_recv_connected(btpro::tcp::bev& that)
    {
        std::string text;
        auto input = that.input();
        input.drain(text);
        cout() << std::endl << text << std::endl;

        static const auto send_text = btref::mkstr(
            std::cref("SEND\r\n"
            "destination:/exchange/ads\r\n"
            "content-type:text/plain\r\n"
            "content-length:5\r\n"
            "\r\n"
            "value"
            "\0"));

        btpro::buffer buf;
        that.write(std::move(
            buf.append_ref(send_text.data(), send_text.size())));

        cout() << std::endl << send_text << std::endl << std::endl;
        bev_.set_recv(&peer::on_recv_send_ask);
    }

    void on_connect(btpro::tcp::bev& that)
    {
        cout() << "peer connected" << std::endl;

        that.enable(EV_READ);

        static const auto conn_str = btref::mkstr(
            std::cref("CONNECT\r\n"
            "accept-version:1.2\r\n"
            "host:fixcl\r\n"
            "login:guest\r\n"
            "passcode:guest\r\n"
            "\r\n\0"));

        btpro::buffer buf;
        that.write(std::move(buf.append_ref(conn_str.data(), conn_str.size())));

        cout() << std::endl << conn_str << std::endl;
        bev_.set_recv(&peer::on_recv_connected);
        bev_.set_ready(&peer::on_ready);
    }

public:
    peer(btpro::queue& queue, btpro::dns& dns, btpro::tcp::bev bev)
        : queue_(queue)
        , dns_(dns)
        , bev_(*this, std::move(bev), &peer::on_event)
    {   }

    void connect(const std::string& hostname, int port)
    {
        bev_.connect(dns_, hostname, port, &peer::on_connect);
    }
};

int main()
{
    try
    {
        // инициализация wsa
        btpro::startup();
        auto queue = create_queue();
        btpro::dns dns(queue, btpro::dns_initialize_nameservers);
        btpro::tcp::bevsock<BEV_OPT_CLOSE_ON_FREE> conn(queue, dns);
        peer p(queue, dns, conn.create());


#ifndef WIN32
        auto f = [&](auto...) {
            MKREFSTR(stop_str, "stop!");
            cerr() << stop_str << std::endl;
            queue.loop_break();
            return 0;
        };

        be::evcore<be::evstack> sint;
        sint.create(queue, SIGINT, EV_SIGNAL|EV_PERSIST, f);
        sint.add();

        be::evcore<be::evstack> sterm;
        sterm.create(queue, SIGTERM, EV_SIGNAL|EV_PERSIST, f);
        sterm.add();
#endif // _WIN32

        p.connect("localhost", 61613);

        queue.dispatch();
    }
    catch (const std::exception& e)
    {
        cerr() << e.what() << std::endl;
    }

    return 0;
}
