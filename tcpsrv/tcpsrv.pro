TEMPLATE = app

CONFIG -= qt
CONFIG -= app_bundle
CONFIG += console c++17 warn_on

TARGET = tcpsrv

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
    LIBS += -lrt
}

macx {
    INCLUDEPATH += /usr/local/include
    LIBS += -L/usr/local/lib -levent
}

SOURCES += \
    main.cpp
