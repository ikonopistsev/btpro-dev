#include "btpro/config.hpp"
#include "btpro/socket.hpp"
#include "btpro/evstack.hpp"
#include "btpro/evheap.hpp"
#include "btpro/evcore.hpp"
#include "btpro/tcp/acceptorfn.hpp"
#include "btpro/tcp/bev.hpp"
#include "btdef/date.hpp"
#include "btpro/tcp/bev.hpp"    
#include <string_view>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h> /* константы для mode */
#include <fcntl.h>
#include <iostream>
#include <list>

#ifndef _WIN32
#include <signal.h>
#endif // _WIN32

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

class server
{
    btpro::queue queue_;
    btpro::dns dns_;
    btpro::tcp::acceptorfn<server> acceptor4_{ *this, &server::accept,
                                             &server::on_throw };
    int memfd_{};

    void accept(be::socket sock, be::ip::addr addr)
    {
        be::sock_addr sa(addr);

        cout() << "connect from:"sv << ' ' << sa << ' '
               << "family:"sv << ' ' << addr.family() << std::endl;

        sock.set(btpro::zerocopy::on());

        char buf[4096];
        auto rc = recv(sock.fd(), buf, sizeof(buf), 0);
        if (btpro::code::fail == rc)
        {
            sock.close();
            throw std::system_error(btpro::net::error_code(), "::recv");
        }

        rc = send(sock.fd(), buf, rc, MSG_ZEROCOPY);
        if (btpro::code::fail == rc)
        {
            sock.close();
            throw std::system_error(btpro::net::error_code(), "::send");
        }

        sock.close();


//        int pfd[2];
//        pipe(pfd);

//#define SPLICE_MAX (1024 * 1024)
//        fcntl(pfd[0], F_SETPIPE_SZ, SPLICE_MAX);
//        fcntl(pfd[1], F_SETPIPE_SZ, SPLICE_MAX);

//        auto n = splice(sock.fd(), nullptr, pfd[1], nullptr, SPLICE_MAX,
//                           SPLICE_F_MOVE);

//        splice(pfd[0], nullptr, sock.fd(), nullptr, n, SPLICE_F_MOVE);

//        auto rc = copy_file_range(sock.fd(),
//            nullptr, sock.fd(), nullptr, 4096, 0);
//        if (btpro::code::fail == rc)
//        {
//            sock.close();
//            throw std::system_error(btpro::net::error_code(), "::copy_file_range");
//        }

//        auto rc = sendfile(memfd_, sock.fd(), 0, 4096);
//        if (btpro::code::fail == rc)
//        {
//            sock.close();
//            throw std::system_error(btpro::net::error_code(), "::sendfile");
//        }

//        auto rc = splice(sock.fd(), nullptr, memfd_, 0, 4096, SPLICE_F_NONBLOCK);
//        if (btpro::code::fail == rc)
//            throw std::system_error(btpro::net::error_code(), "::splice");
    }

    void on_throw(std::exception_ptr eptr)
    {
        try {
            std::rethrow_exception(eptr);
        } catch (const std::runtime_error& e) {
            cerr() << e.what() << std::endl;
        }
    }

    void create()
    {
        cout() << "libevent-"sv << btpro::queue::version() << ' ' << '-' << ' ';

        btpro::config conf;
        for (auto& i : conf.supported_methods())
            std::cout << i << ' ';
        std::endl(std::cout);

#ifndef _WIN32
        conf.require_features(EV_FEATURE_ET|EV_FEATURE_O1);
#endif //
        queue_.create(conf);
        dns_.create(queue_, btpro::dns_initialize_nameservers);
    }

public:
    server(int argc, char* argv[])
    {
        create();

        btpro::sock_addr addr(btpro::ipv4::any(14889));
        if (argc > 1)
            addr.assign(argv[1]);

        acceptor4_.listen(queue_, LEV_OPT_REUSEABLE_PORT, addr);

        auto rc = shm_open("/server1", O_CREAT|O_RDWR, S_IRWXU);
        if (btpro::code::fail == rc)
            throw std::system_error(btpro::net::error_code(), "::shm_open");

        memfd_ = rc;
    }

    void run()
    {
#ifndef WIN32
        auto f = [&](auto...) {
            cerr() << "stop!"sv << std::endl;
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

    try {
        server srv(argc, argv);
        srv.run();
    }  catch (const std::exception& e) {
        cerr() << e.what() << std::endl;
    }

    return 0;
}
