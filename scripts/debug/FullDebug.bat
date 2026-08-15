cd /d "%~dp0..\..\"

cmake -B build -A x64 -DCMAKE_TOOLCHAIN_FILE=C:\dev\vcpkg\scripts\buildsystems\vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static

cmake --build build --config Debug

cd ..\..\build\Debug

.\inject.exe