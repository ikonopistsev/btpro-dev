#include "btpro/evcore.hpp"
#include "btpro/evstack.hpp"
#include "btpro/socket.hpp"
#include "btpro/ipv4/multicast_group.hpp"
#include "btpro/ipv4/multicast_source_group.hpp"
#include "btdef/date.hpp"
#include "btdef/string.hpp"

#include <vector>
#include <array>
#include <list>

#include <iostream>

#ifndef _WIN32
#include <signal.h>
#endif // _WIN32

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

// - using mccl [[[PORT] HOST] MULTICAST_SRC_GROUP...]

int main(int argc, char* argv[])
{
    try
    {
        // инициализация wsa
        btpro::startup();

        MKREFSTR(use_str, "use: ");
        MKREFSTR(mccl_test_str, "mccl test");

        auto queue = create_queue();
        cout() << use_str << queue.method() << std::endl << std::endl;
        cout() << mccl_test_str << std::endl;

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
        auto fn = [&] (evutil_socket_t fd, btpro::event_flag_t ef) {
            // получаем время для лога
            btdef::date time(queue.gettimeofday_cached());

            try
            {
                btpro::socket sock(fd);

                // если произошел таймаут чтения генерируем ошибку
                if (ef & EV_TIMEOUT)
                {
                    MKREFSTR(timeout_str, "timeout");
                    cout() << timeout_str << std::endl;
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

                    MKREFSTR(rx_str, "rx:");
                    MKREFSTR(recv_str, "recv:");
                    MKREFSTR(from_str, "from:");

                    cout() << recv_str << ' ' << res << ' ' << from_str << ' ' 
                        << dest << ' ' << rx_str << ' ' << count << std::endl;

                    // возвращаем событие повторной читки
                    // без ожидания готовности сокета
                    // а в ней сделаем проверку на wouldblock
                    return 0;
                }

                return 0;
            }
            catch (const std::exception& e)
            {
                cerr() << e.what() << std::endl;
            }

            return 0;
        };

        // создаем собыите приема данных
        btpro::evcore<btpro::evstack> ev;
        ev.create(queue, socket, EV_READ|EV_PERSIST|EV_TIMEOUT|EV_ET, fn);

        // стартуем ожидание события приема данных
        ev.add(std::chrono::seconds(20));

#ifndef WIN32
        be::evcore<be::evstack> sint;
        be::evcore<be::evstack> sterm;

        auto f = [&](auto...) {
            MKREFSTR(stop_str, "stop!");
            cerr() << stop_str << std::endl;
            queue.loop_break();
            return 0;
        };

        sint.create(queue, SIGINT, EV_SIGNAL|EV_PERSIST, f);
        sint.add();

        sterm.create(queue, SIGTERM, EV_SIGNAL|EV_PERSIST, f);
        sterm.add();
#endif // _WIN32

        queue.dispatch();

        // сокет сам не закрывается
        socket.close();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
