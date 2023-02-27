include(FetchContent)

FetchContent_Declare(zenohcpp
  SYSTEM
  GIT_REPOSITORY https://github.com/eclipse-zenoh/zenoh-cpp.git
  GIT_TAG        a08baa4b357061996c00b0b318ed5e014ad84ffa
  PATCH_COMMAND git restore . && git apply ${CMAKE_CURRENT_LIST_DIR}/zenohcpp.patch
)

FetchContent_Declare(zenohc
  SYSTEM
  GIT_REPOSITORY https://github.com/eclipse-zenoh/zenoh-c.git
  GIT_TAG        8162f13a2a331b72faa745e5e1e56de54e75d10d
  PATCH_COMMAND git restore . && git apply ${CMAKE_CURRENT_LIST_DIR}/zenohc.patch 
)

FetchContent_Declare(spdlog
  SYSTEM
  GIT_REPOSITORY https://github.com/gabime/spdlog
  GIT_TAG        v1.8.2
)

FetchContent_MakeAvailable(spdlog)
FetchContent_MakeAvailable(zenohcpp zenohc)
FetchContent_GetProperties(zenohc)

add_library(myzenohcpp INTERFACE)
target_include_directories(myzenohcpp SYSTEM INTERFACE ${zenohc_SOURCE_DIR}/include)



if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "cortexa53-crypto" OR ${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")
  set(libzenohc_target "aarch64-unknown-linux-musl")
else()
  set(libzenohc_target "x86_64-unknown-linux-gnu")
endif()

set(libzenohc_libdir "${zenohc_SOURCE_DIR}/target/${libzenohc_target}/release")

add_custom_command(
  OUTPUT ${libzenohc_libdir}/libzenohc.a
  WORKING_DIRECTORY ${zenohc_SOURCE_DIR}
  COMMAND  cargo build --release --features=logger-autoinit --target ${libzenohc_target} 
)

add_custom_target(build_zenohc
  DEPENDS ${libzenohc_libdir}/libzenohc.a
)

target_link_directories(myzenohcpp INTERFACE ${libzenohc_libdir})
target_link_libraries(myzenohcpp INTERFACE zenohcpp libzenohc.a dl)
add_dependencies(myzenohcpp build_zenohc)
add_executable(zenohtest
  main.cpp
)

set_property(TARGET zenohtest PROPERTY CXX_STANDARD 20)
set_property(TARGET zenohtest PROPERTY EXPORT_COMPILE_COMMANDS 1)
target_link_libraries(zenohtest PUBLIC myzenohcpp spdlog)


