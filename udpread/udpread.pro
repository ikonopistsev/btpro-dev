TEMPLATE = app
QT -= core gui

TARGET = udpread

CONFIG += c++11 console warn_on

CONFIG(release, debug|release) {
    DEFINES += NDEBUG
}

CONFIG(debug, debug|release) {
    DEFINES += BTDEF_TRACE_WRAPPER BTDEF_TRACE_CRT_MALLOC
}

INCLUDEPATH += ../btdef

unix:!macx {
    CONFIG += link_pkgconfig
    PKGCONFIG += libevent
}

macx {
    CONFIG -= app_bundle
    INCLUDEPATH += /usr/local/include
    LIBS += -L/usr/local/Cellar/libevent/2.1.8/lib -levent -levent_pthreads
}

SOURCES += \
    examples/udpread/main.cpp

HEADERS += \
    btpro/tcp/listener.hpp \
    btpro/btpro.hpp \
    btpro/buffer.hpp \
    btpro/config.hpp \
    btpro/queue.hpp \
    btpro/sock_addr.hpp \
    btpro/tcp/acceptor.hpp \
    btpro/tcp/queue.hpp \
    btpro/tcp/server.hpp \
    btpro/tcp/tcp.hpp \
    btpro/ev.hpp \
    btpro/storev.hpp \
    btpro/reactor.hpp \
    btpro/evh.hpp \
    btpro/socket.hpp \
    btpro/tcp/bev.hpp
