#include "btpro/ev.hpp"
#include "btpro/queue.hpp"
#include "btpro/socket.hpp"
#include "btpro/ipv4/multicast_group.hpp"
#include "btpro/ipv4/multicast_source_group.hpp"
#include "btdef/date.hpp"
#include <string_view>

#include <vector>
#include <array>
#include <list>

#include <iostream>
#include <signal.h>

using namespace std::literals;

// - multicast client example

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
    cout() << "libevent-"sv << btpro::queue::version() << " - "sv;

    btpro::config conf;
    for (auto& i : conf.supported_methods())
        std::cout << i << ' ';
    std::endl(std::cout);

#ifndef _WIN32
    conf.require_features(EV_FEATURE_ET|EV_FEATURE_O1);
#endif //
    return btpro::queue(conf);
}

// - using mccl [[[PORT] HOST] MULTICAST_SRC_GROUP...]

int main(int argc, char* argv[])
{
    try
    {
        // инициализация wsa
        btpro::startup();

        auto queue = create_queue();
        cout() << "use: "sv << queue.method() << std::endl << std::endl;
        cout() << "mccl test"sv << std::endl;

        // сокеты для мультикаста портируемо биндятся на any
        auto sa = btpro::ipv4::any(4587);
        if (argc > 1)  
        {
            // читаем порт из параметров
            sa = btpro::ipv4::any(std::atoi(argv[1]));
        }

        btpro::socket socket;
        // создаем сокет с возможностью повторного использования адреса
        socket.create(sa, btpro::sock_dgram, btpro::reuse_addr::on());
        if (argc > 2)
        {
            using btpro::ipv4::addr;
            using btpro::ipv4::multicast_group::join;

            // парсим адрес группы из параметров
            addr group_addr(argv[2]);
            // вступаем в указанную группу с фильтрами источников
            if (argc > 3)
            {
                // формируем фильтры источников рассылки
                for (int i = 3; i < argc; ++i)
                {
                    addr src_addr(argv[i]);
                    socket.set(join(group_addr, src_addr));
                }
            }
            else
            {
                // вступаем в группу рассылки переданую в параметрах
                socket.set(join(group_addr));
            }
        }
        else
        {
            using btpro::ipv4::addr;
            using btpro::ipv4::multicast_group::join;

            // по умолчанию
            // вступаем в группу без фильтрации
            socket.set(join(addr("224.0.0.42")));
        }

        std::size_t count = 0;
        auto fn = [&] (btpro::socket sock, btpro::event_flag ef) {
            // получаем время для лога
            btdef::date time(queue.gettimeofday_cached());

            try
            {
                // если произошел таймаут чтения генерируем ошибку
                if (ef & EV_TIMEOUT)
                {
                    cout() << "timeout"sv << std::endl;
                }

                if (ef & EV_READ)
                {
                    char buf[0x10000];
                    btpro::sock_addr dest;
                    // пытаемся читать
                    auto res = sock.recvfrom(dest, buf, sizeof(buf));
                    if (btpro::code::fail == res)
                    {
                        // если произошла иная ошибка
                        auto ec = btpro::net::error_code();
                        throw std::system_error(ec, "recvfrom");
                    }

                    count += static_cast<std::size_t>(res);

                    cout() << "recv:"sv << ' ' << res << ' ' << "from:"sv << ' '
                        << dest << ' ' << "rx:"sv << ' ' << count << std::endl;
                }
            }
            catch (const std::exception& e)
            {
                cerr() << e.what() << std::endl;
            }
        };

        // создаем собыите приема данных
        btpro::evs::socket ev(queue, socket, 
            EV_READ|EV_PERSIST|EV_TIMEOUT|EV_ET, 
            std::chrono::seconds(20), std::move(fn));

        btpro::evs::type sint;
        btpro::evs::type sterm;

        auto f = [&] {
            cout() << "stop!"sv << std::endl;
            queue.loop_break();
        };

        sint.create(queue, SIGINT, EV_SIGNAL|EV_PERSIST, f);
        sint.add();

        sterm.create(queue, SIGTERM, EV_SIGNAL|EV_PERSIST, f);
        sterm.add();

        queue.dispatch();

        // сокет сам не закрывается
        socket.close();
    }
    catch (const std::exception& e)
    {
        cerr() << e.what() << std::endl;
    }

    return 0;
}
