#Requires -Version 5.1
$ErrorActionPreference = "Stop"

$REQUIRED_MAJOR = 21
$SERVER_JAR = $env:SERVER_JAR
if ([string]::IsNullOrEmpty($SERVER_JAR)) {
    $SERVER_JAR = "server.jar"
}
$NOTCHIAN_DIR = "notchian"
$global:JS_RUNTIME = ""

function Get-JavaVersion {
    $versionOutput = cmd /c "java -version 2>&1" | Out-String
    if ($versionOutput -match 'version "?(\d+)\.') {
        return [int]$Matches[1]
    } elseif ($versionOutput -match 'openjdk version "?(\d+)\.') {
        return [int]$Matches[1]
    } else {
        throw "Could not detect Java version from output:`n$versionOutput"
    }
}


function Check-Java {
    if (-not (Get-Command java -ErrorAction SilentlyContinue)) {
        Write-Host "Java not found in PATH."
        exit 1
    }

    $major = Get-JavaVersion
    if ($major -lt $REQUIRED_MAJOR) {
        Write-Host "Java $REQUIRED_MAJOR or newer required, but found Java $major."
        exit 1
    }
}

function Prepare-NotchianDir {
    if (-not (Test-Path $NOTCHIAN_DIR)) {
        Write-Host "Creating $NOTCHIAN_DIR directory..."
        New-Item -ItemType Directory -Path $NOTCHIAN_DIR | Out-Null
    }
    Set-Location $NOTCHIAN_DIR
}

function Dump-Registries {
    if (-not (Test-Path $SERVER_JAR)) {
        Write-Host "No $SERVER_JAR found."
        Write-Host "Please download the server.jar from https://www.minecraft.net/en-us/download/server"
        Write-Host "and place it in the notchian directory."
        exit 1
    }
    & java -DbundlerMainClass="net.minecraft.data.Main" -jar $SERVER_JAR --all
}

function Detect-JSRuntime {
    if (Get-Command node -ErrorAction SilentlyContinue) {
        $global:JS_RUNTIME = "node"
    } elseif (Get-Command bun -ErrorAction SilentlyContinue) {
        $global:JS_RUNTIME = "bun"
    } elseif (Get-Command deno -ErrorAction SilentlyContinue) {
        $global:JS_RUNTIME = "deno run"
    } else {
        Write-Host "No JavaScript runtime found (Node.js, Bun, or Deno)."
        exit 1
    }
}

function Run-JSScript {
    param([string]$script)
    if ([string]::IsNullOrEmpty($global:JS_RUNTIME)) {
        Detect-JSRuntime
    }
    Write-Host "Running $script with $global:JS_RUNTIME..."
    & $global:JS_RUNTIME $script
}

# Main execution
Check-Java
Prepare-NotchianDir
Dump-Registries
Run-JSScript "../build_registries.js"
Write-Host "Registry dump and processing complete."
