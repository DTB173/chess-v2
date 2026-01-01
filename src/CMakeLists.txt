add_executable(chess main.cpp)

target_link_libraries(chess
	PRIVATE
		core
		engine
)

set_target_properties(chess PROPERTIES 
	VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)
