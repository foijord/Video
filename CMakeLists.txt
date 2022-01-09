cmake_minimum_required (VERSION 3.21.1)
project(Video LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS YES CACHE BOOL "Export all symbols")

add_compile_definitions($<$<CONFIG:Debug>:DEBUG>)

if(WIN32)
	# target Windows >= 7 (Boost requires this)
	add_compile_options(-D _WIN32_WINNT=0x0601)
	# warning level 3, warnings as errors, big object files
	add_compile_options(/W3 /WX /bigobj /wd26812)
	# /Zi tells the compiler to put debug info into the OBJ file
	add_compile_options($<$<NOT:$<CONFIG:Debug>>:/Zi>)
	# /DEBUG tells the linker that we want to create a PDB file.
	add_link_options($<$<NOT:$<CONFIG:Debug>>:/DEBUG>)
	# /OPT:REF tells the linker to only put the functions that are referenced into the output binary.
	add_link_options($<$<NOT:$<CONFIG:Debug>>:/OPT:REF>)
	# /OPT:ICF tells the linker to look for functions that have identical assembly language code and only generate one of them
	add_link_options($<$<NOT:$<CONFIG:Debug>>:/OPT:ICF>)
endif(WIN32)

find_package(Boost REQUIRED)
find_package(Vulkan REQUIRED)
find_package(Threads REQUIRED)
find_package(glfw3 3.3 REQUIRED)

include_directories(
	${Boost_INCLUDE_DIRS}
	${Vulkan_INCLUDE_DIRS}
	${CMAKE_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/../ffmpeg-master-latest-win64-gpl-shared/include
	${CMAKE_SOURCE_DIR}/../glm/
	${CMAKE_SOURCE_DIR}/../gli/
	${CMAKE_SOURCE_DIR}/../)

link_directories(
	$ENV{VULKAN_SDK}/Lib
	${CMAKE_SOURCE_DIR}/../ffmpeg-master-latest-win64-gpl-shared/lib
	${Boost_LIBRARY_DIRS}
	${CMAKE_BINARY_DIR})

set(TARGET Video)

set(INNOVATOR_HEADER_FILES
	${CMAKE_SOURCE_DIR}/../Innovator/Exceptions.h
	${CMAKE_SOURCE_DIR}/../Innovator/GliTextureImage.h
	${CMAKE_SOURCE_DIR}/../Innovator/GroupNodes.h
	${CMAKE_SOURCE_DIR}/../Innovator/LruCache.h
	${CMAKE_SOURCE_DIR}/../Innovator/Node.h
	${CMAKE_SOURCE_DIR}/../Innovator/Nodes.h
	${CMAKE_SOURCE_DIR}/../Innovator/Sensors.h
	${CMAKE_SOURCE_DIR}/../Innovator/SparseImageMemory.h
	${CMAKE_SOURCE_DIR}/../Innovator/State.h
	${CMAKE_SOURCE_DIR}/../Innovator/Surface.h
	${CMAKE_SOURCE_DIR}/../Innovator/Swapchain.h
	${CMAKE_SOURCE_DIR}/../Innovator/TileKey.h
	${CMAKE_SOURCE_DIR}/../Innovator/Timer.h
	${CMAKE_SOURCE_DIR}/../Innovator/Visitor.h
	${CMAKE_SOURCE_DIR}/../Innovator/VulkanBufferData.h
	${CMAKE_SOURCE_DIR}/../Innovator/VulkanDevice.h
	${CMAKE_SOURCE_DIR}/../Innovator/VulkanInstance.h
	${CMAKE_SOURCE_DIR}/../Innovator/VulkanObjects.h
	${CMAKE_SOURCE_DIR}/../Innovator/VulkanTextureImage.h
	${CMAKE_SOURCE_DIR}/../Innovator/Window.h
	${CMAKE_SOURCE_DIR}/../Innovator/Worker.h
	${CMAKE_SOURCE_DIR}/../Innovator/VideoNodes.h)

add_executable(${TARGET}
	main.cpp
	ExamineMode.h
	ShaderReload.h
	${INNOVATOR_HEADER_FILES})

target_compile_definitions(${TARGET} PRIVATE
	VK_NO_PROTOTYPES
	VK_ENABLE_BETA_EXTENSIONS
	GLM_FORCE_LEFT_HANDED
	GLM_FORCE_DEPTH_ZERO_TO_ONE)

set_target_properties(${TARGET} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
	target_compile_definitions(${TARGET} PRIVATE VK_USE_PLATFORM_WIN32_KHR NOMINMAX)
else()
	target_compile_definitions(${TARGET} PRIVATE VK_USE_PLATFORM_XCB_KHR)
endif(WIN32)

target_link_libraries(${TARGET}
	${Boost_LIBRARIES}
	glfw
	avutil
	avformat
	avcodec
	Threads::Threads
	shaderc_shared
	${CMAKE_DL_LIBS})
