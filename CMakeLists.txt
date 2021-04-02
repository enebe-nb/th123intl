cmake_minimum_required(VERSION 3.16)

string(TIMESTAMP THI_VER_DAY "%m%d" UTC)
string(TIMESTAMP THI_VER_YEAR "%Y" UTC)
project(th123intl
	VERSION 0.1.${THI_VER_YEAR}.${THI_VER_DAY})
set(THI_VERSION ${PROJECT_VERSION})
string(REPLACE . , THI_VERSION_COMMA ${THI_VERSION})

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${PROJECT_SOURCE_DIR}/thirdparty/json/include)
add_library(th123intl MODULE
    src/main.cpp
    src/hooks.cpp
    src/language.cpp
)
target_link_libraries(th123intl -static)
set_target_properties(th123intl PROPERTIES PREFIX "")

file(GLOB PACK_STRKEYS_SRC ${CMAKE_CURRENT_SOURCE_DIR}/pack/strkeys/*)
set(PACK_STRKEYS_TARGET "${CMAKE_CURRENT_BINARY_DIR}/strkeys.dat")
add_custom_command(OUTPUT ${PACK_STRKEYS_TARGET}
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/bin/shady-cli.exe pack -m data -f default
        -o ${PACK_STRKEYS_TARGET}
        "${CMAKE_CURRENT_SOURCE_DIR}/pack/strkeys"
    DEPENDS ${PACK_STRKEYS_SRC}
    VERBATIM
)

file(GLOB PACK_ENIMG_SRC ${CMAKE_CURRENT_SOURCE_DIR}/pack/en/*)
set(PACK_ENIMG_TARGET "${CMAKE_CURRENT_BINARY_DIR}/locale/en.dat")
add_custom_command(OUTPUT ${PACK_ENIMG_TARGET}
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/bin/shady-cli.exe pack -m data -f default
        -o ${PACK_ENIMG_TARGET}
        "${CMAKE_CURRENT_SOURCE_DIR}/pack/en"
    DEPENDS ${PACK_ENIMG_SRC}
    VERBATIM
)

add_custom_target(generate-pack DEPENDS ${PACK_STRKEYS_TARGET} ${PACK_ENIMG_TARGET})
add_dependencies(th123intl generate-pack)

set(BASE_INSTALL_PATH SWRSToys/modules/th123intl)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/locale)
install(TARGETS th123intl LIBRARY DESTINATION ${BASE_INSTALL_PATH})
install(FILES
    ${PACK_STRKEYS_TARGET}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MonoSpatialModSWR.ttf
    ${CMAKE_CURRENT_SOURCE_DIR}/src/th123intl.ini
    DESTINATION ${BASE_INSTALL_PATH})
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/locale/en.dat
    ${CMAKE_CURRENT_SOURCE_DIR}/locale/en.json
    DESTINATION ${BASE_INSTALL_PATH}/locale)
