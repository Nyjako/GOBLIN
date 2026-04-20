# Written by clanker

$ErrorActionPreference = 'Stop'

$rootDir  = $PSScriptRoot
$testDir  = Join-Path $rootDir 'tests'
$buildDir = Join-Path $rootDir 'build\tests'

New-Item -ItemType Directory -Force -Path $buildDir | Out-Null

Get-ChildItem -Path $testDir -Filter *.c -File | ForEach-Object {
    $base = $_.BaseName
    $name = $base
    $std  = 'c99'

    if ($name -match '\.c([0-9]+)$') {
        $std  = "c$($Matches[1])"
        $name = $name -replace '\.c[0-9]+$', ''
    }

    $exe = Join-Path $buildDir ($name + '.exe')

    Write-Host "==> Building $name (std=$std)"
    $extraFlags = if ($std -eq 'c11') { '/experimental:c11atomics' } else { '' }
    & cl /nologo /W4 /TC "/std:$std" $extraFlags "/I$rootDir" $_.FullName "/Fe:$exe"
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed for $name"
    }

    Write-Host "==> Running $name"
    & $exe
    if ($LASTEXITCODE -ne 0) {
        throw "Test failed for $name"
    }
}

Write-Host "All tests passed."
