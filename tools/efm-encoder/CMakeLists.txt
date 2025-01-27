# Set the target name
set(TARGET_NAME efm-encoder)

file(GLOB_RECURSE SRC_FILES ${CMAKE_SOURCE_DIR}/tools/efm-encoder/src/*.cpp)

add_executable(${TARGET_NAME} ${SRC_FILES})

# Find the Qt library
find_package(Qt5 REQUIRED COMPONENTS Core Widgets)

# Add the include directories for Qt
target_include_directories(${TARGET_NAME} PRIVATE ${Qt5Core_INCLUDE_DIRS} ${Qt5Widgets_INCLUDE_DIRS})

# Add the include directories for the EFM library
target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/../../libs/efm/include)

# Link the Qt libraries
target_link_libraries(${TARGET_NAME} PRIVATE Qt::Core)

# Link the efm library to your target
target_link_libraries(${TARGET_NAME} PRIVATE efm)

# Add the library directory for the EFM library
target_link_directories(${TARGET_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/../../libs/efm/lib)

install(TARGETS ${TARGET_NAME})