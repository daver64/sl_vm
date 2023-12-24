
##
## SL_VM Project Details
##
## (c) 2023 David Rowbotham (thedaver64@gmail.com)
##
##
cmake_minimum_required(VERSION 3.0...3.22)
project("sl_vm")
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "64 bit compiler required.")
endif(CMAKE_SIZEOF_VOID_P EQUAL 4)

##
## MSVC Compile Flags
##
if(MSVC)
set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CXX_FLAGS "/EHsc /Qpar /GS- /O2 /D NDEBUG /std:c++20")
endif(MSVC)


##
## Linux Compile Flags
##
if(UNIX)
set(CMAKE_CXX_FLAGS "-std=c++20 -O2")
endif(UNIX)


##
## MINGW. Not supported atm.
##
if(MINGW)
set(CMAKE_CXX_FLAGS "-std=c++20 -O2")
endif(MINGW)


##
## Project Build.
##

add_executable(sl_vm_test 
    main.cpp)



##
## MSVC Platform Link Options
##
if(MSVC)
set_target_properties(sl_vm_test PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup /SUBSYSTEM:CONSOLE")
endif(MSVC)

