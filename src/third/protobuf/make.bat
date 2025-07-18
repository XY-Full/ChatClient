@echo off
setlocal enabledelayedexpansion

:: Set source directory, output directory and protoc path
set "SRC_DIR=.\proto"
set "OUT_DIR=.\proto_files"
set "PROTOC=.\bin\protoc3.exe"

:: Check if source directory exists
if not exist "%SRC_DIR%" (
    echo Error: Source directory %SRC_DIR% does not exist.
    exit /b 1
)

:: Create output directory (if it doesn't exist)
if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

:: Check if protoc exists
if not exist "%PROTOC%" (
    echo Error: protoc3.exe does not exist in the .\bin directory.
    exit /b 1
)

:: Loop through all .proto files and compile
echo Starting compilation of .proto files...
set "found_files=0"
for %%F in ("%SRC_DIR%\*.proto") do (
    set /a "found_files+=1"
    echo Compiling: %%~nxF
    "%PROTOC%" --proto_path="%SRC_DIR%" --cpp_out="%OUT_DIR%" "%%F"
    if !errorlevel! neq 0 (
        echo Error: Failed to compile %%~nxF.
    ) else (
        echo Successfully compiled %%~nxF.
    )
)

:: Check if files were found and compiled
if %found_files% equ 0 (
    echo Warning: No .proto files found in %SRC_DIR%.
) else (
    echo Compilation complete. Processed %found_files% file(s^).
)

endlocal