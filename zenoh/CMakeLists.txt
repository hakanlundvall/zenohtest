include(FetchContent)

FetchContent_Declare(zenohcpp
  GIT_REPOSITORY https://github.com/eclipse-zenoh/zenoh-cpp.git
  GIT_TAG        a08baa4b357061996c00b0b318ed5e014ad84ffa
  PATCH_COMMAND git restore . && git apply ${CMAKE_CURRENT_LIST_DIR}/zenohcpp.patch 
)

FetchContent_Declare(zenohc
  GIT_REPOSITORY https://github.com/eclipse-zenoh/zenoh-c.git
  GIT_TAG        8162f13a2a331b72faa745e5e1e56de54e75d10d
  PATCH_COMMAND git restore . && git apply ${CMAKE_CURRENT_LIST_DIR}/zenohc.patch 
)

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog
  GIT_TAG        v1.8.2
)

FetchContent_MakeAvailable(spdlog)
FetchContent_MakeAvailable(zenohcpp zenohc)
FetchContent_GetProperties(zenohc)

add_library(myzenohcpp INTERFACE)
target_include_directories(myzenohcpp INTERFACE ${zenohc_SOURCE_DIR}/include)
target_link_directories(myzenohcpp INTERFACE ${zenohc_SOURCE_DIR}/target/release)
target_link_libraries(myzenohcpp INTERFACE zenohcpp zenohc)

add_executable(zenohtest
  main.cpp
)

set_property(TARGET zenohtest PROPERTY CXX_STANDARD 20)
set_property(TARGET zenohtest PROPERTY EXPORT_COMPILE_COMMANDS 1)
target_link_libraries(zenohtest PUBLIC myzenohcpp spdlog)


