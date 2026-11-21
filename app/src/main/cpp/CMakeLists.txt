cmake_minimum_required(VERSION 3.22)
# Modified by EoN Launcher V2: use a project-level C target for the Discord compatibility shim.
project(eon_launcher LANGUAGES C CXX)

set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries" FORCE)

if(ANDROID)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    add_link_options("-Wl,-z,max-page-size=16384")
endif()

add_subdirectory(preloader)
add_subdirectory(libHttpClient/Build/libHttpClient.Android)
add_subdirectory(worldtools)
add_subdirectory(eon)

# Modified by EoN Launcher V2: satisfy libgxcore.so's removed Discord SDK dependency.
add_library(discord_partner_sdk SHARED
    discord_stub/discord_stub.c
    discord_stub/emutls_shim.c
)
