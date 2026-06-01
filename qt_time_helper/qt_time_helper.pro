QT += core network

CONFIG += console c++11
CONFIG -= app_bundle

TEMPLATE = app
TARGET = qt_time_helper

BUILD_ARTIFACTS_DIR = $$OUT_PWD/.build
OBJECTS_DIR = $$BUILD_ARTIFACTS_DIR/obj
MOC_DIR = $$BUILD_ARTIFACTS_DIR/moc
RCC_DIR = $$BUILD_ARTIFACTS_DIR/rcc
UI_DIR = $$BUILD_ARTIFACTS_DIR/ui

SOURCES += \
    src/main.cpp \
    src/qt_time_helper_server.cpp

HEADERS += \
    src/qt_time_helper_server.h
