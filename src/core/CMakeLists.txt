add_library(core STATIC)

target_sources(core
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
        Types.cppm
        Bitboards.cppm
        Position.cppm
        AttackTables.cppm 
        Magics.cppm 
        Zobrist.cppm
)

target_compile_features(core PUBLIC cxx_std_20)

set_target_properties(core PROPERTIES CXX_SCAN_FOR_MODULES ON)

if (MSVC)
    target_compile_options(core PRIVATE /permissive- /W4 /interface) 
else()
    target_compile_options(core PRIVATE -Wall -Wextra -Wpedantic)
endif()
