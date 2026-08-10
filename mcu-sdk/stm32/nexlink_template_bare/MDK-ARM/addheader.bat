@echo off

set PROJ_PATH=%~1
set AXF_NAME=%~2
set BIN_PATH=%~3

:: 时间戳
set "year=%date:~2,2%"
set "month=%date:~5,2%"
set "day=%date:~8,2%"
set "hour_ten=%time:~0,1%"
set "hour_one=%time:~1,1%"
set "minute=%time:~3,2%"
set "second=%time:~6,2%"

:: 去掉时间里的空格
set HH=%HH: =0%

:: 路径定义
set HEADER_FILE=%AXF_NAME%\appheader.bin
set OUT_DIR=%PROJ_PATH%output
set OUT_FILE=%OUT_DIR%\%AXF_NAME%_merge_20%year%%month%%day%.bin

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

echo appheader v2.02 t%year%%month%%day%%hour_ten%%hour_one%%minute%%second% > %HEADER_FILE%

copy /b "%HEADER_FILE%" + "%BIN_PATH%" "%OUT_FILE%"

del "%HEADER_FILE%"

echo [OK] %OUT_FILE%
