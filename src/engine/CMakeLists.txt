add_library(engine INTERFACE)

target_include_directories(engine
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_sources(engine
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/MoveGen.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Perft.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Search.h
        ${CMAKE_CURRENT_SOURCE_DIR}/TranspositionTable.h
        ${CMAKE_CURRENT_SOURCE_DIR}/See.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Evaluationv2.h
        ${CMAKE_CURRENT_SOURCE_DIR}/PawnTable.h
)

target_compile_features(engine INTERFACE cxx_std_20)

if(MSVC)
    target_compile_options(engine INTERFACE
        /permissive-
        /W4
    )
else()
    target_compile_options(engine INTERFACE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

target_link_libraries(engine INTERFACE core)