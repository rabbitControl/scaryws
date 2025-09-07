cmake_minimum_required(VERSION 3.30)

project(scaryws LANGUAGES CXX VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (WIN32)
  if (MSVC)
    add_compile_options(/bigobj)
  else()
    add_compile_options(-Wa,-mbig-obj)
  endif()
endif()

add_library(${PROJECT_NAME} STATIC
  # client
  WebsocketClient.h WebsocketClient.cpp
  ClientSessionBase.h ClientSessionBase.cpp
  ClientSession.h ClientSession.cpp
  ClientSessionSSL.h ClientSessionSSL.cpp
  IClientSessionListener.h
  # server
  WebsocketServer.h WebsocketServer.cpp
  ServerListener.h ServerListener.cpp
  ServerSession.h ServerSession.cpp
  IServerSessionListener.h
)

# openssl
set(OPENSSL_PATH "" CACHE PATH "Custom OpenSSL path")

# boost
set(USE_CUSTOM_BOOST_URL FALSE CACHE BOOL "Use custom path for boost and boost_url")
set(BOOST_INCLUDE_PATH "" CACHE PATH "Path for boost includes")
set(BOOST_URL_LIB_PATH "" CACHE FILEPATH "Path for boost_url library")

# boost certify
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/certify/include)


function(scaryws_setup_target TARGET)

  #
  # boost
  #
  if (USE_CUSTOM_BOOST_URL)

    message("Using custom path for boost and boost_url")
    message("boost path: ${BOOST_INCLUDE_PATH}")
    message("boost_url path: ${BOOST_URL_LIB_PATH}")

    include_directories(${BOOST_INCLUDE_PATH})

    target_link_libraries(${TARGET} PRIVATE ${BOOST_URL_LIB_PATH})

  else()

    set(Boost_USE_STATIC_LIBS TRUE)
    find_package(Boost CONFIG REQUIRED)
    find_package(Boost REQUIRED COMPONENTS url)

    if(Boost_FOUND)
      message("Found Boost: ${Boost_INCLUDE_DIRS}")
      include_directories(${Boost_INCLUDE_DIRS})
      target_link_libraries(${TARGET} PRIVATE Boost::url)
    endif()

  endif()


  #
  # openssl
  #
  if("${OPENSSL_PATH}" STREQUAL "")

    set(OPENSSL_USE_STATIC_LIBS TRUE)
    find_package(OpenSSL REQUIRED)

    if (OpenSSL_FOUND)

      # OPENSSL_VERSION
      # OPENSSL_LIBRARIES

      message("Found OpenSSL at: ${OPENSSL_INCLUDE_DIR}")
      include_directories(${OPENSSL_INCLUDE_DIR})
      target_link_libraries(${TARGET} PRIVATE OpenSSL::SSL)
    endif()

  else()

    message("Using OpenSSL path: ${OPENSSL_PATH}")

    # set custom ssl libraries
    include_directories(${OPENSSL_PATH}/include)
    set(CUSTOM_SSL_LIB_PATH ${OPENSSL_PATH}/lib)

    set( _rcp_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})

    if(WIN32)
        list(INSERT CMAKE_FIND_LIBRARY_SUFFIXES 0 .lib .a)
    else()
      set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
    endif()

    if (WIN32)
      find_library(SSL_LIB NAMES ssl_static REQUIRED NO_CACHE NO_DEFAULT_PATH PATHS ${CUSTOM_SSL_LIB_PATH})
      find_library(CRYPTO_LIB NAMES crypto_static REQUIRED NO_CACHE NO_DEFAULT_PATH PATHS ${CUSTOM_SSL_LIB_PATH})
    else()
      find_library(SSL_LIB NAMES ssl REQUIRED NO_CACHE NO_DEFAULT_PATH PATHS ${CUSTOM_SSL_LIB_PATH})
      find_library(CRYPTO_LIB NAMES crypto REQUIRED NO_CACHE NO_DEFAULT_PATH PATHS ${CUSTOM_SSL_LIB_PATH})
    endif()

    target_link_libraries(${TARGET} PRIVATE ${SSL_LIB} ${CRYPTO_LIB})

  endif()


  if (APPLE)

    target_link_libraries(${TARGET} PRIVATE
      "-framework CoreFoundation"
      "-framework Security"
    )

  elseif (WIN32)

    target_link_libraries(${TARGET} PRIVATE crypt32 wsock32 ws2_32)

  endif()

endfunction()


scaryws_setup_target(${PROJECT_NAME})
