add_library(engine STATIC)

target_sources(engine
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
            MoveGen.cppm
            Perft.cppm
            Search.cppm 
            TranspositionTable.cppm
            See.cppm
            Evaluation.cppm
)
target_compile_features(engine PUBLIC cxx_std_20)
set_target_properties(engine PROPERTIES CXX_SCAN_FOR_MODULES ON)
target_link_libraries(engine PUBLIC core)

if (MSVC)
    target_compile_options(core PRIVATE /permissive- /W4 /interface) 
else()
    target_compile_options(core PRIVATE -Wall -Wextra -Wpedantic)
endif()