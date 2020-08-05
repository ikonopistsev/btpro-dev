TEMPLATE = subdirs

CONFIG -= qt
CONFIG -= app_bundle

SUBDIRS += \
    btdef \
    btpro \
    evheap \
    mcsrv \
    mccl \
    tcpsrv \
    tcpcl \
    stompcl \
    stomptalk \
    stompconn

btdef.subdir = btdef
btpro.subdir = btpro
evheap.subdir = evheap
mcsrv.subdir = mcsrv
mccl.subdir = mccl
tcpsrv.subdir = tcpsrv
tcpcl.subdir = tcpcl
stompcl.subdir = stompcl
stomptalk.subdir = stomptalk
stompconn.subdir = stompconn

stompcl.depends = btpro btdef stomptalk stompconn
