#include "btpro/evstack.hpp"
#include "btpro/evcore.hpp"
#include "btpro/socket.hpp"
#include "btdef/date.hpp"

#include <iostream>

std::ostream& output(std::ostream& os)
{
    auto log_time = btdef::date::log_time_text();
    os << log_time << ' ';
    return os;
}

std::ostream& cerr()
{
    return output(std::cerr);
}

std::ostream& cout()
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

        auto fn = [&](evutil_socket_t fd, btpro::event_flag_t) mutable {
            // подключаем сокет
            try
            {
                // формируем пакет - дата
                auto packet = utility::date(queue.gettimeofday_cached()).json_text();

                MKREFSTR(sendto_str, "sendto: ");
                cout() << sendto_str << dest << ' '
                    << '\'' << packet << '\'' << std::endl;

                // отправляем пакет
                btpro::socket sock(fd);
                auto res = sock.sendto(dest, packet.data(), packet.size());
                if (btpro::code::fail == res)
                    throw std::system_error(btpro::net::error_code(), "sendto");

                return 0;
            }
            catch (const std::exception& e)
            {
                std::cerr << e.what() << std::endl;
            }
            return 0;
        };

        // создаем таймер рассылки
        ev.create(queue, socket, EV_TIMEOUT|EV_PERSIST, fn);

        // таймер отправки заводим на три секунды
        ev.add(std::chrono::milliseconds(100));
        // сразу же делаем рассылку
        ev.active(EV_TIMEOUT);

        queue.dispatch();

        socket.close();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
