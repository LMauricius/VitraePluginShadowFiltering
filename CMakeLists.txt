cmake_minimum_required(VERSION 3.7.0)
project(VitraePluginShadowFiltering VERSION 0.1.0)

include(CTest)
enable_testing()

file(GLOB_RECURSE SrcFiles CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE HeaderFiles CONFIGURE_DEPENDS include/*.h include/*.hpp)

add_library(VitraePluginShadowFiltering SHARED ${SrcFiles} ${HeaderFiles})
target_compile_features(VitraePluginShadowFiltering PUBLIC cxx_std_20)
target_include_directories(VitraePluginShadowFiltering PUBLIC include)

target_link_libraries(VitraePluginShadowFiltering PUBLIC VitraeEngine)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
