# Visualizador de Cenas 3D — Pipeline Gráfico Programável

Trabalho prático de Computação Gráfica.  
Implementa um leitor e visualizador de cenas 3D com iluminação de Phong, controle de câmera FPS, transformações geométricas interativas e suporte a múltiplos objetos.

---

## Integrantes

Emanuele Schlemmer Thomazzoni
Ana Beatriz Stahl

---

## Funcionalidades

- **Parser de geometria** via [Assimp](https://github.com/assimp/assimp): lê `.obj`, `.ply` e outros formatos, extraindo vértices, faces (triângulos), normais (`vn`) e coordenadas de textura (`vt`).
- **VAO / VBO / EBO** corretamente configurados com `glVertexAttribPointer` compatível com o Vertex Shader.
- **Múltiplos objetos** na cena com seleção por teclado (teclas `1`–`9`).
- **Transformações geométricas** (translação, rotação em X/Y/Z, escala uniforme e por eixos) para o objeto selecionado.
- **Câmera FPS** com teclado (WASD) e mouse; troca entre **projeção perspectiva e ortográfica** (tecla `P`).
- **Modelo de iluminação de Phong** (componentes Ambiente, Difusa e Especular) implementado nos shaders GLSL.
- **Fonte de luz pontual** configurável (posição e cor).
- **Propriedades de material** (`ka`, `kd`, `ks`, brilho) configuráveis em tempo real via interface.
- **Modo wireframe** sobreposto à geometria sólida (tecla `V`).
- **Grid de chão** e **eixos XYZ** (extras opcionais).
- **Interface ImGui** para ajuste de todos os parâmetros sem recompilar.

---

## Dependências

| Biblioteca | Versão mínima | Instalação |
|---|---|---|
| OpenGL | 3.3 Core | Driver da GPU |
| [GLFW](https://www.glfw.org/) | 3.3 | vcpkg / apt / brew |
| [GLM](https://github.com/g-truc/glm) | 0.9.9 | vcpkg / apt / brew |
| [Assimp](https://github.com/assimp/assimp) | 5.x | vcpkg / apt / brew |
| [glad](https://glad.dav1d.de/) | OpenGL 3.3 Core | gerado (ver abaixo) |
| [Dear ImGui](https://github.com/ocornut/imgui) | 1.90 | clonado em `extern/` |
| CMake | 3.16+ | cmake.org |

---

## Compilação

### Pré-requisitos comuns

- CMake ≥ 3.16
- Compilador com suporte a C++17 (GCC 10+, Clang 12+, MSVC 2019+)
- Git

---

### Windows (vcpkg + Visual Studio / MinGW)

#### 1. Instalar dependências via vcpkg

```powershell
# Instala vcpkg 
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat

# Instala as bibliotecas
C:\vcpkg\vcpkg install glfw3 glm assimp --triplet x64-windows
```

#### 2. Configurar glad e ImGui

```powershell
# Na pasta do projeto:
.\setup_deps.ps1
```

> Se o script não conseguir gerar o glad automaticamente, acesse  
> <https://glad.dav1d.de/>, selecione **Language: C/C++**, **API gl: Version 3.3**, **Profile: Core** e baixe o ZIP.  
> Extraia o conteúdo em `extern/glad/` (deve conter `include/` e `src/`).

#### 3. Compilar

```powershell
cmake -B build -DCMAKE_BUILD_TYPE=Release `
      -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

O executável estará em `build/Release/visualizador.exe`.

---

### Linux (apt)

```bash
sudo apt update
sudo apt install cmake libglfw3-dev libglm-dev libassimp-dev python3-pip

# Gera glad
pip install glad2
python -m glad --api gl:core=3.3 --out-path extern/glad c

# Clona ImGui
git clone --depth 1 https://github.com/ocornut/imgui.git extern/imgui

# Compila
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

O executável estará em `build/visualizador`.

---

### macOS (Homebrew)

```bash
brew install cmake glfw glm assimp python

pip3 install glad2
python3 -m glad --api gl:core=3.3 --out-path extern/glad c

git clone --depth 1 https://github.com/ocornut/imgui.git extern/imgui

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(sysctl -n hw.logicalcpu)
```

---

## Exemplo de Uso

```bash
# Carregar um modelo
./visualizador modelos/teapot.obj

# Carregar múltiplos modelos
./visualizador modelos/cube.obj modelos/teapot.obj modelos/bunny.ply
```

> Os arquivos de shader (`shaders/`) são copiados automaticamente para o diretório de build pelo CMake.

---

## Controles

### Câmera
| Tecla | Ação |
|---|---|
| `W / S` | Avança / recua |
| `A / D` | Lateral esquerda / direita |
| `Q / E` | Desce / sobe |
| Mouse | Rotaciona a câmera |
| Scroll | Zoom (altera FOV) |
| `Tab` | Alterna captura do mouse (libera para ImGui) |
| `P` | Alterna projeção perspectiva ↔ ortográfica |

### Objeto Selecionado
| Tecla | Ação |
|---|---|
| `1` – `9` | Seleciona objeto pelo índice |
| `I / K` | Translação +Y / −Y |
| `J / L` | Translação −X / +X |
| `U / O` | Translação +Z / −Z |
| `F1` (+`Shift`) | Rotação no eixo X (horário / anti-horário) |
| `F2` (+`Shift`) | Rotação no eixo Y |
| `F3` (+`Shift`) | Rotação no eixo Z |
| `NumPad +` | Escala uniforme +5% |
| `NumPad −` | Escala uniforme −5% |

### Visualização
| Tecla | Ação |
|---|---|
| `V` | Alterna wireframe sobreposto |
| `Esc` | Fecha a aplicação |

---

## Arquitetura do Código

```
src/
├── main.cpp      Loop principal, callbacks GLFW, renderização, ImGui
├── Shader.h      Compilação e linkagem de shaders, uniformes
├── Mesh.h        Estrutura Vertex, setup de VAO/VBO/EBO, draw call
├── Model.h       Parser via Assimp, hierarquia de malhas, transformações
└── Camera.h      Câmera FPS com ângulos de Euler (yaw/pitch)

shaders/
├── phong.vert    Vertex shader: transforma vértices, passa atributos
├── phong.frag    Fragment shader: cálculo de Phong (ambiente+difusa+especular)
├── wireframe.vert/frag  Shader simples de cor sólida para wireframe/eixos
└── grid.vert/frag       Shader da grid de chão com transparência
```

### Fluxo CPU → GPU

1. **CPU**: Assimp lê o arquivo e preenche `std::vector<Vertex>` com `{Position, Normal, TexCoords}` e `std::vector<unsigned int>` com índices.  
2. **`Mesh::setupMesh()`**: cria VAO, VBO e EBO; chama `glVertexAttribPointer` para mapear cada campo do `Vertex` para as `location` 0, 1 e 2 do Vertex Shader.  
3. **Vertex Shader** (`phong.vert`): multiplica posição por `model × view × projection`; transforma a normal pela inversa-transposta da matriz de modelo.  
4. **Fragment Shader** (`phong.frag`): recebe posição e normal interpoladas; calcula as três componentes de Phong usando a posição da luz e da câmera.  
5. **`glDrawElements`**: renderiza usando o EBO, descrevendo cada face triangular por três índices.

---

## Modelos para Teste

Recomendamos modelos triangularizados com normais e UVs, como os disponíveis no repositório de apoio da disciplina ou em:

- <https://github.com/alecjacobson/common-3d-test-models>
- <https://people.sc.fsu.edu/~jburkardt/data/obj/obj.html>
