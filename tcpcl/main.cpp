#include "btpro/config.hpp"
#include "btpro/socket.hpp"

#include <vector>
#include <array>
#include <list>

#include <iostream>

int main(int argc, char* argv[])
{
//    try
//    {
//        // инициализация wsa
//        btpro::startup();

//        btpro::config c;
//#ifndef _WIN32
//        c.require_features(EV_FEATURE_ET);
//#endif //
//        btpro::queue q(c);
//        std::cout << btpro::queue::version() << ' ' << q.method() << std::endl;

//        // сокеты для мультикаста портируемо биндятся на any
//        auto sa = btpro::ipv4::any(4587);
//        if (argc > 1)
//        {
//            // читаем порт из параметров
//            sa = btpro::ipv4::any(std::atoi(argv[1]));
//        }

//        btpro::socket socket;
//        btpro::socket bevsock;
//        // создаем сокет с возможностью повторного использования адреса
//        socket.create(sa, btpro::sock_dgram, btpro::reuse_addr::on());
//        bevsock.create(sa, btpro::sock_dgram, btpro::reuse_addr::on());

//        if (argc > 2)
//        {
//            using btpro::ipv4::addr;
//            using btpro::ipv4::multicast_group::join;

//            // парсим адрес группы из параметров
//            addr group_addr(argv[2]);
//            // вступаем в указанную группу с фильтрами источников
//            if (argc > 3)
//            {
//                // формируем фильтры источников рассылки
//                for (int i = 3; i < argc; ++i)
//                {
//                    addr src_addr(argv[i]);
//                    socket.set(join(group_addr, src_addr));
//                    bevsock.set(join(group_addr, src_addr));
//                }
//            }
//            else
//            {
//                // вступаем в группу рассылки переданую в параметрах
//                socket.set(join(group_addr));
//                bevsock.set(join(group_addr));
//            }
//        }
//        else
//        {
//            using btpro::ipv4::addr;
//            using btpro::ipv4::multicast_group::join;

//            // по умолчанию
//            // вступаем в группу без фильтрации
//            socket.set(join(addr("224.0.0.42")));
//            bevsock.set(join(addr("224.0.0.42")));
//        }

//        struct reader
//        {
//            utility::date time_{};

//            reader(timeval tv)
//                : time_(tv)
//            {   }

//            void push(const btpro::sock_addr& dest,
//                const char *buf, std::size_t len)
//            {
//                std::cout << time_.json_text()
//                    << " recv: '" << ref::string(buf, len)
//                    << "' from: " << dest << std::endl;
//            }
//        };

//        // создаем собыите приема данных
//        btpro::evfun ev;
//        btpro::udp::basic_recvfrom<1024, 16> recv_from;
//        ev.assign(q, socket.fd(), EV_READ|EV_PERSIST|EV_TIMEOUT|EV_ET,
//            [&](ev_socklen_t fd, short ef) {
//                // получаем время для лога
//                utility::date time(q.gettimeofday_cached());
//                // если произошел таймаут чтения генерируем ошибку
//                if (ef & EV_TIMEOUT)
//                {
//                    std::cout << time.json_text() << " recv: timeout" << std::endl;
//                    throw std::runtime_error("recv timeout");
//                }

//                reader r(q.gettimeofday_cached());
//                auto count = recv_from(fd, r);
//                if (count)
//                {
//                    // активируем событие повторно
//                    // это приведет к повторному чтению сокета на следующем цикле очереди
//                    // без выполнения epoll_wait, poll, select и т.п.
//                    // указываем фалг, что это не был таймаут
//                    ev.next(EV_READ|EV_ACTIVATE);
//                }
//        });

//        // стартуем ожидание события приема данных
//        ev.add(std::chrono::seconds(30));
//        // сразу активируем эвент чтобы сделать попытку чтения в первый раз
//        ev.active(EV_READ|EV_ACTIVATE);

//        //btpro::udp::pev udpbev(q, bevsock.fd());

//        q.dispatch();

//        socket.close();
//    }
//    catch (const std::exception& e)
//    {
//        std::cerr << e.what() << std::endl;
//    }

    return 0;
}
