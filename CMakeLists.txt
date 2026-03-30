cmake_minimum_required(VERSION 2.0)
project(Konata-Dancer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_WIN32_EXECUTABLE ON)

set(SOURCES
    timotei.cpp
    resource.h
)

set(RESOURCES
    konata.ico
    Konata.gif
    windows_xp_smth.png
)

add_executable(Konata-Dancer WIN32 ${SOURCES})

foreach(RES ${RESOURCES})
    configure_file(${RES} ${CMAKE_CURRENT_BINARY_DIR}/${RES} COPYONLY)
endforeach()

target_link_libraries(Konata-Dancer
    gdi32
    gdiplus
    shell32
    user32
    comctl32
)

if(WIN32)
    target_compile_definitions(Konata-Dancer PRIVATE
        _WIN32_WINNT=0x0600
        UNICODE
        _UNICODE
    )
    
    target_link_libraries(Konata-Dancer comctl32)
    
    set(RC_FILE resource.rc)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${RC_FILE} 
        "IDI_ICON1 ICON \"konata.ico\"\n")
    
    target_sources(Konata-Dancer PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/${RC_FILE})
    configure_file(konata.ico ${CMAKE_CURRENT_BINARY_DIR}/konata.ico COPYONLY)
endif()
