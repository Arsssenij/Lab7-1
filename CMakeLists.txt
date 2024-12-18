cmake_minimum_required(VERSION 3.14)

project(lab7 VERSION 0.1.0)

# Установить стандарт C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Добавляем FetchContent
include(FetchContent)

# Конфигурация для Boost
FetchContent_Declare(
  Boost
  GIT_REPOSITORY https://github.com/boostorg/boost.git
  GIT_TAG boost-1.86.0 # Укажите версию Boost, которая поддерживает вашу версию CMake
)

# Загружаем и добавляем Boost
FetchContent_MakeAvailable(Boost)

# Устанавливаем Boost в проект
find_package(Boost 1.86.0 REQUIRED COMPONENTS filesystem)

# Исполняемая цель
add_executable(lab7 main.cpp)

# Подключаем Boost
target_include_directories(lab7 PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(lab7 PRIVATE ${Boost_LIBRARIES})

# Установка исполняемого файла
install(TARGETS lab7 DESTINATION bin)

# Пакетирование
set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_NAME "lab7")
include(CPack)
