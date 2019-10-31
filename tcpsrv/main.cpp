#include "btpro/config.hpp"
#include "btpro/socket.hpp"
#include "btpro/evstack.hpp"
#include "btpro/evheap.hpp"
#include "btpro/evcore.hpp"
#include "btpro/tcp/acceptorfn.hpp"
#include "btpro/tcp/bev.hpp"
#include "btdef/date.hpp"
#include "btpro/tcp/bev.hpp"

#include <iostream>
#include <list>

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

#define MKREFSTR(x, y) \
    static const auto x = btref::mkstr(std::cref(y))


//void rcb(bufferevent *, void *arg)
//{
//    auto c = static_cast<btpro::tcp::bev*>(arg);
//    cout() << "rcb:" << c->input_length() << std::endl;
//    btpro::buffer b(c->input());
//    cout() << "rcb:" << b.str() << std::endl;
//}

//void wcb(bufferevent *, void *arg)
//{
//    auto c = static_cast<btpro::tcp::bev*>(arg);
//    cout() << "wcb: " << c->output_length() << std::endl;
//}

//void evcb(bufferevent *, short what, void *arg)
//{
//    cout() << "evcb: " << what << std::endl;

//    if (what & BEV_EVENT_EOF)
//        cout() << "evcb: eof" << std::endl;
//    if (what & BEV_EVENT_ERROR)
//        cout() << "evcb: error" << std::endl;
//    if (what & BEV_EVENT_TIMEOUT)
//        cout() << "evcb: timeout" << std::endl;
//    if (what & BEV_EVENT_CONNECTED)
//    {
//        cout() << "evcb: connected" << std::endl;

//        MKREFSTR(get_req, "GET / HTTP/1.0\n");
//        MKREFSTR(host_req, "Host: seafile.dev4.fun\n\n");

//        auto c = static_cast<btpro::tcp::bev*>(arg);
//        c->write(get_req.data(), get_req.size(), [&]{
//            cout() << "get sended" << std::endl;
//        });
//        c->write(host_req.data(), host_req.size(), [&]{
//            cout() << "host sended" << std::endl;
//        });
//    }
//    if (what & BEV_EVENT_READING)
//        cout() << "evcb: error encountered while reading" << std::endl;
//    if (what & BEV_EVENT_WRITING)
//        cout() << "evcb: error encountered while writing" << std::endl;
//}

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

class server
{
    btpro::queue queue_{ create_queue() };
    btpro::dns dns_{ queue_, btpro::dns_initialize_nameservers };
    btpro::tcp::acceptorfn<server> acceptor4_{ *this, &server::accept };

    void accept(be::socket sock, be::ip::addr addr)
    {
        MKREFSTR(family_str, "family:");
        MKREFSTR(connect_str, "connect from:");

        be::sock_addr sa(addr);

        cout() << connect_str << ' ' << sa << ' '
               << family_str << ' ' << addr.family() << std::endl;

        sock.close();
    }

public:
    server(int argc, char* argv[])
    {
        btpro::sock_addr addr(btpro::ipv4::any(14889));
        if (argc > 1)
            addr.assign(argv[1]);

        acceptor4_.listen(queue_, LEV_OPT_REUSEABLE_PORT, addr);
    }

    void run()
    {
#ifndef WIN32
        auto f = [&](auto...) {
            MKREFSTR(stop_str, "stop!");
            cerr() << stop_str << std::endl;
            queue_.loop_break();
            return 0;
        };

        be::evcore<be::evstack> sint;
        sint.create(queue_, SIGINT, EV_SIGNAL|EV_PERSIST, f);
        sint.add();

        be::evcore<be::evstack> sterm;
        sterm.create(queue_, SIGTERM, EV_SIGNAL|EV_PERSIST, f);
        sterm.add();
#endif // _WIN32

        queue_.dispatch();
    }
};

int main(int argc, char* argv[])
{
    // инициализация wsa
    btpro::startup();

    server srv(argc, argv);
    srv.run();

    return 0;
}
