#include "btpro/evstack.hpp"
#include "btpro/evcore.hpp"
#include "btpro/socket.hpp"
#include "btdef/date.hpp"
#include <iostream>

int main(int argc, char* argv[])
{
    try
    {
        // инициализация wsa
        btpro::startup();

        btpro::config conf;
#ifndef _WIN32
        conf.set_flag(EVENT_BASE_FLAG_EPOLL_USE_CHANGELIST);
        conf.require_features(EV_FEATURE_ET|EV_FEATURE_O1|EV_FEATURE_EARLY_CLOSE);
#endif //
        btpro::queue queue(conf);

        std::cout << "libevent-" << btpro::queue::version() << ' ' << '-' << ' ';
        for (auto& i : conf.supported_methods())
            std::cout << i << ' ';
        std::endl(std::cout);
        std::cout << "use: " << queue.method() << std::endl << std::endl;
        std::cout << "mcsrv test" << std::endl;

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
                std::cout << "sendto: " << dest
                    << " '" << packet << '\'' << std::endl;

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
