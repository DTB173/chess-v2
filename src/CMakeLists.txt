add_executable(chess main.cpp)

target_sources(chess
    PUBLIC
		FILE_SET cxx_modules TYPE CXX_MODULES FILES
		CLI.cppm
)

target_link_libraries(chess
	PRIVATE
		core
		engine
)

set_target_properties(chess PROPERTIES 
	VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)
