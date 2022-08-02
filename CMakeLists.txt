cmake_minimum_required(VERSION 3.16)

string(TIMESTAMP THI_VER_DAY "%m%d" UTC)
string(TIMESTAMP THI_VER_YEAR "%Y" UTC)
project(th123intl
    VERSION 0.2.${THI_VER_YEAR}.${THI_VER_DAY}
    DESCRIPTION "Internationalization tool for a specific fighting game.")
set(THI_VERSION ${PROJECT_VERSION})
string(REPLACE . , THI_VERSION_COMMA ${THI_VERSION})

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BASE_INSTALL_PATH th123intl)

macro(add_resourcepack target source)
    file(GLOB _PACK_SRC_${source} ${CMAKE_CURRENT_SOURCE_DIR}/pack/${source}/*)
    add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/pack/${source}.dat"
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/bin/shady-cli.exe pack -m data
            -o "${CMAKE_CURRENT_BINARY_DIR}/pack/${source}.dat"
            "${CMAKE_CURRENT_SOURCE_DIR}/pack/${source}"
        DEPENDS ${_PACK_SRC_${source}}
        VERBATIM
    )
    add_custom_target(${target}-${source} DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/pack/${source}.dat")
    add_dependencies(${target} ${target}-${source})
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pack/${source}.dat DESTINATION ${BASE_INSTALL_PATH}/pack)
endmacro()

include_directories(${PROJECT_SOURCE_DIR}/thirdparty/json/include)
add_subdirectory(${PROJECT_SOURCE_DIR}/thirdparty/SokuLib)
add_library(th123intl MODULE
    src/main.cpp
    src/hooks.cpp
    src/language.cpp
    src/parser.cpp
)
target_link_libraries(th123intl SokuLib)
set_target_properties(th123intl PROPERTIES PREFIX "")
add_custom_target(generate-pack)
add_resourcepack(generate-pack empty-csv)
add_resourcepack(generate-pack jp)
add_resourcepack(generate-pack en)
add_resourcepack(generate-pack zh)
add_resourcepack(generate-pack pt-BR)
add_resourcepack(generate-pack tr)
add_dependencies(th123intl generate-pack)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/locale)
install(TARGETS th123intl LIBRARY DESTINATION ${BASE_INSTALL_PATH})
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/font
    DESTINATION ${BASE_INSTALL_PATH}
    FILES_MATCHING PATTERN "*.ttf")
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/locale
    DESTINATION ${BASE_INSTALL_PATH}
    FILES_MATCHING PATTERN "*.json")
