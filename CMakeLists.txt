cmake_minimum_required(VERSION 3.14)
project(ipc
  VERSION 0.1.0)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

option(BUILD_TESTING "" ON)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

if(WIN32)
  set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/win32)
  set(PLATFORM __WIN32__)
elseif(UNIX OR LINUX OR MACOSX)
  set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/posix)
  set(PLATFORM __POSIX__)
endif()

aux_source_directory(${SRC_DIR} SRC)

add_library(ipc STATIC "")
target_include_directories(ipc 
  PUBLIC "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>"
  PUBLIC "$<INSTALL_INTERFACE:include/shm_kernel>"
)
target_sources(ipc PRIVATE ${SRC} ${CMAKE_CURRENT_SOURCE_DIR}/src/ec.cxx ${CMAKE_CURRENT_SOURCE_DIR}/src/except.cxx)
target_compile_features(ipc PUBLIC cxx_std_17)
target_compile_definitions(ipc PUBLIC ${PLATFORM})
if (NOT WIN32)
  target_link_libraries(ipc PUBLIC rt pthread)
  target_compile_options(ipc PRIVATE -fPIC)
endif()

if(BUILD_TESTING)
  find_package(Catch2 REQUIRED)
  enable_testing()

  add_library(Testcase_main OBJECT "")
  target_sources(Testcase_main PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests/Testcase_main.cxx)
  target_link_libraries(Testcase_main PUBLIC Catch2::Catch2 ipc)

  add_executable(Testcase_shmhdl "")
  target_sources(Testcase_shmhdl PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests/Testcase_shmhdl.cxx)
  target_link_libraries(Testcase_shmhdl PRIVATE Testcase_main)

  add_executable(Testcase_semhdl "")
  target_sources(Testcase_semhdl PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests/Testcase_semhdl.cxx)
  target_link_libraries(Testcase_semhdl PRIVATE Testcase_main)

  add_test(NAME shmhdl 
    COMMAND ./Testcase_shmhdl
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/
  )
  add_test(NAME semhdl 
    COMMAND ./Testcase_semhdl 
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/)
endif()

write_basic_package_version_file(${CMAKE_PROJECT_NAME}ConfigVersion.cmake
      VERSION ${CMAKE_PROJECT_VERSION}
      COMPATIBILITY SameMajorVersion)

install(TARGETS ipc 
  EXPORT 
     ipcTargets
  RUNTIME 
      DESTINATION ${CMAKE_INSTALL_BINDIR} 
  LIBRARY 
      DESTINATION ${CMAKE_INSTALL_LIBDIR} 
  ARCHIVE 
      DESTINATION ${CMAKE_INSTALL_LIBDIR} 
  PUBLIC_HEADER 
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

install(FILES 
          ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
          ${CMAKE_CURRENT_SOURCE_DIR}/cmake/${CMAKE_PROJECT_NAME}Config.cmake
        DESTINATION 
          lib/cmake/ipc)
install(FILES 
        ${CMAKE_CURRENT_SOURCE_DIR}/include/common.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ec.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/except.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/semhdl.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/shmhdl.hpp
      DESTINATION
        include/shm_kernel/ipc
  )

  install(EXPORT ipcTargets
        FILE ipcTargets.cmake
        NAMESPACE shm_kernel::
        DESTINATION lib/cmake/ipc)
