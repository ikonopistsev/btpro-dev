#include "btpro/config.hpp"
#include "btpro/socket.hpp"
#include "btpro/evstack.hpp"
#include "btpro/evheap.hpp"
#include "btpro/evcore.hpp"
#include "btpro/tcp/acceptorfn.hpp"
#include "btpro/tcp/bev.hpp"
#include "btdef/date.hpp"

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
    btpro::tcp::acceptorfn<server> acceptor4_{ *this, &server::accept };
    std::list<btpro::tcp::bev> peer_{};

    void accept(be::socket sock, be::ip::addr addr)
    {
        MKREFSTR(family_str, "family:");
        MKREFSTR(connect_str, "connect from:");

        be::sock_addr sa(addr);

        cout() << connect_str << ' ' << sa << ' '
               << family_str << ' ' << addr.family() << std::endl;

        auto peer = peer_.emplace(peer_.end(),
            queue_, sock, BEV_OPT_CLOSE_ON_FREE);

        // готовим буфер для отправки
        MKREFSTR(bye_str, "bye!");

        // отправляем статический буфер
        // указатель на пир и адрес подклчения копируем
        peer->write_ref(bye_str.data(), bye_str.size(), [&, peer, sa]{

            MKREFSTR(close_str, "close:");
            cout() << close_str << ' ' << sa << std::endl;

            // удаляем пир
            // это приведет к закрытию сокета (BEV_OPT_CLOSE_ON_FREE)
            peer_.erase(peer);
        });
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
