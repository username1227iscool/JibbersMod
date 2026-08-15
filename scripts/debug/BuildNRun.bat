cd /d "%~dp0..\..\"

cmake --build build --config Debug

cd ..\..\build\Debug

.\inject.exe