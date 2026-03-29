add_executable(chess_engine main.cpp)

target_sources(chess_engine
    PRIVATE
        CLI.h
        UCI.h
        #Logger.h
)

target_link_libraries(chess_engine
    PRIVATE
        core
        engine
)

if(MSVC)
    set_target_properties(chess_engine PROPERTIES
        VS_GLOBAL_BasicRuntimeChecks "Default"
    )

    string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    string(REPLACE "/RTCs" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    string(REPLACE "/RTCu" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    string(REPLACE "/RTCc" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
else()
    target_compile_options(chess_engine PRIVATE
        -O3
        -march=native
        -flto
    )
    set_target_properties(chess_engine PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION TRUE
    )

    if(MINGW OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_link_options(chess_engine PRIVATE -Wl,--stack,16777216)
    endif()
endif()

set_target_properties(chess_engine PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)
