cmake_minimum_required (VERSION 3.8)

if (POLICY CMP0141)
	cmake_policy(SET CMP0141 NEW)
	set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("NeLobbyAutoKit")

if(MSVC)
	add_compile_options(/utf-8)
	add_compile_options(/EHsc)
endif()

if(WIN32)
	# set(OPENSSL_ROOT_DIR "${CMAKE_SOURCE_DIR}/OpenSSL-Win64")
	include_directories("${CMAKE_SOURCE_DIR}/libs/OpenSSL-Win64/include")
	link_directories("${CMAKE_SOURCE_DIR}/libs/OpenSSL-Win64/lib/VC/x64/MT")
endif()

# 启用 OpenSSL 支持
# add_definitions(-DCPPHTTPLIB_OPENSSL_SUPPORT)
# 设置 CMake 静态链接 OpenSSL
# set(OPENSSL_USE_STATIC_LIBS TRUE)

# miniz 仅压缩
set(MINIZ_ENABLE_ZIP_READER OFF CACHE BOOL "" FORCE)
set(MINIZ_ENABLE_ZIP_WRITER OFF CACHE BOOL "" FORCE)
add_subdirectory("libs/miniz")

add_executable (NeLobbyAutoKit "main.cpp" "libs/Utils/EnvEncode.cpp" "libs/Utils/EnvEncode.h" "libs/Utils/httplib.h" "libs/Utils/json.hpp" "NeAutoKit.hpp")
# target_link_libraries(NeLobbyAutoKit OpenSSL::SSL OpenSSL::Crypto)
target_link_libraries(NeLobbyAutoKit libssl_static libcrypto_static)
target_link_libraries(NeLobbyAutoKit miniz)

set_property(TARGET NeLobbyAutoKit PROPERTY CXX_STANDARD 20)
