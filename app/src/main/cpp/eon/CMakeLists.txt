# Modified by EoN Launcher V2.
cmake_minimum_required(VERSION 3.22)
project(eon LANGUAGES CXX)

add_library(eon SHARED
        src/eon.cpp
        src/game_profile.cpp
        src/mod_lifecycle.cpp
        src/module.cpp
        src/module_manager.cpp
        src/modules.cpp)

target_include_directories(eon PRIVATE include)
target_compile_features(eon PRIVATE cxx_std_20)
target_compile_options(eon PRIVATE -Wall -Wextra -Wpedantic)
target_link_options(eon PRIVATE -Wl,-z,max-page-size=16384)

find_library(log-lib log)
find_library(dl-lib dl)
target_link_libraries(eon PRIVATE
        ${log-lib}
        ${dl-lib}
        preloader
        nlohmann_json::nlohmann_json
        Boost::pfr
        magic_enum::magic_enum)
