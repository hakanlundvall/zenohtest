include(FetchContent)

FetchContent_Declare(zenohcxx 
  SYSTEM
  GIT_REPOSITORY https://github.com/eclipse-zenoh/zenoh-cpp.git 
  GIT_TAG c384caccef36253ba13d6fb575b433cab0f1ac59
)

FetchContent_Declare(zenohc
  SYSTEM
  GIT_REPOSITORY https://github.com/eclipse-zenoh/zenoh-c.git
  GIT_TAG        0766a78a6bdbf29aca44a6436c6ca6b8a481b451
)

FetchContent_Declare(spdlog
  SYSTEM
  GIT_REPOSITORY https://github.com/gabime/spdlog
  GIT_TAG        v1.8.2
)

FetchContent_Declare(protobuf
  SYSTEM
  GIT_REPOSITORY https://github.com/protocolbuffers/protobuf.git
  GIT_TAG        f0dc78d7e6e331b8c6bb2d5283e06aa26883ca7c
)
set(protobuf_BUILD_TESTS OFF CACHE INTERNAL "")

FetchContent_MakeAvailable(protobuf)
FetchContent_MakeAvailable(spdlog)
FetchContent_MakeAvailable(zenohc)
FetchContent_MakeAvailable(zenohcxx)

# Generates cpp code for grpc and protobuf from a list of proto files.
# SRC and HDRS will contain the generated files.
function(generate_protobuf_cpp SRCS HDRS)
   if(NOT ARGN)
      message(SEND_ERROR "Error: generate_protobuf_cpp() called without any proto files")
      return()
   endif()

   # Create an include path for each file specified
   foreach(FIL ${ARGN})
      get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
      get_filename_component(ABS_PATH ${ABS_FIL} PATH)
      list(FIND _protobuf_include_path ${ABS_PATH} _contains_already)
      if(${_contains_already} EQUAL -1)
         list(APPEND _protobuf_include_path -I ${ABS_PATH})
      endif()
   endforeach()
   set(${SRCS})
   set(${HDRS})

   foreach(CURR_PROTO_FILE ${ARGN})
      get_filename_component(_proto_abs_file ${CURR_PROTO_FILE} ABSOLUTE)
      get_filename_component(_proto_file_we ${CURR_PROTO_FILE} NAME_WE)
      get_filename_component(_proto_dir ${CURR_PROTO_FILE} DIRECTORY)

      list(APPEND ${SRCS} "${CMAKE_CURRENT_BINARY_DIR}/${_proto_dir}/${_proto_file_we}.pb.cc")
      list(APPEND ${HDRS} "${CMAKE_CURRENT_BINARY_DIR}/${_proto_dir}/${_proto_file_we}.pb.h")

      add_custom_command(
         OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${_proto_dir}/${_proto_file_we}.pb.cc"
                "${CMAKE_CURRENT_BINARY_DIR}/${_proto_dir}/${_proto_file_we}.pb.h"
         COMMAND $<TARGET_FILE:protoc>
         ARGS --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
               -I ${CMAKE_CURRENT_SOURCE_DIR}
               -I ${CMAKE_BINARY_DIR}/_deps/protobuf-src/src/
         ${_proto_abs_file}
         DEPENDS ${_proto_abs_file} $<TARGET_FILE:protoc> 
         COMMENT "Running protocol buffer compiler on ${CURR_PROTO_FILE}"
         VERBATIM
      )
   endforeach()

   set_source_files_properties(${${SRCS}} ${${HDRS}} PROPERTIES GENERATED TRUE)
   set(${SRCS} ${${SRCS}} PARENT_SCOPE)
   set(${HDRS} ${${HDRS}} PARENT_SCOPE)
endfunction()

set(PROTOS
   src/test.proto
)

generate_protobuf_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS})

add_executable(zenohtest
  main.cpp
  ${PROTO_SRCS}
  ${PROTO_HDRS}
)

set_property(TARGET zenohtest PROPERTY CXX_STANDARD 20)
set_property(TARGET zenohtest PROPERTY EXPORT_COMPILE_COMMANDS 1)
target_link_libraries(zenohtest PUBLIC dl zenohcxx::zenohc::static zenohc::static spdlog libprotobuf dl)
target_include_directories(zenohtest SYSTEM PUBLIC   ${CMAKE_CURRENT_BINARY_DIR})


