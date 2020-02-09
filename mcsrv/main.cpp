#include "btpro/queue.hpp"
#include "btpro/evcore.hpp"
#include "btpro/socket.hpp"
#include "btdef/date.hpp"

#include <iostream>

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
    btpro::queue queue;
    queue.create(conf);
    return queue;
}

// - using  mcsrv dst_host:dst_port
// - or     mcsrv dst_host dst_port

int main(int argc, char* argv[])
{
    try
    {
        // инициализация wsa
        btpro::startup();

        MKREFSTR(use_str, "use: ");
        MKREFSTR(mccl_test_str, "mcsrv test");

        auto queue = create_queue();
        cout() << use_str << queue.method() << std::endl << std::endl;
        cout() << mccl_test_str << std::endl;

        // адрес и порт для рассылки
        btpro::ipv4::addr dest("224.0.0.42", 4587);
        if (argc == 2)
        {
            // парсим адрес и порт из строки прим. "224.0.0.42:4587"
            dest.assign(argv[1]);
        }

        if (argc == 3)
        {
            // парсим адрес из строки прим. "224.0.0.42" а порт из параметра
            dest.assign(argv[1], std::atoi(argv[2]));
        }

        btpro::socket socket;
        // создаем сокет на произвольном порту
        socket.create(dest.family(), btpro::sock_dgram);

        btpro::evcore<btpro::evstack> ev;

        auto fn = [&](evutil_socket_t fd, btpro::event_flag_t) {
            // подключаем сокет
            try
            {
                // формируем пакет - дата
                btdef::date time(queue.gettimeofday_cached());
                auto packet = time.json_text();

                MKREFSTR(sendto_str, "sendto: ");
                cout() << sendto_str << dest << ' '
                    << '\'' << packet << '\'' << std::endl;

                // отправляем пакет
                btpro::socket sock(fd);
                auto res = sock.sendto(dest, packet.data(), packet.size());
                if (btpro::code::fail == res)
                    throw std::system_error(btpro::net::error_code(), "sendto");
            }
            catch (const std::exception& e)
            {
                std::cerr << e.what() << std::endl;
            }
        };

        // создаем таймер рассылки
        ev.create(queue, socket, EV_TIMEOUT|EV_PERSIST, fn);

        // таймер отправки заводим на три секунды
        ev.add(std::chrono::milliseconds(100));
        // сразу же делаем рассылку
        ev.active(EV_TIMEOUT);

#ifndef WIN32
        be::evcore<be::evstack> sint;
        be::evcore<be::evstack> sterm;

        auto f = [&](auto...) {
            MKREFSTR(stop_str, "stop!");
            cerr() << stop_str << std::endl;
            queue.loop_break();
        };

        sint.create(queue, SIGINT, EV_SIGNAL|EV_PERSIST, f);
        sint.add();

        sterm.create(queue, SIGTERM, EV_SIGNAL|EV_PERSIST, f);
        sterm.add();
#endif // _WIN32

        queue.dispatch();

        // закроем сокет перед выходом
        socket.close();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
