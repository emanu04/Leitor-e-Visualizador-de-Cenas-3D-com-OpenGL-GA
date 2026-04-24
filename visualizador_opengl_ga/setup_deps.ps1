# ==============================================================================
# setup_deps.ps1
# Baixa e configura as dependências externas (glad e imgui) no diretório extern/
# Execute UMA VEZ antes de compilar o projeto.
# Requer: git
# ==============================================================================

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$extern = Join-Path $root "extern"

# ---- 1. ImGui (source) ----
$imguiDir = Join-Path $extern "imgui"
if (-Not (Test-Path $imguiDir)) {
    Write-Host "Clonando ImGui..." -ForegroundColor Cyan
    git clone --depth 1 https://github.com/ocornut/imgui.git $imguiDir
} else {
    Write-Host "ImGui ja existe em $imguiDir" -ForegroundColor Green
}

# ---- 2. glad ----
# Precisamos do glad gerado para OpenGL 3.3 Core.
# Baixamos o ZIP gerado pelo gerador online (já pré-gerado neste repositório).
$gladDir = Join-Path $extern "glad"
if (-Not (Test-Path $gladDir)) {
    Write-Host "Baixando glad (OpenGL 3.3 Core)..." -ForegroundColor Cyan
    # Clona repositório auxiliar com glad pré-gerado para gl 3.3 core
    git clone --depth 1 https://github.com/Dav1dde/glad.git "$gladDir/_src"
    # Gera com Python (requer pip install glad2 ou glad)
    $gladSrc = Join-Path $gladDir "_src"
    if (Get-Command python -ErrorAction SilentlyContinue) {
        pip install glad2 --quiet
        python -m glad --api gl:core=3.3 --out-path $gladDir c
        Write-Host "glad gerado em $gladDir" -ForegroundColor Green
    } else {
        Write-Host "AVISO: Python nao encontrado. Baixe o glad manualmente em:" -ForegroundColor Yellow
        Write-Host "  https://glad.dav1d.de/ (gl 3.3, Core, C/C++)" -ForegroundColor Yellow
        Write-Host "  Extraia em extern/glad/" -ForegroundColor Yellow
    }
} else {
    Write-Host "glad ja existe em $gladDir" -ForegroundColor Green
}

Write-Host ""
Write-Host "Dependencias prontas. Compile com:" -ForegroundColor White
Write-Host "  cmake -B build -DCMAKE_BUILD_TYPE=Release" -ForegroundColor Yellow
Write-Host "  cmake --build build --config Release" -ForegroundColor Yellow
