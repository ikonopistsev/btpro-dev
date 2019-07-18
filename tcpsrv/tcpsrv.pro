TEMPLATE = app

CONFIG -= qt app_bundle
CONFIG += console c++14 warn_on

TARGET = tcpsrv

CONFIG(release, debug|release) {
    DEFINES += NDEBUG
}

INCLUDEPATH += \
    ../

unix:!macx {
    CONFIG += link_pkgconfig
    PKGCONFIG += libevent
}

macx {
    INCLUDEPATH += /usr/local/include
    LIBS += -L/usr/local/Cellar/libevent/2.1.8/lib -levent -levent_pthreads
}

SOURCES += \
    main.cpp
