#-------------------------------------------------
#
# Project created by QtCreator 2014-07-02T15:24:28
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = FFMPEG
TEMPLATE = app


SOURCES += main.cpp\
        widget.cpp \
    videothread.cpp

HEADERS  += widget.h \
    videothread.h

FORMS    += widget.ui

LIBS += -lavutil -lavformat -lavcodec -lswscale

QMAKE_CXXFLAGS += -std=c++11
