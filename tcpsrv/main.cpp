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

class server
{
    btpro::queue queue_;
    btpro::dns dns_;
    btpro::tcp::acceptorfn<server> acceptor4_{ *this, &server::accept };

    void accept(be::socket sock, be::ip::addr addr)
    {
        MKREFSTR(family_str, "family:");
        MKREFSTR(connect_str, "connect from:");

        be::sock_addr sa(addr);

        cout() << connect_str << ' ' << sa << ' '
               << family_str << ' ' << addr.family() << std::endl;

        btpro::buffer in;
        auto sz = in.read(sock, 64);
        if (sz > 0)
            cout() << in.str() << std::endl;
       
        btpro::buffer out;
        out.append(btdef::date(queue_).to_log_time());
        out.write(sock);

        sock.close();
    }

    void create()
    {
        MKREFSTR(libevent_str, "libevent-");
        cout() << libevent_str << btpro::queue::version() << ' ' << '-' << ' ';

        btpro::config conf;
        for (auto& i : conf.supported_methods())
            std::cout << i << ' ';
        std::endl(std::cout);

#ifndef _WIN32
        conf.require_features(EV_FEATURE_ET | EV_FEATURE_O1);
#endif //
        queue_.create(conf);
        dns_.create(queue_, btpro::dns_initialize_nameservers);
    }

public:
    server(int argc, char* argv[])
    {
        create();

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
