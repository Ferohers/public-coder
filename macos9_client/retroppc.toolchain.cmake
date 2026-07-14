# Retro68 PPC Toolchain (no Carbon)
set( CMAKE_SYSTEM_NAME RetroPPC )
set( CMAKE_SYSTEM_VERSION 1)

if(DEFINED ENV{RETRO68_ROOT})
    set( RETRO68_DEFAULT_ROOT "$ENV{RETRO68_ROOT}" )
elseif(EXISTS "/Users/altan/Documents/ambitious/Retro68-build/toolchain")
    set( RETRO68_DEFAULT_ROOT "/Users/altan/Documents/ambitious/Retro68-build/toolchain" )
else()
    set( RETRO68_DEFAULT_ROOT "/opt/retro68/toolchain" )
endif()

set( RETRO68_ROOT "${RETRO68_DEFAULT_ROOT}" CACHE PATH "path to root of Retro68 Toolchain" )
set( CMAKE_INSTALL_PREFIX "${RETRO68_ROOT}/powerpc-apple-macos/" CACHE PATH "installation prefix" )
set( CMAKE_SYSTEM_PREFIX_PATH "${RETRO68_ROOT}/powerpc-apple-macos/" )

set( MAKE_PEF "${RETRO68_ROOT}/bin/MakePEF" )
set( MAKE_IMPORT "${RETRO68_ROOT}/bin/MakeImport" )
set( REZ "${RETRO68_ROOT}/bin/Rez" )
set( REZ_INCLUDE_PATH "${RETRO68_ROOT}/powerpc-apple-macos/RIncludes" )

set( CMAKE_C_COMPILER "${RETRO68_ROOT}/bin/powerpc-apple-macos-gcc" )
set( CMAKE_CXX_COMPILER "${RETRO68_ROOT}/bin/powerpc-apple-macos-g++" )

set( REZ_TEMPLATES_PATH ${REZ_INCLUDE_PATH})

list( APPEND CMAKE_MODULE_PATH "${RETRO68_ROOT}/powerpc-apple-macos/cmake" )
include(add_application)
