project(Program)

	find_package(WiringPi REQUIRED)
	find_package(Threads REQUIRED)
	find_package(OpenCV REQUIRED)

	add_executable(Program ./src/main.cpp)
	add_executable(Program ./src/vision.cpp)

	include_directories(./inc)
	include_directories(${WIRINGPI_INCLUDE_DIRS})
	add_definitions(-lwiringPi -lpthread)

	target_link_libraries(Program ${OpenCV_LIBS})
	target_link_libraries(Program ${WIRINGPI_LIBRARIES}) 
	target_link_libraries(Program ${CMAKE_THREAD_LIBS_INIT}) 
