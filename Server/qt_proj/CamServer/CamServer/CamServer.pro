TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_LINK += -pthread

SOURCES += main.cpp \
    NetworkServer.cpp \
    PThreadPool.cpp \
    CamControlserver.cpp \
    ConfigManager.cpp \
    md5.cpp \
    SerialPort.cpp \
    FixServoControl.cpp \
    thread_enabler.c \
    CntServoControl.cpp

include(deployment.pri)
qtcAddDeployment()

HEADERS += \
    NetworkServer.h \
    PThreadPool.h \
    nullptr.h \
    CamControlserver.h \
    ConfigManager.h \
    md5.h \
    SerialPort.h \
    ServoControl.h \
    FixServoControl.h \
    thread_enabler.h \
    CntServoControl.h

