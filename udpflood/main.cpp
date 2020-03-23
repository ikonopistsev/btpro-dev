#include "btpro/evh.hpp"
#include "btpro/socket.hpp"
#include <vector>
#include <array>
#include <list>
#include <mutex>
#include <thread>

#include <iostream>

int main(int argc, char* argv[])
{
    std::size_t id = 1; //std::atoi(argv[1]);
    std::size_t psize = 0;

    be::sock_addr dest;
    dest = be::sock_addr::loopback(3345);

    if (argc > 3)
    {
        id = std::atoi(argv[1]);
        dest.assign(argv[2], std::atoi(argv[3]));
    }

    if (argc > 4)
        psize = std::atoi(argv[4]);

    be::socket socket;
    socket.create(dest.family(), SOCK_DGRAM);

    std::size_t tx = 0;
    std::size_t txf = 0;
    std::size_t txmln = 0;
    std::size_t txcount = 0;
    std::size_t count = 0;
    std::size_t iops = 0;

    be::config conf;
    conf.require_features(EV_FEATURE_ET);

    be::queue q(conf);

    char b[65000];

    utility::date st;

    be::evh evh(q, socket.fd(), EV_PERSIST|EV_TIMEOUT|EV_ET,
        [&](evutil_socket_t fd, short ef) {

        if (ef & EV_TIMEOUT)
            throw std::runtime_error("send timeout");

        if (ef & EV_WRITE)
        {
            be::socket sock(fd);
            utility::text msg;
            do
            {
                msg = utility::to_text(id);
                msg += ' ';
                msg += utility::to_text(txcount);
                msg += ' ';
                msg += utility::date::now().json_text();

                auto msg_size = msg.size();
                memcpy(b, msg.data(), msg_size);

                if (psize < msg_size)
                    psize = msg_size;

                ++iops;
                auto sz = socket.sendto(dest, b, psize);
                if (sz == -1)
                {
                    ++count;
                    break;
                }

                if (psize != static_cast<std::size_t>(sz))
                    throw std::runtime_error("send error");

                txf += sz;
                tx += sz;

                static const auto v = 1000000u;
                if (++txcount == v)
                {
                    double mb = tx / (1024.0 * 1024.0);
                    auto et = utility::date::now();
                    auto d = (et - st) / 1000.0;

                    utility::text s(std::cref("p: "));
                    s += utility::to_text(++txmln);
                    s += std::cref(" m, tx: ");
                    s += utility::to_text(txf);
                    s += std::cref(", f: ");
                    s += utility::to_text(mb / d, 2);
                    s += std::cref(" Mb/s, i: ");
                    s += utility::to_text(iops / d, 2);
                    s += std::cref(" iops c: ");
                    s += utility::to_text(count);
                    s += std::cref(", b: ");
                    s += utility::to_text(count / d, 2);
                    s += std::cref(" c/s");
                    std::cout << s << std::endl;

                    txcount -= v;
                    iops = 0;
                    tx = 0;
                    st = et;
                }


            } while (1);
        }
    });

    evh.active(EV_WRITE);

    st = utility::date::now();
    q.dispatch();

    return 0;
}
