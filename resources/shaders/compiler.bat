@echo off
echo Current directory: %cd%

set /p shaderName=Shader Name:
echo Input is: %shaderName%

set /p shaderType=V/F shader?:

if /i "%shaderType%"=="V" (
    echo Vertex Shader
    glslc %shaderName% -o vert.spv
) else (
    echo Frag Shader
    glslc %shaderName% -o frag.spv
)

echo End
pause