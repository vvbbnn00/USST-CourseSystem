cmake_minimum_required(VERSION 3.22)

project(CourseSystem VERSION 1.0.204)
set(CMAKE_C_STANDARD 11)

set(SocketCore_Version_Major 0)
set(SocketCore_Version_Minor 1)

# 引入第三方库，该处使用的库如下（若因系统不兼容代码自带库而编译失败，需要下载源代码自行编译）：
# pcre2 https://pcre2project.github.io/pcre2/
include_directories("./cmake-libraries/include")
link_directories("./cmake-libraries/libs")

# 引入代码所关联的全部文件
include_directories(libs
        common
        services
        utils)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/meta.h.in meta.h)
set(VERSIONINFO_RC "${CMAKE_CURRENT_SOURCE_DIR}/VersionInfo.rc")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/include/version.rc.in" "${VERSIONINFO_RC}")

# 添加可执行文件
add_executable(CourseSystem
        ${VERSIONINFO_RC}
        libs/cJSON.c
        libs/cJSON.h
        libs/hmacsha256.c
        libs/hmacsha256.h
        libs/AES.c
        libs/AES.h
        utils/sysinfo.c
        utils/sysinfo.h
        utils/socket.c
        utils/socket.h
        utils/string_ext.c
        utils/string_ext.h
        common/global.c
        common/global.h
        common/simple_string_hash_list_obj.c
        common/simple_string_hash_list_obj.h
        common/utf8_words.h
        common/link_list_object.c
        common/link_list_object.h
        services/user.c
        services/user.h
        services/ui_utf8.c
        services/ui_utf8.h
        services/course.c
        services/course.h
        services/ui_gbk.c
        services/ui_gbk.h
        services/system_config.c
        services/system_config.h
        meta.h
        main.c)



# 为指定项目添加 include 路径
target_include_directories(CourseSystem PUBLIC "${PROJECT_BINARY_DIR}")
target_link_libraries(CourseSystem pcre2-8)

if (WIN32)
    target_link_libraries(CourseSystem wsock32 ws2_32)
endif ()
