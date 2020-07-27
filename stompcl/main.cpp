#include "btpro/evcore.hpp"
#include "btpro/evstack.hpp"
#include "btpro/uri.hpp"
#include "btdef/date.hpp"
#include "btpro/tcp/bevfn.hpp"
#include "btdef/ref.hpp"
#include "stomptalk/v12.hpp"
#include "stomptalk/rabbitmq.hpp"
#include "stomptalk/parser.hpp"
#include "stomptalk/parser_hook.hpp"
#include "stomptalk/antoull.hpp"
#include "stomptalk/btpro/connection.hpp"
#include "stomptalk/v12.hpp"

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

inline std::ostream& endl2(std::ostream& os)
{
    return os << std::endl << std::endl;
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
    btpro::queue q;
    q.create(conf);
    return q;
}

class peer
{
    typedef stomptalk::tcp::connection<peer>
        connection_type;

    btpro::queue_ref queue_;
    btpro::dns_ref dns_;

    connection_type conn_{ queue_, *this,
        &peer::on_event, &peer::on_connect, &peer::on_logon };

public:
    peer(btpro::queue_ref queue, btpro::dns_ref dns)
        : queue_(queue)
        , dns_(dns)
    {   }

    void connect(const std::string& host, int port)
    {
        cout([&]{
            std::string text("connect to: ");
            text += host;
            if (port)
                text += ' ' + std::to_string(port);
            return text;
        });

        conn_.connect(dns_, host, port);
    }

    void on_event(short)
    {
        // любое событие приводик к закрытию сокета
        queue_.once(std::chrono::seconds(5), [&](...){
            connect("192.168.10.77", 61613);
        });
    }

    void on_connect()
    {
        conn_.logon(stomptalk::v12::connect("one", "bob", "bobone"));
    }

    void on_logon()
    {
        stomptalk::tcp::subscribe subs("/queue/stompcl",
            [&](btpro::buffer buf) {
                cout() << "RECEIVE: " << buf.str() << endl2;
                return;
                stomptalk::tcp::send send("/queue/stompcl");
                auto time_text = btdef::date::to_log_time();
                send.payload(btpro::buffer(time_text));
                conn_.send(std::move(send), [=]{
                    cout() << "SENDED: " << time_text << endl2;
                });
        });

        conn_.subscribe(std::move(subs), [&]{
            stomptalk::tcp::send send("/queue/stompcl");
            auto time_text = btdef::date::to_log_time();
            send.payload(btpro::buffer(time_text));
            conn_.send(std::move(send), [=]{
                cout() << "SENDED: " << time_text << endl2;
            });
        });

    }
};

int main()
{
    try
    {
        // инициализация wsa
        btpro::startup();
        btpro::dns dns;
        auto queue = create_queue();
        dns.create(queue, btpro::dns_initialize_nameservers);

        peer p(queue, dns);

        //stomptalk::asked ask;
        //cout() << ask.id() << std::endl;
#ifndef WIN32
        auto f = [&](auto...) {
            MKREFSTR(stop_str, "stop!");
            cerr() << stop_str << std::endl;
            queue.loop_break();
            return 0;
        };

        btpro::evs sint;
        sint.create(queue, SIGINT, EV_SIGNAL|EV_PERSIST, f);
        sint.add();

        btpro::evs sterm;
        sterm.create(queue, SIGTERM, EV_SIGNAL|EV_PERSIST, f);
        sterm.add();
#endif // _WIN32

        p.connect("192.168.10.77", 61613);

        queue.dispatch();
    }
    catch (const std::exception& e)
    {
        cerr() << e.what() << std::endl;
    }

    return 0;
}
