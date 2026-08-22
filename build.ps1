if (-not (Test-Path "include\registries.h")) {
    Write-Error "Error: 'include\registries.h' is missing."
    Write-Host "Please follow the 'Compilation' section of the README to generate it."
    exit 1
}

if (-not (Test-Path "obj")) {
    New-Item -ItemType Directory -Path "obj" | Out-Null
}

cl src\*.c /I include /Fe: bareiron.exe /Fo: "obj\\" /O2 /link ws2_32.lib