add_executable(test_grovedb
  main.cpp
  whoami.cpp
)

target_include_directories(test_grovedb PRIVATE
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_BINARY_DIR}/src
  ${Boost_INCLUDE_DIRS}
)

if(DEFINED GROVEDB_CXX_INCLUDE_DIR)
  target_include_directories(test_grovedb PRIVATE ${GROVEDB_CXX_INCLUDE_DIR})
endif()

target_link_libraries(test_grovedb PRIVATE grovedb)

# Link Rust/CXX bridge libraries when provided by the parent project
if(DEFINED GROVEDB_CXX_LIBRARIES)
  target_link_libraries(test_grovedb PRIVATE ${GROVEDB_CXX_LIBRARIES})
endif()

add_test(NAME test_grovedb COMMAND test_grovedb)
