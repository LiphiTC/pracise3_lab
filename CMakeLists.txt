cmake_minimum_required(VERSION 3.16)
project(Practical_03_GroundStation LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)
add_library(groundstation_model
    uav.h uav.cpp sensor.h sensor.cpp navigation.h navigation.cpp
    controller.h controller.cpp autopilot.h autopilot.cpp point3d.h)
target_include_directories(groundstation_model PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(groundstation_model PUBLIC Qt${QT_VERSION_MAJOR}::Core)
if(MSVC)
    target_compile_options(groundstation_model PUBLIC /utf-8)
endif()

option(GROUNDSTATION_BUILD_GUI "Build the ground station window" ON)
if(GROUNDSTATION_BUILD_GUI)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
        Widgets Charts 3DCore 3DRender 3DExtras 3DInput Multimedia Svg)
    add_library(groundstation_gui STATIC
        mainwindow.h mainwindow.cpp altitudegraph.h altitudegraph.cpp
        routeview.h routeview.cpp scene3d.h scene3d.cpp
        dronevisual.h dronevisual.cpp flightaudio.h flightaudio.cpp)
    target_link_libraries(groundstation_gui PUBLIC groundstation_model
        Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Charts
        Qt${QT_VERSION_MAJOR}::3DCore Qt${QT_VERSION_MAJOR}::3DRender
        Qt${QT_VERSION_MAJOR}::3DExtras Qt${QT_VERSION_MAJOR}::3DInput
        Qt${QT_VERSION_MAJOR}::Multimedia Qt${QT_VERSION_MAJOR}::Svg)
    add_executable(Practical_03_GroundStation main.cpp assets.qrc)
    target_link_libraries(Practical_03_GroundStation PRIVATE groundstation_gui)
endif()

include(CTest)
if(BUILD_TESTING)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)
    add_executable(groundstation_tests tests.cpp)
    target_link_libraries(groundstation_tests PRIVATE groundstation_model Qt${QT_VERSION_MAJOR}::Test)
    add_test(NAME groundstation_model COMMAND groundstation_tests)
    if(GROUNDSTATION_BUILD_GUI)
        add_executable(groundstation_window_tests window_tests.cpp assets.qrc)
        target_link_libraries(groundstation_window_tests PRIVATE groundstation_gui Qt${QT_VERSION_MAJOR}::Test)
        add_test(NAME groundstation_window COMMAND groundstation_window_tests)
        set_tests_properties(groundstation_window PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 90)
    endif()
endif()
