cmake_minimum_required(VERSION 3.22.1)
project("xtra_custom_binary")

# 1. Compile strictly as a true executable
add_executable(xtra-hand-start xtra-hand-start.c)
target_compile_options(xtra-hand-start PRIVATE -g3 -Wall -Wextra)
set_property(TARGET xtra-hand-start PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(xtra-hand-start dl log)

# 2. Add an automatic post-build hook to copy it directly into your project's assets
add_custom_command(TARGET xtra-hand-start POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/../jniLibs/${ANDROID_ABI}"
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:xtra-hand-start> "${CMAKE_SOURCE_DIR}/../jniLibs/${ANDROID_ABI}/libxtra-hand-start.so"
        COMMENT "Copying executable binary to jniLibs folder as a fake .so asset"
)
