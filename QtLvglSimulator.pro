QT += core gui widgets
TEMPLATE = app
TARGET = lvgl_simulator
CONFIG += c++17 c99 object_parallel_to_source

LVGL_DIR = $$PWD/ThirdParty/lvgl
!exists($$LVGL_DIR/lvgl.h) {
    error("LVGL is missing. Run: git submodule update --init --recursive")
}

DEFINES += LV_CONF_INCLUDE_SIMPLE LV_KCONFIG_IGNORE
INCLUDEPATH += $$PWD $$PWD/Application $$PWD/Simulator $$LVGL_DIR $$LVGL_DIR/include

SOURCES += \
    $$PWD/Simulator/main.cpp \
    $$PWD/Simulator/lvgl_widget.cpp \
    $$PWD/Application/app_ui.c \
    $$PWD/Application/app_climate.c

HEADERS += \
    $$PWD/Simulator/lvgl_widget.h \
    $$PWD/Application/app_ui.h \
    $$PWD/Application/app_climate.h \
    $$PWD/lv_conf.h

SOURCES += $$files($$LVGL_DIR/src/*.c, true)
HEADERS += $$files($$LVGL_DIR/include/*.h, true)

DISTFILES += README.md ThirdParty/README.md CMakeLists.txt Tests/tst_simulator.cpp