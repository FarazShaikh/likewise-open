noinst_LTLIBRARIES = libsrv.protocols.smb2.la

libsrv_protocols_smb2_la_SOURCES = \
    cancel.c         \
    close.c          \
    connection.c     \
    config.c         \
    create.c         \
    echo.c           \
    error.c          \
    find.c           \
    flush.c          \
    getfileinfo.c    \
    getfsinfo.c      \
    getinfo.c        \
    getquotainfo.c   \
    getsecinfo.c     \
    globals.c        \
    ioctl.c          \
    libmain.c        \
    lock.c           \
    logging.c        \
    logoff.c         \
    negotiate.c      \
    notify_request.c \
    notify_state.c   \
    oplock.c         \
    read.c           \
    session.c        \
    session_setup.c  \
    setinfo.c        \
    tree.c           \
    treeconnect.c    \
    treedisconnect.c \
    utils.c          \
    wire.c           \
    write.c

libsrv_protocols_smb2_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/server/srv/include \
    -I@top_srcdir@/server/srv/protocols/include

libsrv_protocols_smb2_la_LDFLAGS =

