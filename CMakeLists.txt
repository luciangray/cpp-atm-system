cmake_minimum_required(VERSION 3.16)
project(cpp_atm_system LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(-Wall -Wextra -Wpedantic)

add_library(atm_core src/BankCard.cpp src/ATM.cpp src/Money.cpp)
target_include_directories(atm_core PUBLIC src)
target_compile_definitions(atm_core PUBLIC
    ATM_DEFAULT_DATA_FILE="${CMAKE_CURRENT_SOURCE_DIR}/data/atm_data.txt")

add_executable(atm_console src/main_console.cpp)
target_link_libraries(atm_console PRIVATE atm_core)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
set(CMAKE_AUTORCC ON)
add_library(atm_widgets src/ATMWindow.cpp src/BalanceChart.cpp resources.qrc)
target_link_libraries(atm_widgets PUBLIC atm_core Qt6::Widgets)
add_executable(atm_gui src/main_gui.cpp)
target_link_libraries(atm_gui PRIVATE atm_widgets)

include(CTest)
if(BUILD_TESTING)
    add_executable(core_tests tests/core_tests.cpp)
    target_link_libraries(core_tests PRIVATE atm_core)
    add_test(NAME core_tests COMMAND core_tests)

    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_test(NAME console_tests COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/console_tests.py $<TARGET_FILE:atm_console>)

    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_executable(gui_tests tests/gui_tests.cpp)
    target_link_libraries(gui_tests PRIVATE atm_widgets Qt6::Test)
    add_test(NAME gui_tests COMMAND gui_tests)
    set_tests_properties(gui_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

endif()
