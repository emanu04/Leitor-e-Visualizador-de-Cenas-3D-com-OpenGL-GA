cmake_minimum_required(VERSION 3.10)
project(VisualizadorCena3D)

# Define o padrão do C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Ativa o FetchContent
include(FetchContent)

# Configura o FetchContent para baixar e compilar GLFW automaticamente
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4
)

# Configura o FetchContent para baixar a GLM automaticamente
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG master
)

# Faz o download e compila as bibliotecas
FetchContent_MakeAvailable(glfw glm)

# Configura o FetchContent para baixar a stb_image automaticamente
FetchContent_Declare(
  stb_image
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG master
)

FetchContent_MakeAvailable(stb_image)

# Configura o FetchContent para baixar Assimp automaticamente
FetchContent_Declare(
  assimp
  GIT_REPOSITORY https://github.com/assimp/assimp.git
  GIT_TAG 5.3
)
FetchContent_MakeAvailable(assimp)

# Configura o FetchContent para baixar Dear ImGui automaticamente
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG master
)
FetchContent_MakeAvailable(imgui)

# Adiciona as pastas de cabeçalhos
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/common)
include_directories(${CMAKE_SOURCE_DIR}/include/glad)
include_directories(${glm_SOURCE_DIR})
include_directories(${stb_image_SOURCE_DIR})
include_directories(${imgui_SOURCE_DIR})
include_directories(${imgui_SOURCE_DIR}/backends)
include_directories(${assimp_SOURCE_DIR}/include)

add_compile_options(-Wno-pragmas)

# Define as bibliotecas para cada sistema operacional
if(WIN32)
    set(OPENGL_LIBS opengl32)
elseif(APPLE)
    find_library(OpenGL_LIBRARY OpenGL)
    set(OPENGL_LIBS ${OpenGL_LIBRARY})
else()
    find_package(OpenGL REQUIRED)
    set(OPENGL_LIBS ${OPENGL_gl_LIBRARY})
endif()

# Caminho esperado para a GLAD
set(GLAD_C_FILE "${CMAKE_SOURCE_DIR}/common/glad.c")

# Verifica se os arquivos da GLAD estão no lugar
if (NOT EXISTS ${GLAD_C_FILE})
    message(FATAL_ERROR "Arquivo glad.c não encontrado! Baixe a GLAD manualmente em [https://glad.dav1d.de/](https://glad.dav1d.de/) e coloque glad.h em include/glad/ e glad.c em common/")
endif()

# Cria biblioteca ImGui
set(IMGUI_DIR ${imgui_SOURCE_DIR})
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
    ${GLAD_C_FILE}
)

# Configura os includes e linka as bibliotecas
target_include_directories(visualizador PRIVATE 
    ${CMAKE_SOURCE_DIR}/include/glad 
    ${glm_SOURCE_DIR} 
    ${stb_image_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(visualizador 
    glfw 
    ${OPENGL_LIBS}
    assimp::assimp
    imgui
)
