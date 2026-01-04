add_library(engine STATIC)

target_sources(engine
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
        ChessEngine.cppm
)

target_link_libraries(engine PUBLIC core)