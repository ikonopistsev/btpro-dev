#include "btpro/evcore.hpp"
#include "btpro/evstack.hpp"
#include "btpro/ssl/bevsock.hpp"
#include "btpro/uri.hpp"
#include "btdef/date.hpp"
#include "btpro/wslay/context.hpp"
#include "btpro/ssl/base64.hpp"
#include "btpro/curl/client.hpp"

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

using namespace std::literals;

#define MKREFSTR(x, y) \
    static const auto x = y##sv

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

void connect(btpro::curl::client& cl, btpro::queue_ref)
{
    constexpr auto url =
        "https://wt.roboforex.com/demopro/status?a=9741243f-85ae-11eb-ad92-3417ebee2def";
//    constexpr auto proto =
//        "SnapshotFullRefresh2";

    cl.get([&](auto req){

        req.push("pider-header-x", "ti-pidr");

        req.set(std::string(url), [](btpro::curl::resp_ext res){
            std::endl(std::cout);
            std::cout << "http-code: "sv << btpro::curl::http_code(res) << std::endl;
            std::cout << "data-size: "sv << res.buffer().size() << std::endl;
            std::cout << "content-type: "sv << res.get("content-type") << std::endl;
        });
    });


    cl.get(url, [](btpro::curl::resp_ext res){
        std::endl(std::cout);
        std::cout << "http-code: "sv << btpro::curl::http_code(res) << std::endl;
        std::cout << "data-size: "sv << res.buffer().size() << std::endl;
        std::cout << "content-type: "sv << res.get("content-type") << std::endl;
    });
}

int main()
{
    try
    {
        btpro::ssl::rand::poll();

        // инициализация wsa
        btpro::startup();
        btpro::curl::startup();

        auto queue = create_queue();

        btpro::dns dns;
        dns.create(queue, btpro::dns_initialize_nameservers);

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


        btpro::curl::client cl(queue);

        connect(cl, queue);

        queue.dispatch();
    }
    catch (const std::exception& e)
    {
        cerr() << e.what() << std::endl;
    }

    return 0;
}
