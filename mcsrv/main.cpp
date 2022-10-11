#include "btpro/queue.hpp"
#include "btpro/ev.hpp"
#include "btpro/socket.hpp"
#include "btdef/date.hpp"

#include <iostream>
#include <string_view>
#include <signal.h>

using namespace std::literals;

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

btpro::queue create_queue()
{
    cout() << "libevent-"sv << btpro::queue::version() << ' ' << '-' << ' ';

    btpro::config conf;
    for (auto& i : conf.supported_methods())
        std::cout << i << ' ';
    std::endl(std::cout);

#ifndef _WIN32
    conf.require_features(EV_FEATURE_ET|EV_FEATURE_O1);
#endif //
    return btpro::queue(conf);
}

// - using  mcsrv dst_host:dst_port
// - or     mcsrv dst_host dst_port

int main(int argc, char* argv[])
{
    try
    {
        // инициализация wsa
        btpro::startup();

        auto queue = create_queue();
        cout() << "use: "sv << queue.method() << std::endl << std::endl;
        cout() << "mcsrv test"sv << std::endl;

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
        btpro::socket::guard sock_guard(socket);
        // создаем сокет на произвольном порту
        socket.create(dest.family(), btpro::sock_dgram);

        auto fn = [&](btpro::socket sock, btpro::event_flag) {
            try
            {
                // формируем пакет - дата
                btdef::date time(queue.gettimeofday_cached());
                auto packet = time.json_text();

                cout() << "sendto: "sv << dest << ' '
                    << '\'' << packet << '\'' << std::endl;

                // отправляем пакет
                auto res = sock.sendto(dest, packet.data(), packet.size());
                if (btpro::code::fail == res)
                    throw std::system_error(btpro::net::error_code(), "sendto");
            }
            catch (const std::exception& e)
            {
                cerr() << e.what() << std::endl;
            }
        };

        // создаем таймер рассылки
        btpro::evs::socket ev(queue, socket, EV_TIMEOUT|EV_PERSIST, std::move(fn));

        // таймер отправки заводим на три секунды
        ev.add(std::chrono::milliseconds(100));
        // сразу же делаем рассылку
        ev.active(EV_TIMEOUT);

        auto f = [&] {
            cout() << "stop!"sv << std::endl;
            queue.loop_break();
        };

        btpro::evs::type sint(queue, SIGINT, EV_SIGNAL|EV_PERSIST, f);
        sint.add();

        btpro::evs::type sterm(queue, SIGTERM, EV_SIGNAL|EV_PERSIST, f);
        sterm.add();

        queue.dispatch();
    }
    catch (const std::exception& e)
    {
        cerr() << e.what() << std::endl;
    }

    return 0;
}
