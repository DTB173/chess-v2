add_library(core INTERFACE)

target_include_directories(core
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_sources(core
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/Types.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Bitboards.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Position.h
        ${CMAKE_CURRENT_SOURCE_DIR}/AttackTables.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Magics.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Zobrist.h
        ${CMAKE_CURRENT_SOURCE_DIR}/PSQTables.h
)

target_compile_features(core INTERFACE cxx_std_20)

if(MSVC)
    target_compile_options(core INTERFACE
        /permissive-
        /W4
    )
else()
    target_compile_options(core INTERFACE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()