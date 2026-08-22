@echo off
setlocal

set REQUIRED_MAJOR=21
if not defined SERVER_JAR (
    set SERVER_JAR=server.jar
)
set NOTCHIAN_DIR=notchian
set JS_RUNTIME=
set JAVA_VER=

goto:main

:get_java_version
for /F "usebackq tokens=3 delims=. " %%g in (`java -version 2^>^&1 ^| findstr /i "version"`) do set JAVA_VER=%%g
set JAVA_VER=%JAVA_VER:"=%
exit /b 0

:check_java
where java >nul 2>nul
if not %ERRORLEVEL% equ 0 (
    echo Java not found in PATH.
    exit /b 1
)

call:get_java_version

if 1%JAVA_VER% lss 1%REQUIRED_MAJOR% (
    echo Java %REQUIRED_MAJOR% or newer required, but found Java %JAVA_VER%.
    exit /b 1
)
exit /b 0

:prepare_notchian_dir
if not exist "%NOTCHIAN_DIR%\" (
    echo Creating %NOTCHIAN_DIR% directory...
    mkdir "%NOTCHIAN_DIR%"
)
exit /b 0

REM Those parantheses have to be escaped because batch parses them as an if predicate...
REM thanks, microsoft.
:dump_registries
if not exist "%SERVER_JAR%" (
    echo No server.jar found ^(looked for %SERVER_JAR%^).
	echo Please download the 1.21.8 server.jar ^(e.g. from https://mcversions.net/download/1.21.8^)
	echo and place it in the "notchian" directory.
    exit /b 1
)

java -DbundlerMainClass="net.minecraft.data.Main" -jar "%SERVER_JAR%" --all
exit /b 0

REM Gross! Batch doesn't really lend itself to cleanliness...
:detect_js_runtime
where node >nul 2>nul
if %ERRORLEVEL% equ 0 (
    set JS_RUNTIME=node
) else (
    where bun >nul 2>nul
    if %ERRORLEVEL% equ 0 (
        set JS_RUNTIME=bun
    ) else (
        where deno >nul 2>nul
        if %ERRORLEVEL% equ 0 (
            set JS_RUNTIME=deno run
        ) else (
            echo No JavaScript runtime found ^(Node.js, Bun, or Deno^).
            exit /b 1
        )
    )
)
exit /b 0

:run_js_script
set script=%~1
if "%JS_RUNTIME%"=="" (
    call:detect_js_runtime
    if not %ERRORLEVEL% equ 0 (
        exit /b 1
    )
)

echo Running %script% with %JS_RUNTIME%...
%JS_RUNTIME% %script%
exit /b 0

:main
call:check_java
if not %ERRORLEVEL% equ 0 (
    exit /b 1
)

call:prepare_notchian_dir
if not %ERRORLEVEL% equ 0 (
    exit /b 1
)

pushd "%NOTCHIAN_DIR%"
call:dump_registries
if not %ERRORLEVEL% equ 0 (
    exit /b 1
)
popd

call:run_js_script "build_registries.js"
if not %ERRORLEVEL% equ 0 (
    exit /b 1
)

echo Registry dump and processing complete.
endlocal