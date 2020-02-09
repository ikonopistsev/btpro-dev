#include "btpro/evcore.hpp"
#include "btpro/evstack.hpp"
#include "btpro/ssl/connector.hpp"
#include "btpro/uri.hpp"
#include "btdef/date.hpp"

#include <iostream>
#include <list>
#include <string_view>

#ifndef _WIN32
#include <signal.h>
#endif // _WIN32

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

template <class F>
inline std::ostream& cout(F fn)
{
    auto text = fn();
    return std::endl(output(std::cout)
        .write(text.data(), static_cast<std::streamsize>(text.size())));
}

template <class F>
inline std::ostream& cerr(F fn)
{
    auto text = fn();
    return std::endl(output(std::cerr)
        .write(text.data(), static_cast<std::streamsize>(text.size())));
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
    btpro::queue queue;
    queue.create(conf);
    return queue;
}

int main()
{
    try
    {
        // инициализация wsa
        btpro::startup();
        auto queue = create_queue();
        btpro::dns dns;
        dns.create(queue, btpro::dns_initialize_nameservers);
        auto ssl = btpro::ssl::client();

        btpro::ssl::connector<BUFFEREVENT_SSL_CONNECTING,
                BEV_OPT_CLOSE_ON_FREE> conn(ssl, queue, dns);

        auto bev = conn.create();
        btpro::sock_addr addr = btpro::sock_addr("localhost:22");
        bev.connect(addr);

        btpro::uri<btpro::detail::data_ptr<btpro::detail::uri_data<std::string>>> uri;
        uri.set_scheme("http");
        uri.set_host(std::string("localhost"));

        btpro::uri<btpro::detail::uri_data<std::string>> text_uri, text_uri1;
        text_uri.set_host("localhost");

        cout() << "sizeof ptr_uri: " << sizeof(uri) << std::endl;
        cout() << "sizeof text_uri: " << sizeof(text_uri) << std::endl;

        text_uri = uri;
        text_uri1 = text_uri;

        text_uri1.set_user(btref::mkstr(std::cref("user")));


#ifndef WIN32
        auto f = [&](auto...) {
            MKREFSTR(stop_str, "stop!");
            cerr() << stop_str << std::endl;
            queue.loop_break();
            return 0;
        };

        be::evcore<be::evstack> sint;
        sint.create(queue, SIGINT, EV_SIGNAL|EV_PERSIST, f);
        sint.add();

        be::evcore<be::evstack> sterm;
        sterm.create(queue, SIGTERM, EV_SIGNAL|EV_PERSIST, f);
        sterm.add();
#endif // _WIN32

        queue.dispatch();
    }
    catch (const std::exception& e)
    {
        cerr() << e.what() << std::endl;
    }

    return 0;
}
