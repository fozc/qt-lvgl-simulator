cmake_minimum_required(VERSION 3.21)
project(QtLvglSimulator VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_SIMULATOR "Build the Qt desktop window" ON)
option(BUILD_TESTING "Build the simulator tests" OFF)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/lvgl/CMakeLists.txt")
    message(FATAL_ERROR "LVGL is missing. Run: git submodule update --init --recursive")
endif()

# LVGL is local: no downloads, demos, optional graphics libraries or install rules.
set(LV_BUILD_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h" CACHE PATH "LVGL configuration" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG OFF CACHE BOOL "" FORCE)
set(LV_FETCH_DEPENDENCIES OFF CACHE BOOL "" FORCE)
set(LV_USE_FIND_PACKAGE OFF CACHE BOOL "" FORCE)
set(LV_USE_PKG_CONFIG OFF CACHE BOOL "" FORCE)
set(LV_BUILD_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(ThirdParty/lvgl)

# This target is plain C. It does not depend on Qt or the simulator.
add_library(application STATIC
    Application/app_ui.c
    Application/app_ui.h
    Application/app_climate.c
    Application/app_climate.h
)
target_include_directories(application PUBLIC Application)
target_link_libraries(application PUBLIC lvgl::lvgl)

if(BUILD_SIMULATOR)
    find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

    add_executable(lvgl_simulator WIN32
        Simulator/main.cpp
        Simulator/lvgl_widget.cpp
        Simulator/lvgl_widget.h
    )
    target_link_libraries(lvgl_simulator PRIVATE application Qt${QT_VERSION_MAJOR}::Widgets)

    if(BUILD_TESTING)
        enable_testing()
        find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)
        add_executable(simulator_test
            Tests/tst_simulator.cpp
            Simulator/lvgl_widget.cpp
            Simulator/lvgl_widget.h
        )
        set_target_properties(simulator_test PROPERTIES AUTOMOC ON)
        target_include_directories(simulator_test PRIVATE Simulator)
        target_link_libraries(simulator_test PRIVATE application Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Test)
        add_test(NAME simulator_ui COMMAND simulator_test)
        set_tests_properties(simulator_ui PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)
    endif()
endif()