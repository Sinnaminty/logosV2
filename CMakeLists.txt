	cmake_minimum_required(VERSION 3.22)
project(Logos VERSION 0.0.1 DESCRIPTION "Discord Bot using the DPP API lib. Created by Fizzy.")
	 
	list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
	 
	add_executable(${PROJECT_NAME}
	    src/main.cpp
      src/info.h
	)
	 
	find_package(DPP REQUIRED)
	 
	target_link_libraries(${PROJECT_NAME} 
	    ${DPP_LIBRARIES}
	)
	 
	target_include_directories(${PROJECT_NAME} PRIVATE
	    ${DPP_INCLUDE_DIR}
	)
	 
	set_target_properties(${PROJECT_NAME} PROPERTIES
	    CXX_STANDARD 17
	    CXX_STANDARD_REQUIRED ON
	)
