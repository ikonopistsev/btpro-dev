TEMPLATE = app

CONFIG -= qt
CONFIG -= app_bundle
CONFIG += console c++17 warn_on

TARGET = evheap

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
    CONFIG -= app_bundle
    INCLUDEPATH += /usr/local/include
    LIBS += -L/usr/local/Cellar/libevent/2.1.8/lib -levent -levent_pthreads
}

SOURCES += \
    main.cpp
