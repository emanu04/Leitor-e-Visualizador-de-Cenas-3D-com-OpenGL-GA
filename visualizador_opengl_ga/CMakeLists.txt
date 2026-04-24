cmake_minimum_required(VERSION 3.16)
project(VisualizadorCena3D)

set(CMAKE_CXX_STANDARD 17)

# ---- Pacotes ----
find_package(OpenGL REQUIRED)
find_package(glfw3   REQUIRED)
find_package(assimp  REQUIRED)

# glad (incluso na pasta extern/)
add_library(glad STATIC extern/glad/src/glad.c)
target_include_directories(glad PUBLIC extern/glad/include)

# imgui (incluso na pasta extern/imgui/)
set(IMGUI_DIR extern/imgui)
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
target_link_libraries(imgui glfw)

# ---- Executável principal ----
add_executable(visualizador
    src/main.cpp
)

target_include_directories(visualizador PRIVATE
    src/
    extern/glad/include
    extern/imgui
    extern/imgui/backends
    ${ASSIMP_INCLUDE_DIRS}
)

target_link_libraries(visualizador
    glad
    imgui
    glfw
    OpenGL::GL
    assimp::assimp
)

# Copiar shaders para o diretório de build
file(COPY ${CMAKE_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR})
