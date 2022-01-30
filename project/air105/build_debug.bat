::编译前准备工作
@echo off
setlocal enabledelayedexpansion
set line=0
for /f  %%a in ('wmic cpu get numberofcores') do (
set /a line+=1
if !line!==2 set A=%%a
)
set /a CPU_LINE=%A%+%A%

if "%1" == "" (
    set make_target=app
) else (
    set make_target=%1
)
set CROSS_TOOL_WIN32_PATH=D:\gcc-mcu
set CROSS_TOOL_PATH=%CROSS_TOOL_WIN32_PATH:\=/%

set PATH=%CROSS_TOOL_WIN32_PATH%\bin;%PATH%
REM debug属性标记
set __DEBUG__=true
REM 获取项目路径
set PROJ_DIR=%cd:\=/%
set CUR_DIR=%cd%
REM 获取项目名称
for /f %%i in ("%cd%") do set PROJ_NAME=%%~ni
REM 获取代码根目录
cd ../..
set ROOT_PATH=%cd:\=/%
set TOP_PATH=%cd%
set BUILD_PATH=%TOP_PATH%\build\%PROJ_NAME%\debug
set HEX_PATH=%TOP_PATH%\hex\%PROJ_NAME%\debug
if not exist %HEX_PATH% (mkdir %HEX_PATH%)
if not exist %BUILD_PATH% (mkdir %BUILD_PATH%)
REM 进入第一个makefile的目录project
cd %CUR_DIR%/../../compilation
set BUILD_PATH=%TOP_PATH%\build\%PROJ_NAME%\debug
if exist %BUILD_PATH%\build_error.log (
	del %BUILD_PATH%\build_error.log
)
make %make_target% 2>%BUILD_PATH%\build_error.log
if %errorlevel% == 0 (
    @echo build ok
) else (
    type %BUILD_PATH%\build_error.log
)

REM make all

if not exist %BUILD_PATH%\bootloader.hex (
    echo bootloader.hex file not exist
    goto end
)
if not exist %BUILD_PATH%\app.hex (
    echo app.hex file not exist
    goto end
)
if exist %BUILD_PATH%\all.hex (
    del %BUILD_PATH%\all.hex

)

for /f "delims=" %%i in (%BUILD_PATH%\bootloader.hex) do (
    if "%%i"==":00000001FF" (
        goto add_bl_done
    ) else (
        echo %%i>>%BUILD_PATH%\all.hex
    )
)
:add_bl_done
type %BUILD_PATH%\app.hex >> %BUILD_PATH%\all.hex
copy %BUILD_PATH%\bootloader.bin %HEX_PATH%\bootloader.bin
copy %BUILD_PATH%\app.bin %HEX_PATH%\app.bin
copy %CUR_DIR%\info.json %HEX_PATH%\info.json
copy %CUR_DIR%\soc_download.exe %HEX_PATH%\soc_download.exe
:end

::回到当前项目目录
cd %CUR_DIR%
del %HEX_PATH%\LuatOS-SoC_%PROJ_NAME%.soc
7za.exe a "%HEX_PATH%\LuatOS-SoC_%PROJ_NAME%.soc" "%HEX_PATH%\*.*"
pause