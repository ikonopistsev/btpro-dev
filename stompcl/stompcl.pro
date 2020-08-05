TEMPLATE = app

CONFIG -= qt
CONFIG -= app_bundle
CONFIG += console c++17 warn_on

TARGET = stompcl

CONFIG(release, debug|release) {
    DEFINES += NDEBUG
}

INCLUDEPATH += \
    ../stomptalk/include \
    ../stompconn/include \
    ../

LIBS += \
    -L../stomptalk -lstomptalk \
    -L../stompconn -lstompconn

unix:!macx {
    CONFIG += link_pkgconfig
    PKGCONFIG += libevent # libevent_openssl libssl libcrypto
    CONFIG(release, debug|release) {
        QMAKE_POST_LINK=$(STRIP) $(TARGET)
    }
}

macx {
    INCLUDEPATH += /usr/local/include
    LIBS += -L/usr/local/lib -levent
}

SOURCES += \
    main.cpp
