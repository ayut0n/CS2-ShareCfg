cmake_minimum_required(VERSION 3.15)
project(CS2_ShareCfg)

# Включение статической линковки для MinGW
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Линкуем необходимые системные библиотеки
link_libraries(advapi32 shell32)

# Добавляем ресурсы в сборку исполняемого файла
add_executable(${PROJECT_NAME} main.cpp ${APP_RESOURCES})