TEMPLATE = app

CONFIG -= qt
CONFIG -= app_bundle
CONFIG += console c++14 warn_on

TARGET = mcsrv

CONFIG(release, debug|release) {
    DEFINES += NDEBUG
}

INCLUDEPATH += \
    ../

unix:!macx {
    CONFIG += link_pkgconfig
    PKGCONFIG += libevent
    CONFIG(release, debug|release) {
        QMAKE_POST_LINK=$(STRIP) $(TARGET)
    }
}

macx {
    INCLUDEPATH += /usr/local/include
    LIBS += -L/usr/local/Cellar/libevent/2.1.8/lib -levent -levent_pthreads
}

SOURCES += \
    main.cpp
