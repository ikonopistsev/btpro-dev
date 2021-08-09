TEMPLATE = app

CONFIG -= qt
CONFIG -= app_bundle
CONFIG += console c++17 warn_on

TARGET = tcpcl

CONFIG(release, debug|release) {
    DEFINES += NDEBUG
}

INCLUDEPATH += \
    /usr/local/include \
    ../

unix:!macx {
    CONFIG += link_pkgconfig
    PKGCONFIG += libevent libevent_openssl libssl libcrypto libcurl
    CONFIG(release, debug|release) {
        QMAKE_POST_LINK=$(STRIP) $(TARGET)
    }

    LIBS += -L/usr/local/lib64 -lwslay -L/usr/local/lib -levhtp -lpthread
}

macx {
    INCLUDEPATH += /usr/local/include
    LIBS += -L/usr/local/lib -levent
}

SOURCES += \
    main.cpp

#openssl_hostname_validation.c hostcheck.c
