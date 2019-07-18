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

btpro::queue create_queue()
{
    btpro::config conf;

    std::cout << "libevent-" << btpro::queue::version() << ' ' << '-' << ' ';
    for (auto& i : conf.supported_methods())
        std::cout << i << ' ';
    std::endl(std::cout);

#ifndef _WIN32
    conf.set_flag(EVENT_BASE_FLAG_EPOLL_USE_CHANGELIST);
    conf.require_features(EV_FEATURE_ET | EV_FEATURE_O1 | EV_FEATURE_EARLY_CLOSE);
#endif //
    return btpro::queue(conf);
}

int main(int argc, char* argv[])
{
    try
    {
        // инициализация wsa
        btpro::startup();

        auto queue = create_queue();
        std::cout << "use: " << queue.method() << std::endl << std::endl;

        std::cout << "mccl test" << std::endl;

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
            utility::date time(queue.gettimeofday_cached());

            try
            {
                btpro::socket sock(fd);

                // если произошел таймаут чтения генерируем ошибку
                if (ef & EV_TIMEOUT)
                {
                    static const auto recv_timeout = std::mkstr(std::cref("recv: timeout"));
                    std::cout << time.json_text() << ' ' << recv_timeout << std::endl;
                    throw std::runtime_error("recv timeout");
                }

                if (ef & EV_READ)
                {
                    char buf[0x10000];
                    btpro::sock_addr dest;
                    // пытаемся читать
                    auto res = sock.recvfrom(dest, buf, sizeof(buf));
                    if (btpro::code::fail == res)
                    {
                        // UDP может быть заблочен только если нет данных
                        // те если уже вычитали все
                        // тогда просто выходим
                        if (sock.wouldblock())
                            return 0;
                        else
                        {
                            // если произошла иная ошибка
                            auto ec = btpro::net::error_code();
                            throw std::system_error(ec, "recvfrom");
                        }
                    }

                    static const auto recv = std::mkstr(std::ref("recv:"));
                    static const auto from = std::mkstr(std::ref("from:"));
                    static const auto rx = std::mkstr(std::ref("rx:"));

                    count += static_cast<std::size_t>(res);
                    std::cout << time.json_text() << ' ' << recv << ' ' << res
                            << ' ' << from << ' ' << dest << ' ' << rx << ' ' << count << std::endl;

                    // возвращаем событие повторной читки
                    // без ожидания готовности сокета
                    // а в ней сделаем проверку на wouldblock
                    return 0;
                }

                return 0;
            }
            catch (const std::exception& e)
            {
                std::cerr << e.what() << std::endl;
            }

            return 0;
        };

        // создаем собыите приема данных
        btpro::evcore<btpro::evstack> ev;
        ev.create(queue, socket, EV_READ|EV_PERSIST|EV_TIMEOUT|EV_ET, fn);

        // стартуем ожидание события приема данных
        ev.add(std::chrono::seconds(30));
        // сразу активируем эвент чтобы сделать попытку чтения в первый раз
        ev.active(EV_READ);

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