#include "btpro/evh.hpp"
#include "btpro/socket.hpp"

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include <array>
#include <list>

#include <iostream>

//struct iovec {                    /* массив элементов приёма/передачи */
//   void  *iov_base;              /* начальный адрес */
//   size_t iov_len;               /* количество передаваемых байт */
//};
//
//struct msghdr {
//   void         *msg_name;       /* необязательный адрес */
//   socklen_t     msg_namelen;    /* размер адреса */
//   struct iovec *msg_iov;        /* массив приёма/передачи */
//   size_t        msg_iovlen;     /* количество элементов в msg_iov */
//   void         *msg_control;    /* вспомогательные данные,
//                                    см. ниже */
//   size_t        msg_controllen; /* размер буфера вспомогательных
//                                    данных */
//   int           msg_flags;      /* флаги принятого сообщения */
//};

//struct mmsghdr {
//         struct msghdr msg_hdr;  /* Заголовок сообщения */
//         unsigned int  msg_len;  /* Количество полученных байт
//                                    для заголовка */
//};

std::size_t rxmln{};
std::atomic<std::size_t> rx{};
std::atomic<std::size_t> rxfull{};
std::atomic<std::size_t> rxcount{};
std::atomic<std::size_t> iops{};
std::atomic<std::size_t> iocount{};

class rxbuf_ref
{
    iovec iovec_{};

public:
    rxbuf_ref() = default;

    rxbuf_ref(void *ptr, std::size_t size) noexcept
        : iovec_(iovec{ptr, size})
    {    }

    char* begin() noexcept
    {
        return reinterpret_cast<char*>(iovec_.iov_base);
    }

    char* end() noexcept
    {
        return begin() + size();
    }

    const char* begin() const noexcept
    {
        return reinterpret_cast<const char*>(iovec_.iov_base);
    }

    const char* end() const noexcept
    {
        return begin() + size();
    }

    std::size_t size() const noexcept
    {
        return iovec_.iov_len;
    }

    bool empty() const noexcept
    {
        return size() == 0;
    }
};

template<int chunk_size, int vlen>
class recv_mmsg
{
    class mmsg
    {
        mmsghdr mmsg_[vlen];
        iovec iovec_[vlen];
        char buff_[vlen][chunk_size];

    public:
        mmsg()
        {
            for (std::size_t i = 0; i < vlen; ++i)
            {
                iovec_[i].iov_base = buff_[i];
                iovec_[i].iov_len = chunk_size;
                mmsg_[i].msg_hdr.msg_iov = &iovec_[i];
                mmsg_[i].msg_hdr.msg_iovlen = 1;
            }
        }

        mmsghdr* data() noexcept
        {
            return mmsg_;
        }

        rxbuf_ref rxbuf(std::size_t i) noexcept
        {
            return rxbuf_ref(iovec_[i].iov_base, mmsg_[i].msg_len);
        }
    };

    std::unique_ptr<mmsg> data_{ new mmsg() };

public:
    recv_mmsg() = default;

public:

    std::size_t receive(be::socket sock)
    {
        int res = 0;
        res = ::recvmmsg(sock.fd(), data_->data(), vlen, 0, nullptr);
        if (res == -1)
        {
            if ((errno == EAGAIN) ||(errno == EWOULDBLOCK))
            {
                size_ = 0;
                break;
            }

            auto ec = evutil_socket_geterror(sock.fd());
            auto text = evutil_socket_error_to_string(ec);
            throw std::runtime_error(text);
        }

        return static_cast<std::size_t>(res);
    }

    bool empty() const noexcept
    {
        return size() == 0;
    }

    rxbuf_ref operator[](std::size_t i) const noexcept
    {
        return data_->rxbuf(i);
    }
};

auto sa = be::sock_addr::loopback(3345);

std::size_t thread_count = 1;
std::size_t socket_count = 1;

void run()
{
    be::config conf;
    conf.require_features(EV_FEATURE_ET);
    be::queue q(conf);
    utility::date st;

    recv_mmsg<65000, 16> rbuf;

    auto f = [&](evutil_socket_t fd, short ef) {
        if (ef & EV_TIMEOUT)
            throw std::runtime_error("recv timeout");

        if (ef & EV_READ)
        {            
            if (!st.time())
                st = utility::date(q.gettimeofday_cached());

            auto sz = buf.receive(be::socket(fd));

            bool again = false;
            do
            {
                ++iops;
                again = rbuf.receive(be::socket(fd));

                auto count = rbuf.size();
                iocount += count;

                for (std::size_t i = 0; i < count; ++i)
                {
                    rxsize += rbuf[i].size();
                    static const auto v = 1000000u;
                    if (++rxcount == v)
                    {
                        ++rxmln;

                        auto size = rxsize.load();
                        double mb = size / (1024.0 * 1024.0);
                        auto et = utility::date::now();
                        auto d = (et - st) / 1000.0;

                        utility::text s(std::cref("p: "));
                        s += utility::to_text(rxmln);
                        s += std::cref(" m, rx: ");
                        s += utility::to_text(size);
                        s += std::cref(", f: ");
                        s += utility::to_text(mb / d, 2);
                        s += std::cref(" Mb/s, i: ");
                        s += utility::to_text(iops / d, 2);
                        s += std::cref(" iops, a:");
                        s += utility::to_text(iocount / (double)iops.load(), 2);
                        std::cout << s << std::endl;

                        rxcount -= v;

                        st = et;
                    }
                }
            } while (again);
        }
    };

    std::vector<be::evh> evarr;
    for (std::size_t i = 0; i < socket_count; ++i)
    {
        be::socket socket;
        socket.create(sa, SOCK_DGRAM, LEV_OPT_REUSEABLE_PORT);

        evarr.emplace_back(std::ref(q), socket.fd(),
                           EV_PERSIST|EV_READ|EV_ET|EV_TIMEOUT, std::ref(f));
        evarr.back().add(std::chrono::seconds(30));
    }

    q.dispatch();
}

int main(int argc, char* argv[])
{
    be::startup();

    if (argc > 2)
        sa.assign(argv[1], std::atoi(argv[2]));

    if (argc > 3)
    {
        thread_count = std::atoi(argv[3]);
        if (thread_count < 1)
            thread_count = 1;
    }

    if (argc > 4)
    {
        socket_count = std::atoi(argv[4]);
        if (socket_count < 1)
            socket_count = 1;
    }

    std::vector<std::thread> thr;
    for (std::size_t i = 0; i < thread_count; ++i)
        thr.emplace_back(&run);

    std::this_thread::sleep_for(std::chrono::hours(1));
    return 0;
}
