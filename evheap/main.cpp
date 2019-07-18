#include "btpro/evheap.hpp"
#include "btpro/evstack.hpp"
#include "btpro/evcore.hpp"
#include "btdef/date.hpp"

#include <iostream>
#include <signal.h>

void call(evutil_socket_t, short, void *)
{
    std::cout << "method!" << std::endl;
}

int main(int, char**)
{
    btpro::socket s;
    s.create(btpro::ipv6::addr("[::1]:45678"), btpro::sock_stream,
        btpro::reuse_addr::on(), btpro::reuse_port::on());
    //s.listen(1024);

    static constexpr auto d = double { 123.0 };
    std::cout << d << std::endl;

    static constexpr auto date =  utility::date { };
    std::cout << date << std::endl;

    btpro::queue q;
    btpro::evcore<btpro::evheap> evh;
    btpro::evcore<btpro::evstack> evs;

    evh.create(q, EV_TIMEOUT, call, nullptr);

    auto l = [&](...){
        std::cout << " lambda!" << std::endl;
        q.loop_break();
    };
    evs.create(q, SIGINT, EV_SIGNAL, l);

    evh.add(std::chrono::milliseconds(250));
    evs.add();

    q.dispatch();

    evh.destroy();
    evs.destroy();

    s.close();

    return 0;
}
