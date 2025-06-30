cmake_minimum_required(VERSION 3.16)
project(YouTubeLinkCopier)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Debug bilgisi korunarak hafif optimizasyon
set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")

add_executable(YouTubeLinkCopier main.cpp)

# Windows kütüphaneleri
if(WIN32)
    target_link_libraries(YouTubeLinkCopier user32 kernel32 shell32)
    set_target_properties(YouTubeLinkCopier PROPERTIES WIN32_EXECUTABLE TRUE)
endif() 