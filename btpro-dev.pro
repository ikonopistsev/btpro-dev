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
    stomptalk

btdef.subdir = btdef
btpro.subdir = btpro
evheap.subdir = evheap
mcsrv.subdir = mcsrv
mccl.subdir = mccl
tcpsrv.subdir = tcpsrv
tcpcl.subdir = tcpcl
stompcl.subdir = stompcl
stomptalk.subdir = stomptalk

stompcl.depends = btpro btdef stomptalk
