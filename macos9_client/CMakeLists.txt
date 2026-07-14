cmake_minimum_required(VERSION 3.10)
project(AltanAI C)

set(CMAKE_C_STANDARD 90)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-multichar -Wno-pointer-sign")

find_file(OPEN_TRANSPORT_APP_PPC
    NAMES OpenTransportAppPPC.o libOpenTransportAppPPC.a
    PATHS
        "${RETRO68_ROOT}/powerpc-apple-macos/lib"
        "${RETRO68_ROOT}/universal/libppc"
        "${RETRO68_ROOT}/multiversal/libppc"
    NO_DEFAULT_PATH
)

add_application(AltanAI
    TYPE APPL
    CREATOR ALTN
    src/common.h
    src/main.c
    src/network.c
    src/utils.c
    src/dialogs.c
    src/file_ops.c
    src/ai_context.c
    src/ai_executor.c
    src/ai_diff.c
    src/ai_history.c
    src/editor.c
    src/ui_draw.c
    src/ui_events.c
    src/ui_actions.c
    src/ui_menu.c
    src/ui_utils.c
    rsrc/MacApp.r
)

if(CMAKE_SYSTEM_NAME MATCHES RetroCarbon)
    target_compile_definitions(AltanAI PRIVATE ENABLE_CLASSIC_NETWORK=0)
    target_link_libraries(AltanAI
        InterfaceLib
        CarbonLib
        DriverServicesLib
        InternetConfigLib
        retrocrt
    )
else()
    if(OPEN_TRANSPORT_APP_PPC)
        message(STATUS "Found PPC Open Transport glue: ${OPEN_TRANSPORT_APP_PPC}")
        target_compile_definitions(AltanAI PRIVATE ENABLE_CLASSIC_NETWORK=1)
        target_link_libraries(AltanAI
            InterfaceLib
            AppearanceLib
            IconServicesLib
            DriverServicesLib
            NavigationLib
            ${OPEN_TRANSPORT_APP_PPC}
            OpenTransportLib
            OpenTptInternetLib
            InternetConfigLib
            retrocrt
        )
    else()
        message(WARNING
            "OpenTransportAppPPC.o was not found. "
            "Classic networking will be disabled to avoid PPC Open Transport crashes. "
            "Install Apple Universal Interfaces/Libraries 3.x into the Retro68 toolchain "
            "to enable the network client."
        )
        target_compile_definitions(AltanAI PRIVATE ENABLE_CLASSIC_NETWORK=0)
        target_link_libraries(AltanAI
            InterfaceLib
            AppearanceLib
            IconServicesLib
            DriverServicesLib
            NavigationLib
            InternetConfigLib
            retrocrt
        )
    endif()
endif()

target_include_directories(AltanAI PRIVATE src/)
