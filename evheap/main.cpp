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
    btpro::startup();

    btpro::queue q;
    
    btpro::evcore<btpro::evheap> evh;
    evh.create(q, EV_TIMEOUT, call, nullptr);

    btpro::evcore<btpro::evstack> evs;
    auto l = [&](...){
        std::cout << "+lambda!" << std::endl;

        evh.destroy();
        evs.destroy();

        q.loop_break();
    };
    evs.create(q, EV_TIMEOUT, l);

    evh.add(std::chrono::milliseconds(250));
    evs.add(std::chrono::milliseconds(550));

    q.dispatch();

    return 0;
}
