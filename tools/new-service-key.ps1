<#
.SYNOPSIS
    Creates the manufacturer service login for the Water Tanks Monitor firmware (woodyouloveit.com).

.DESCRIPTION
    Writes water_level_indicator\service_secret.h with a random salt and SHA256(salt + password).
    Firmware built with that file has a "Service" login (needed for firmware updates from the app or dashboard).
    The password itself is never written into the project; keep it in a password manager.
    The file is listed in .gitignore: do not commit or share it.

    Without the file, the firmware has no service login at all (there is no default password).

.PARAMETER Password
    Use this password instead of a generated one (at least 12 characters).
.PARAMETER Force
    Replace an existing service key. Devices keep the key of the firmware they run until they are updated.
.PARAMETER Show
    Show the password saved by this script on this Windows account.

.EXAMPLE
    .\tools\new-service-key.ps1
.NOTES
    Copyright (C) 2026 Chanchal Sakarde - woodyouloveit.com
    SPDX-License-Identifier: GPL-3.0-or-later
#>
[CmdletBinding()]
param(
    [string]$Password,
    [switch]$Force,
    [switch]$Show
)
Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

$Repo   = Split-Path -Parent $PSScriptRoot
$Header = [IO.Path]::Combine($Repo, 'water_level_indicator', 'service_secret.h')
$OnWin  = [Environment]::OSVersion.Platform -eq [PlatformID]::Win32NT
$Store  = [IO.Path]::Combine([Environment]::GetFolderPath('UserProfile'), '.woodyouloveit', 'wtms-service.dat')

function Write-Ok([string]$m)   { Write-Host "[ok] $m" -ForegroundColor Green }
function Write-Warn2([string]$m){ Write-Host "[!]  $m" -ForegroundColor Yellow }

if ($Show) {
    if (-not $OnWin -or -not (Test-Path $Store)) { throw "No saved service password on this account ($Store)." }
    $sec = Get-Content -Raw $Store | ConvertTo-SecureString
    $plain = [Runtime.InteropServices.Marshal]::PtrToStringBSTR([Runtime.InteropServices.Marshal]::SecureStringToBSTR($sec))
    Write-Host "Service password: $plain"
    exit 0
}

if ((Test-Path $Header) -and -not $Force) {
    throw "A service key already exists ($Header). Use -Force to replace it: tanks keep the old key until they get firmware built with the new one."
}

$rng = [Security.Cryptography.RandomNumberGenerator]::Create()
function New-RandomBytes([int]$n) { $b = New-Object byte[] $n; $rng.GetBytes($b); return $b }

if (-not $Password) {
    # 20 characters without look-alikes (no 0/O, 1/l/I): about 115 bits
    $chars = 'abcdefghijkmnpqrstuvwxyzABCDEFGHJKLMNPQRSTUVWXYZ23456789'
    $Password = -join ((New-RandomBytes 20) | ForEach-Object { $chars[$_ % $chars.Length] })
}
if ($Password.Length -lt 12) { throw 'The service password needs at least 12 characters.' }

$salt = -join ((New-RandomBytes 16) | ForEach-Object { $_.ToString('x2') })
$sha  = [Security.Cryptography.SHA256]::Create()
$hash = -join ($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($salt + $Password)) | ForEach-Object { $_.ToString('x2') })

$lines = @(
    '// Manufacturer service login for the Water Tanks Monitor firmware (woodyouloveit.com).',
    "// Made by tools/new-service-key.ps1 on $(Get-Date -Format 'yyyy-MM-dd'). DO NOT COMMIT OR SHARE THIS FILE.",
    '// Only a salted SHA-256 hash of the password is stored here.',
    '#pragma once',
    "#define WTMS_SERVICE_SALT `"$salt`"",
    "#define WTMS_SERVICE_HASH `"$hash`""
)
[IO.File]::WriteAllText($Header, ($lines -join "`r`n") + "`r`n", (New-Object Text.UTF8Encoding($false)))
Write-Ok "Service key written: $Header"

if ($OnWin) {
    New-Item -ItemType Directory -Force -Path (Split-Path $Store) | Out-Null
    ConvertTo-SecureString $Password -AsPlainText -Force | ConvertFrom-SecureString | Set-Content -Path $Store
    Write-Ok "Password saved for this Windows account only (show it again with -Show)."
}
Write-Host ''
Write-Host "  Service password:  $Password" -ForegroundColor White
Write-Host ''
Write-Warn2 'Store it in your password manager now. Anyone with it can update the firmware of your tanks.'
Write-Host 'Next: build the firmware (Arduino IDE: Sketch > Export Compiled Binary) and copy the .bin into firmware\.'
