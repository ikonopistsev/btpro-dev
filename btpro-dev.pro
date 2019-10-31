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
    tcpcl

btdef.subdir = btdef
btpro.subdir = btpro
evheap.subdir = evheap
mcsrv.subdir = mcsrv
mccl.subdir = mccl
tcpsrv.subdir = tcpsrv
tcpcl.subdir = tcpcl
