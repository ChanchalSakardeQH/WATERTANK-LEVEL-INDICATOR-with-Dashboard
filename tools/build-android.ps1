<#
.SYNOPSIS
    Build, sign, check and install the Water Tanks Monitor Android app on Windows.

.DESCRIPTION
    One script for the whole local Android workflow:
      - finds your Android folder (for example D:\Android with Android Studio, Sdk and Gradle) by itself
      - checks Java (17 to 24), the Android SDK and Gradle; uses Android Studio's own Java when it fits
      - Debug:   app-debug.apk, ready to install on your own phones (installs as "WTMS Debug", beside the release app)
      - Release: signed app-release.apk and app-release.aab (Google Play), signatures verified
      - copies everything to release\android\<version>\ with SHA-256 checksums and a build log
      - optionally installs on a USB-connected phone and starts the app
      - TestAab: installs the .aab on your phone the way Google Play would (bundletool)
      - sets itself up: with your permission it installs what is missing (Android SDK, Java 21)
        and fixes common build failures (licences, damaged downloads, stuck Gradle processes) automatically

    The dashboard itself comes from each tank device, so there is nothing to build first.
    Run it without parameters for a simple menu (or double-click build-android.bat).

    woodyouloveit | woodyouloveit.com | wooduloveit.com
    Copyright (C) 2026 Chanchal Sakarde

.PARAMETER Task
    Menu (default), Debug, Release, All (debug + release), Keystore (create the signing key),
    TestAab (install the release .aab on a phone with bundletool), Doctor (check the set-up), Clean.

.PARAMETER Install
    Install the built APK on the phone connected by USB (USB debugging on).

.PARAMETER Launch
    Start the app after installing.

.PARAMETER VersionName
    Override the version shown to users, e.g. 1.3.0 (otherwise android\version.properties).

.PARAMETER VersionCode
    Override the internal version number. Google Play needs a higher number for every upload.

.PARAMETER Keystore
    The release signing key (default: <Android folder>\Keys\wtms-release.jks, outside the project;
    a key made by the earlier build script in %USERPROFILE%\.woodyouloveit is used if it is there).

.PARAMETER RememberPassword
    Remember the keystore password for next time, encrypted with your Windows login (DPAPI).

.PARAMETER Yes
    Answer yes to the automatic set-up and fix questions (unattended builds). This also accepts the Android SDK licence.

.PARAMETER NoAutoFix
    Never change the set-up: report problems only.

.PARAMETER DryRun
    Show what would run without running it.

.EXAMPLE
    .\tools\build-android.ps1
    Shows the menu.

.EXAMPLE
    .\tools\build-android.ps1 -Task Debug -Install -Launch
    Builds the debug APK, installs it on the connected phone and starts it.

.EXAMPLE
    .\tools\build-android.ps1 -Task Release -VersionName 1.3.0 -VersionCode 4
    Builds the signed release APK + AAB for Google Play.
#>

[CmdletBinding()]
param(
    [ValidateSet('Menu', 'Debug', 'Release', 'All', 'Keystore', 'TestAab', 'Doctor', 'Clean')]
    [string]$Task = 'Menu',

    [switch]$Install,
    [switch]$Launch,
    [string]$VersionName = '',
    [int]$VersionCode = 0,
    [string]$Keystore = '',
    [string]$KeyAlias = '',
    [switch]$RememberPassword,
    [switch]$Open,
    [switch]$Yes,
    [switch]$NoAutoFix,
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

# ---------------------------------------------------------------- paths and constants

$OnWindows   = ($env:OS -eq 'Windows_NT')
$Repo        = Split-Path -Parent $PSScriptRoot
$AndroidDir  = Join-Path $Repo 'android'
$AppName     = 'Water Tanks Monitor'
$AppId       = 'com.woodyouloveit.wtms'
$DebugAppId  = "$AppId.debug"                 # the debug build installs as its own app, "WTMS Debug"
$FilePrefix  = 'woodyouloveit-wtms'
$Home_       = if ($OnWindows) { $env:USERPROFILE } else { $env:HOME }

# Your Android folder: the one holding Sdk plus Android Studio or Gradle (for example D:\Android),
# found by walking up from this project (D:\Android\Projects\...). SM_ANDROID_ROOT overrides it.
function Find-AndroidRoot {
    if ($env:SM_ANDROID_ROOT -and (Test-Path $env:SM_ANDROID_ROOT)) { return $env:SM_ANDROID_ROOT }
    $dir = Get-Item -LiteralPath $Repo
    while ($dir) {
        $hasSdk = Test-Path (Join-Path $dir.FullName 'Sdk')
        if ($hasSdk -and ((Test-Path (Join-Path $dir.FullName 'Android Studio')) -or (Test-Path (Join-Path $dir.FullName 'Gradle')))) { return $dir.FullName }
        $dir = $dir.Parent
    }
    if ($OnWindows -and (Test-Path 'D:\Android\Sdk')) { return 'D:\Android' }
    return $null
}
$AndroidRoot = Find-AndroidRoot

# Keys, Java and tools live in the Android folder, outside the project, so every new project version uses them
$ToolsDir    = if ($AndroidRoot) { Join-Path $AndroidRoot 'Tools' } else { Join-Path $Home_ '.woodyouloveit' }
$KeysDir     = if ($AndroidRoot) { Join-Path $AndroidRoot 'Keys' } else { Join-Path $Home_ '.woodyouloveit' }
$JdkDir      = if ($AndroidRoot) { Join-Path $AndroidRoot 'jdk-21' } else { Join-Path $ToolsDir 'jdk-21' }
$SecretFile  = Join-Path $KeysDir 'wtms-signing.dat'
$LegacySigning = Join-Path $KeysDir 'wtms-keystore.properties'             # storeFile / keyAlias / storePassword, if you keep one
$BundletoolVersion = '1.17.2'
$GradleHome  = if ($env:GRADLE_USER_HOME) { $env:GRADLE_USER_HOME }
               elseif ($AndroidRoot -and (Test-Path (Join-Path $AndroidRoot 'Gradle'))) { Join-Path $AndroidRoot 'Gradle' }
               else { Join-Path $Home_ '.gradle' }
$env:GRADLE_USER_HOME = $GradleHome

# Where the automatic set-up downloads from. (The WYLI_* variables only exist to test it against local copies.)
$SdkRepoUrl  = if ($env:WYLI_SDK_REPO) { $env:WYLI_SDK_REPO } else { 'https://dl.google.com/android/repository' }
$AdoptiumApi = if ($env:WYLI_ADOPTIUM_API) { $env:WYLI_ADOPTIUM_API } else { 'https://api.adoptium.net' }
$CmdlineToolsFallback = '15859902'        # used only if Google's package list cannot be read
# Gradle and Android versions are read from the project, so this script never drifts from it
$WrapperProps  = [IO.Path]::Combine($AndroidDir, 'gradle', 'wrapper', 'gradle-wrapper.properties')
$GradleVersion = if ((Test-Path $WrapperProps) -and ((Get-Content -Raw -Path $WrapperProps) -match 'gradle-([0-9.]+)-(bin|all)\.zip')) { $Matches[1] } else { '8.14.3' }
$AppGradle     = [IO.Path]::Combine($AndroidDir, 'app', 'build.gradle.kts')
$CompileSdk    = if ((Test-Path $AppGradle) -and ((Get-Content -Raw -Path $AppGradle) -match 'compileSdk\s*=\s*(\d+)')) { $Matches[1] } else { '35' }
$SdkPackages   = @('platform-tools', "platforms;android-$CompileSdk", 'build-tools;35.0.0')
$MinJava = 17                             # Gradle 8.14 runs on Java 17 to 24, 8.10-8.13 to 23 (newer Android Studio ships Java 25: too new)
$MaxJava = if ([version]$GradleVersion -ge [version]'8.14') { 24 } elseif ([version]$GradleVersion -ge [version]'8.10') { 23 } else { 22 }
$JdkVersion = 21                          # what the script installs when no fitting Java is found
$GradleDistPrefix = "gradle-$GradleVersion-"

# The release key: <Android folder>\Keys (the V1.1 / V1.2 build menu made it there already)
function Read-Properties([string]$File) {
    $h = @{}
    if (Test-Path $File) {
        foreach ($l in Get-Content $File) {
            # Java properties: a backslash escapes the next character
            if ($l -match '^\s*([^#=]+?)\s*=\s*(.*)$') { $h[$Matches[1]] = ($Matches[2].Trim() -replace '\\(.)', '$1') }
        }
    }
    return $h
}
$Legacy = Read-Properties $LegacySigning
if (-not $Keystore) {
    $Keystore = Join-Path $KeysDir 'wtms-release.jks'
    if ($Legacy['storeFile'] -and (Test-Path $Legacy['storeFile'])) { $Keystore = $Legacy['storeFile'] }
    # A key made by the earlier build script lives in %USERPROFILE%\.woodyouloveit: keep using it (Google Play needs the same key)
    $EarlierKey = [IO.Path]::Combine($Home_, '.woodyouloveit', 'wtms-release.jks')
    if (-not (Test-Path $Keystore) -and (Test-Path $EarlierKey)) {
        $Keystore = $EarlierKey
        $EarlierSecret = [IO.Path]::Combine($Home_, '.woodyouloveit', 'wtms-signing.dat')
        if (-not (Test-Path $SecretFile) -and (Test-Path $EarlierSecret)) { $SecretFile = $EarlierSecret }
    }
}
if (-not $KeyAlias) { $KeyAlias = if ($Legacy['keyAlias']) { $Legacy['keyAlias'] } else { 'wtms' } }

$Stamp   = Get-Date -Format 'yyyyMMdd-HHmmss'
$LogDir  = [IO.Path]::Combine($Repo, 'release', 'logs')
$LogFile = Join-Path $LogDir "build-android-$Stamp.log"
$Started = Get-Date

$Java = $null; $Keytool = $null; $Jarsigner = $null
$Quit = $false
$Secrets = New-Object System.Collections.Generic.List[string]     # never written to the screen or the log
$Sdk = $null; $Adb = $null; $Apksigner = $null
$LastExit = 0
$SdkManagerProblem = @()

# ---------------------------------------------------------------- output helpers

# Removes every known secret (keystore passwords) from text before it is shown or logged
function Hide-Secrets([string]$Text) {
    foreach ($secret in $Secrets) { if ($secret) { $Text = $Text.Replace($secret, '***') } }
    return $Text
}

function Write-Log([string]$Text) {
    if (-not (Test-Path $LogDir)) { New-Item -ItemType Directory -Force -Path $LogDir | Out-Null }
    Add-Content -Path $LogFile -Value (Hide-Secrets $Text)
}

function Step([string]$Text) { Write-Host ''; Write-Host "==> $Text" -ForegroundColor Cyan; Write-Log "==> $Text" }
function Ok([string]$Text)   { Write-Host "    [ok] $Text" -ForegroundColor Green; Write-Log "    [ok] $Text" }
function Info([string]$Text) { Write-Host "    $Text" -ForegroundColor Gray; Write-Log "    $Text" }
function Warn([string]$Text) { Write-Host "    [!] $Text" -ForegroundColor Yellow; Write-Log "    [!] $Text" }
function Fail([string]$Text) { throw $Text }

# Runs a program, shows and logs its output, and stops on failure.
# (Windows PowerShell 5.1 treats a program's warnings on stderr as errors, so they are passed through as text.)
function Invoke-Native {
    param([string]$Exe, [string[]]$Arguments, [string]$WorkDir = $Repo, [switch]$AllowFail, [switch]$Quiet, [string[]]$InputLines = $null)
    # Hide the value that follows a password option, and pass:... values
    $masked = New-Object System.Collections.Generic.List[string]
    $hideNext = $false
    foreach ($a in $Arguments) {
        if ($hideNext) { $masked.Add('***'); $hideNext = $false; continue }
        if ($a -match '^-(storepass|keypass|srcstorepass|deststorepass)$') { $masked.Add($a); $hideNext = $true; continue }
        $masked.Add(($a -replace '(pass[=:])(pass:)?[^\s]*', '$1***'))
    }
    $shown = Hide-Secrets ($masked -join ' ')
    Write-Log "    > $Exe $shown"
    if ($DryRun) { Write-Host "    [dry run] $Exe $shown" -ForegroundColor DarkGray; $global:LASTEXITCODE = 0; $script:LastExit = 0; return @() }

    $previous = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    Push-Location $WorkDir
    try {
        $lines = New-Object System.Collections.Generic.List[string]
        $sink = {
            $line = Hide-Secrets "$_"
            $lines.Add($line)
            Add-Content -Path $LogFile -Value $line
            if (-not $Quiet) { Write-Host "    $line" }
        }
        if ($InputLines) { $InputLines | & $Exe @Arguments 2>&1 | ForEach-Object -Process $sink }     # answers on stdin
        else { & $Exe @Arguments 2>&1 | ForEach-Object -Process $sink }
        $code = $LASTEXITCODE
        $script:LastExit = $code
    }
    finally {
        Pop-Location
        $ErrorActionPreference = $previous
    }
    if ($code -ne 0 -and -not $AllowFail) { Fail "$([IO.Path]::GetFileName($Exe)) failed (exit code $code). See the log: $LogFile" }
    return ,$lines.ToArray()
}

function Read-Plain([Security.SecureString]$Secure) {
    $ptr = [Runtime.InteropServices.Marshal]::SecureStringToBSTR($Secure)
    try { return [Runtime.InteropServices.Marshal]::PtrToStringBSTR($ptr) }
    finally { [Runtime.InteropServices.Marshal]::ZeroFreeBSTR($ptr) }
}

# ---------------------------------------------------------------- asking and downloading

# Asks before changing anything. Enter means yes, except where -DefaultNo asks for an explicit "y" (licences).
function Confirm-Fix([string]$Question, [switch]$DefaultNo) {
    if ($Yes -and -not $DefaultNo) { Info "$Question -> yes (-Yes)"; return $true }
    if ($Yes -and $DefaultNo) { Info "$Question -> yes (-Yes)"; return $true }
    if ($DryRun) { Info "[dry run] would ask: $Question"; return $false }
    $hint = if ($DefaultNo) { '[y/N]' } else { '[Y/n]' }
    $answer = Read-Host "    $Question $hint"
    if ($null -eq $answer) { $answer = '' }
    Write-Log "    ? $Question $hint -> '$answer'"
    if ($DefaultNo) { return ($answer -match '^\s*[yY]') }
    return ($answer.Trim() -eq '' -or $answer -match '^\s*[yY]')
}

function Get-FreeGB([string]$Path) {
    try {
        $full = [IO.Path]::GetFullPath($Path)
        $drive = New-Object IO.DriveInfo([IO.Path]::GetPathRoot($full))
        return [math]::Round($drive.AvailableFreeSpace / 1GB, 1)
    } catch { return -1 }
}

function Test-FreeSpace([string]$Path, [double]$NeedGB) {
    $free = Get-FreeGB $Path
    if ($free -ge 0 -and $free -lt $NeedGB) {
        Fail "Not enough free disk space: $free GB free on $([IO.Path]::GetPathRoot([IO.Path]::GetFullPath($Path))), about $NeedGB GB needed. Free some space and try again."
    }
}

function Set-WebDefaults {
    try { [Net.ServicePointManager]::SecurityProtocol = [Net.ServicePointManager]::SecurityProtocol -bor [Net.SecurityProtocolType]::Tls12 } catch { }
}

function Get-WebText([string]$Url) {
    Set-WebDefaults
    $wc = New-Object Net.WebClient
    $wc.Headers['User-Agent'] = 'woodyouloveit-build'
    if ($wc.Proxy) { $wc.Proxy.Credentials = [Net.CredentialCache]::DefaultCredentials }    # office / college proxies
    $wc.Encoding = [Text.Encoding]::UTF8
    try { return $wc.DownloadString($Url) }
    catch { Fail (Get-DownloadAdvice $Url $_.Exception) }
    finally { $wc.Dispose() }
}

# Plain-language reason for a failed download
function Get-DownloadAdvice([string]$Url, $Exception) {
    $hostName = try { ([Uri]$Url).Host } catch { $Url }
    $reason = $Exception.Message
    while ($Exception.InnerException) { $Exception = $Exception.InnerException; $reason = $Exception.Message }
    $tip = if ($reason -match 'trust|SSL|TLS|certificate') { 'A proxy or antivirus may be inspecting secure downloads; try another network (for example a phone hotspot).' }
           elseif ($reason -match '404|Not Found') { 'The file has moved; please send the log to woodyouloveit.' }
           else { 'Check the internet connection (and any proxy or firewall), then try again.' }
    return "Could not download from $hostName ($reason). $tip"
}

# Downloads with progress, then checks the official checksum (a damaged or altered file is deleted and refused)
function Save-Download([string]$Url, [string]$Path, [string]$Hash = '', [string]$Algorithm = 'SHA256') {
    Info "Downloading $Url"
    if ($DryRun) { return }
    Set-WebDefaults
    $request = [Net.HttpWebRequest]::Create($Url)
    $request.UserAgent = 'woodyouloveit-build'
    if ($request.Proxy) { $request.Proxy.Credentials = [Net.CredentialCache]::DefaultCredentials }
    try { $response = $request.GetResponse() }
    catch { Fail (Get-DownloadAdvice $Url $_.Exception) }
    $total = $response.ContentLength
    $source = $response.GetResponseStream()
    $target = [IO.File]::Create($Path)
    try {
        $buffer = New-Object byte[] 262144
        $done = [long]0
        $shown = -10
        while (($read = $source.Read($buffer, 0, $buffer.Length)) -gt 0) {
            $target.Write($buffer, 0, $read)
            $done += $read
            if ($total -gt 0) {
                $percent = [int](100 * $done / $total)
                if ($percent -ge $shown + 10) { Write-Host ("    {0,3}% of {1:N0} MB" -f $percent, ($total / 1MB)); $shown = $percent }
            }
        }
    }
    finally { $target.Close(); $source.Close(); $response.Close() }

    if ($Hash) {
        $actual = (Get-FileHash -Algorithm $Algorithm -Path $Path).Hash
        if ($actual -ne $Hash.Trim()) {
            Remove-Item -Force $Path -ErrorAction SilentlyContinue
            Fail "The download was damaged or changed on the way (checksum mismatch), so it was deleted: $Url. Try again; if it repeats, the network (or antivirus) is altering downloads."
        }
        Ok "Download verified ($Algorithm checksum)"
    }
    else { Warn 'No official checksum was available for this download.' }
}

# Unpacks .zip (Windows' built-in tar is much faster than Expand-Archive) or .tar.gz / .tar.xz
function Expand-Download([string]$Archive, [string]$Destination) {
    New-Item -ItemType Directory -Force -Path $Destination | Out-Null
    if ($DryRun) { return }
    Info 'Unpacking...'
    $winTar = if ($OnWindows) { Join-Path $env:SystemRoot 'System32\tar.exe' } else { '' }
    if ($Archive -match '\.zip$') {
        if ($OnWindows -and (Test-Path $winTar)) { Invoke-Native $winTar @('-xf', $Archive, '-C', $Destination) -Quiet | Out-Null }
        elseif ($OnWindows) { Expand-Archive -Path $Archive -DestinationPath $Destination -Force }
        else { Invoke-Native 'unzip' @('-q', '-o', $Archive, '-d', $Destination) -Quiet | Out-Null }
    }
    else { Invoke-Native 'tar' @('-xf', $Archive, '-C', $Destination) -Quiet | Out-Null }
}

# The last lines a tool printed, shown when it fails (everything is in the log)
function Show-Tail([string[]]$Lines, [int]$Count = 10) {
    $useful = @($Lines | Where-Object { $_ -and $_.Trim() -and $_ -notmatch '^\s*\[=*\s*\]' })
    if ($useful.Count -eq 0) { Info '(the tool printed nothing)'; return }
    Info 'It said:'
    $useful | Select-Object -Last $Count | ForEach-Object { Write-Host "      | $_" -ForegroundColor DarkYellow }
}

function New-TempDir([string]$Name) {
    $dir = Join-Path ([IO.Path]::GetTempPath()) "wyli-$Name-$Stamp"
    if (Test-Path $dir) { Remove-Item -Recurse -Force $dir }
    New-Item -ItemType Directory -Force -Path $dir | Out-Null
    return $dir
}

function Get-OsArch {
    $mac = [bool](Get-Variable -Name IsMacOS -ValueOnly -ErrorAction SilentlyContinue)
    $os = if ($OnWindows) { 'windows' } elseif ($mac) { 'mac' } else { 'linux' }
    $arm = ($env:PROCESSOR_ARCHITECTURE -eq 'ARM64') -or ([Runtime.InteropServices.RuntimeInformation]::OSArchitecture -eq 'Arm64')
    return @{ Os = $os; Arm = $arm }
}

# ---------------------------------------------------------------- automatic set-up: Java

function Install-Jdk {
    $target = $JdkDir
    Info "I can download Java $JdkVersion (Eclipse Temurin by the Adoptium project, free and open source, about 190 MB)"
    Info "into $target, used only for these builds (Android Studio and the rest of the PC do not change)."
    if (-not (Confirm-Fix "Download Java $JdkVersion now?")) { Fail "Java $MinJava to $MaxJava is needed. Run this again and answer y, or set JAVA_HOME to such a JDK." }
    Test-FreeSpace (Split-Path -Parent $target) 1
    $sys = Get-OsArch
    $os = $sys.Os; $arch = if ($sys.Arm) { 'aarch64' } else { 'x64' }
    $assets = @(Get-WebText "$AdoptiumApi/v3/assets/latest/$JdkVersion/hotspot?architecture=$arch&image_type=jdk&os=$os&vendor=eclipse" | ConvertFrom-Json)
    if ($assets.Count -eq 0) { Fail "Adoptium did not offer a Java $JdkVersion download for this PC." }
    $package = $assets[0].binary.package
    $tmp = New-TempDir 'jdk'
    $file = Join-Path $tmp $package.name
    Save-Download $package.link $file $package.checksum 'SHA256'
    $unpacked = Join-Path $tmp 'x'
    Expand-Download $file $unpacked
    $top = Get-ChildItem -Path $unpacked -Directory | Select-Object -First 1
    if (Test-Path $target) { Remove-Item -Recurse -Force $target }
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $target) | Out-Null
    Move-Item -Path $top.FullName -Destination $target
    Remove-Item -Recurse -Force $tmp -ErrorAction SilentlyContinue
    # macOS packages keep Java inside Contents/Home
    $javaHome = if (Test-Path (Join-Path $target 'Contents/Home')) { Join-Path $target 'Contents/Home' } else { $target }
    Ok "Java $JdkVersion installed: $javaHome"
    return $javaHome
}

# ---------------------------------------------------------------- automatic set-up: Android SDK

function Get-DefaultSdkDir {
    if ($env:WYLI_SDK_DEFAULT) { return $env:WYLI_SDK_DEFAULT }
    if ($AndroidRoot) { return (Join-Path $AndroidRoot 'Sdk') }
    if ($OnWindows) { return (Join-Path $env:LOCALAPPDATA 'Android\Sdk') }
    return (Join-Path $Home_ 'Android/Sdk')
}

function Get-SdkManager([string]$SdkDir) {
    $name = if ($OnWindows) { 'sdkmanager.bat' } else { 'sdkmanager' }
    $latest = [IO.Path]::Combine($SdkDir, 'cmdline-tools', 'latest', 'bin', $name)
    if (Test-Path $latest) { return $latest }
    $any = Find-Latest (Join-Path $SdkDir 'cmdline-tools') (Join-Path 'bin' $name)
    if ($any) { return $any }
    $old = [IO.Path]::Combine($SdkDir, 'tools', 'bin', $name)
    if (Test-Path $old) { return $old }
    return $null
}

# Google's package list says which command-line tools are current and gives their official checksum
function Get-CmdlineToolsArchive {
    $sys = Get-OsArch
    $os = @{ windows = 'windows'; mac = 'macosx'; linux = 'linux' }[$sys.Os]
    $tag = @{ windows = 'win'; mac = 'mac'; linux = 'linux' }[$sys.Os]
    try {
        [xml]$repository = Get-WebText "$SdkRepoUrl/repository2-3.xml"
        foreach ($package in $repository.SelectNodes("//*[local-name()='remotePackage' and @path='cmdline-tools;latest']")) {
            $channel = $package.SelectSingleNode("*[local-name()='channelRef']")
            if ($channel -and $channel.GetAttribute('ref') -ne 'channel-0') { continue }      # stable releases only
            foreach ($archive in $package.SelectNodes(".//*[local-name()='archive']")) {
                $hostOs = $archive.SelectSingleNode("*[local-name()='host-os']")
                if (-not $hostOs -or $hostOs.InnerText -ne $os) { continue }
                $url = $archive.SelectSingleNode(".//*[local-name()='url']").InnerText
                $sum = $archive.SelectSingleNode(".//*[local-name()='checksum']").InnerText
                if ($url -notmatch '^https?:') { $url = "$SdkRepoUrl/$url" }
                return @{ Url = $url; Sha1 = $sum }
            }
        }
        Warn "Google's package list had no command-line tools for this system; using a known version."
    }
    catch { Warn "Could not read Google's package list ($($_.Exception.Message)); using a known version." }
    return @{ Url = "$SdkRepoUrl/commandlinetools-$tag-$($CmdlineToolsFallback)_latest.zip"; Sha1 = '' }
}

function Install-CmdlineTools([string]$SdkDir) {
    Step "Installing Google's Android SDK command-line tools"
    $archive = Get-CmdlineToolsArchive
    $tmp = New-TempDir 'cmdline'
    $zip = Join-Path $tmp 'cmdline-tools.zip'
    Save-Download $archive.Url $zip $archive.Sha1 'SHA1'
    $unpacked = Join-Path $tmp 'x'
    Expand-Download $zip $unpacked
    if ($DryRun) { return $null }
    $target = [IO.Path]::Combine($SdkDir, 'cmdline-tools', 'latest')
    if (Test-Path $target) { Remove-Item -Recurse -Force $target }
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $target) | Out-Null
    Move-Item -Path (Join-Path $unpacked 'cmdline-tools') -Destination $target       # sdkmanager expects cmdline-tools\latest
    Remove-Item -Recurse -Force $tmp -ErrorAction SilentlyContinue
    $name = if ($OnWindows) { 'sdkmanager.bat' } else { 'sdkmanager' }
    $manager = Join-Path (Join-Path $target 'bin') $name
    if (-not (Test-Path $manager)) { Fail 'The command-line tools were unpacked, but sdkmanager was not found in them.' }
    if (-not (Test-SdkManager $manager $SdkDir)) {
        Warn 'The freshly installed SDK manager does not start.'
        Show-Tail $script:SdkManagerProblem 8
        Fail "Google's SDK manager could not start with Java at $env:JAVA_HOME. Please send the log to woodyouloveit: $LogFile"
    }
    Ok 'Command-line tools installed and working'
    return $manager
}

# Runs "sdkmanager --version": old copies (Java 8 era "tools\bin") and broken unzips fail here
function Test-SdkManager([string]$Manager, [string]$SdkDir) {
    $out = Invoke-Native $Manager @("--sdk_root=$SdkDir", '--version') -AllowFail -Quiet
    if ($DryRun) { return $true }
    $ok = ($script:LastExit -eq 0) -and (($out -join ' ') -match '\d+\.\d+')
    if (-not $ok) { $script:SdkManagerProblem = $out }
    return $ok
}

function Get-OrInstallSdkManager([string]$SdkDir, [switch]$NoAsk) {
    $manager = Get-SdkManager $SdkDir
    if ($manager) {
        if (Test-SdkManager $manager $SdkDir) { Info "Using the SDK manager in $(Split-Path -Parent (Split-Path -Parent $manager))"; return $manager }
        Warn "An SDK manager is already in $SdkDir, but it does not run with this set-up (usually an old or incomplete copy)."
        Show-Tail $script:SdkManagerProblem 6
        Info 'A fresh copy will be installed next to it (the old one is left alone).'
        $NoAsk = $false
    }
    if (-not $NoAsk) {
        Info "Google's SDK manager (part of the command-line tools, about 150 MB) is needed for this."
        if (-not (Confirm-Fix 'Download it now?')) { Fail 'The SDK manager is needed. Install Android Studio, or run this again and answer y.' }
    }
    return (Install-CmdlineTools $SdkDir)
}

function Test-SdkLicences([string]$SdkDir) { return (Test-Path ([IO.Path]::Combine($SdkDir, 'licenses', 'android-sdk-license'))) }

function Approve-SdkLicences([string]$SdkDir, [switch]$Force) {
    if ((Test-SdkLicences $SdkDir) -and -not $Force) { return }
    Step 'Android SDK licence'
    Info "Building Android apps needs Google's Android SDK licence to be accepted. Read it here:"
    Info '  https://developer.android.com/studio/terms'
    if (-not (Confirm-Fix 'Do you accept the Android SDK licence terms?' -DefaultNo)) {
        Fail 'The Android SDK licence must be accepted before Android apps can be built.'
    }
    $manager = Get-OrInstallSdkManager $SdkDir
    if ($DryRun) { return }
    $out = Invoke-Native $manager @("--sdk_root=$SdkDir", '--licenses') -InputLines (@(1..40) | ForEach-Object { 'y' }) -Quiet -AllowFail
    if (Test-SdkLicences $SdkDir) { Ok 'Android SDK licences accepted'; return }

    Warn "Google's SDK manager did not record the licence."
    Show-Tail $out 8
    # You accepted the terms above: record that acceptance the same way sdkmanager does (Gradle reads these files)
    Save-LicenceAcceptance $SdkDir
}

# Writes the acceptance records sdkmanager would write: one SHA-1 per accepted version of each licence text
function Save-LicenceAcceptance([string]$SdkDir) {
    $dir = Join-Path $SdkDir 'licenses'
    New-Item -ItemType Directory -Force -Path $dir | Out-Null
    $records = @{
        'android-sdk-license'         = @('24333f8a63b6825ea9c5514f83c2829b004d1fee', 'd56f5187479451eabf01fb78af6dfcb131a6481e', '8933bad161af4178b1185d1a37fbf41ea5269c55')
        'android-sdk-preview-license' = @('84831b9409646a918e30573bab4c9c91346d8abd')
    }
    foreach ($name in $records.Keys) {
        $file = Join-Path $dir $name
        $existing = if (Test-Path $file) { @(Get-Content $file) } else { @() }
        $lines = @($existing + $records[$name] | Where-Object { $_ } | Select-Object -Unique)
        Set-Content -Path $file -Value $lines -Encoding ASCII
    }
    if (-not (Test-SdkLicences $SdkDir)) { Fail "The licence acceptance could not be saved in $dir (is the folder read-only?). See the log: $LogFile" }
    Ok "Your acceptance of the Android SDK licence was recorded in $dir"
}

function Install-SdkPackages([string]$SdkDir, [string[]]$Packages) {
    $manager = Get-OrInstallSdkManager $SdkDir
    Info "Installing $($Packages -join ', ') (a few minutes)..."
    if ($DryRun) { return }
    $out = Invoke-Native $manager (@("--sdk_root=$SdkDir") + $Packages) -InputLines (@(1..10) | ForEach-Object { 'y' }) -Quiet -AllowFail
    if ($script:LastExit -eq 0) { Ok "Installed $($Packages -join ', ')"; return }
    Warn "Google's SDK manager could not install $($Packages -join ', ')."
    Show-Tail $out 8
    Info 'Not a problem for building: Gradle downloads the parts it needs during the build.'
}

function Save-AndroidHome([string]$SdkDir) {
    if (-not $OnWindows -or $DryRun) { return }
    if ([Environment]::GetEnvironmentVariable('ANDROID_HOME', 'User') -eq $SdkDir) { return }
    if (Confirm-Fix 'Remember this SDK folder for other tools too (sets ANDROID_HOME for your Windows account)?') {
        [Environment]::SetEnvironmentVariable('ANDROID_HOME', $SdkDir, 'User')
        Ok 'ANDROID_HOME saved (new windows will see it)'
    }
}

function Install-AndroidSdk {
    $dir = Get-DefaultSdkDir
    Warn "The Android SDK (Google's tools for building Android apps) is not installed on this PC."
    Info 'I can set it up for you now:'
    Info "  1. download Google's SDK command-line tools from dl.google.com (about 150 MB, checksum-verified)"
    Info '  2. ask you to accept Google''s Android SDK licence'
    Info '  3. install the Android 16 platform, build tools and platform tools (about 400 MB)'
    Info "  into $dir (shared with Android Studio)."
    if (-not (Confirm-Fix 'Set up the Android SDK now?')) {
        Fail 'The Android SDK is needed to build the app. Install Android Studio, or run this again and answer y.'
    }
    Test-FreeSpace (Split-Path -Parent $dir) 3
    New-Item -ItemType Directory -Force -Path $dir | Out-Null
    Get-OrInstallSdkManager $dir -NoAsk | Out-Null
    Approve-SdkLicences $dir
    Step 'Installing the Android SDK packages'
    Install-SdkPackages $dir $SdkPackages
    Save-AndroidHome $dir
    return $dir
}

# ---------------------------------------------------------------- Gradle doctor

# Recognises common build failures; each comes with a plain explanation and, where possible, a fix
function Get-GradleDiagnosis([string[]]$Output) {
    $text = ($Output -join "`n")
    if ($text -match 'licen[cs]es? (have|has) not been accepted|not accepted the licen[cs]e agreements') {
        return @{ Key = 'licences'; Problem = "Google's Android SDK licence has not been accepted on this PC."
            Question = 'Show the licence and accept it, then build again?'; Fix = { Approve-SdkLicences $script:Sdk -Force }
            Advice = 'Accept the Android SDK licence (run "7 Check and fix my set-up").' }
    }
    if ($text -match "Failed to find Build Tools revision ([0-9.]+)") {
        $package = "build-tools;$($Matches[1])"
        return @{ Key = $package; Problem = "The build needs Android build tools $($Matches[1]), which are not installed."
            Question = "Install $package now, then build again?"; Fix = [scriptblock]::Create("Install-SdkPackages `$script:Sdk @('$package')")
            Advice = "Install $package with Android Studio's SDK Manager." }
    }
    if ($text -match "Failed to find target with hash string '?android-(\d+)|Failed to find Platform SDK with path: platforms;android-(\d+)") {
        $level = if ($Matches[1]) { $Matches[1] } else { $Matches[2] }
        $package = "platforms;android-$level"
        return @{ Key = $package; Problem = "The Android $level platform is not installed."
            Question = "Install $package now, then build again?"; Fix = [scriptblock]::Create("Install-SdkPackages `$script:Sdk @('$package')")
            Advice = "Install $package with Android Studio's SDK Manager." }
    }
    if ($text -match 'SDK location not found|ANDROID_HOME.*(not set|invalid)') {
        return @{ Key = 'sdk'; Problem = 'Gradle could not find the Android SDK.'
            Question = 'Set up the Android SDK now, then build again?'; Fix = { $script:Sdk = Install-AndroidSdk; Write-LocalProperties }
            Advice = 'Install the Android SDK (run "7 Check and fix my set-up").' }
    }
    if ($text -match 'requires Java (\d+) to run|Unsupported class file major version|Unsupported Java|incompatible with the Java version|JAVA_HOME is set to an invalid directory') {
        return @{ Key = 'java'; Problem = "This build needs Java $MinJava to $MaxJava, and the Java in use doesn't fit."
            Question = "Download Java $JdkVersion for these builds, then build again?"; Fix = { Use-Java (Install-Jdk) }
            Advice = "Install Java $JdkVersion and set JAVA_HOME to it (Android Studio's own Java 25 is too new for this project)." }
    }
    if ($text -match 'Could not install Gradle distribution|zip END header not found|java\.util\.zip\.ZipException|error in opening zip file|Could not unzip') {
        return @{ Key = 'dist'; Problem = "Gradle's own download (from an earlier run) is incomplete or damaged."
            Question = 'Delete it and download Gradle again?'; Fix = { Remove-GradleDistribution }
            Advice = "Delete the folder $([IO.Path]::Combine($GradleHome, 'wrapper', 'dists')) and try again." }
    }
    if ($text -match 'build daemon disappeared|daemon .*unexpectedly|Could not connect to the Gradle daemon|Timeout waiting to connect to the Gradle daemon|Could not receive a message from the daemon|Timeout waiting to lock|currently in use by another Gradle instance') {
        return @{ Key = 'daemon'; Problem = 'A Gradle background process from an earlier build is stuck or holding files.'
            Question = 'Stop the Gradle background processes, then build again?'; Fix = { Stop-GradleDaemons }
            Advice = 'Restart the PC, then build again.' }
    }
    if ($text -match 'OutOfMemoryError|Java heap space|GC overhead limit|Metaspace') {
        return @{ Key = 'memory'; Problem = 'Gradle ran out of memory.'
            Question = 'Give Gradle more memory (4 GB) and build again?'; Fix = { Set-GradleMemory '4096m' }
            Advice = 'Close other programs, or raise org.gradle.jvmargs in android\gradle.properties.' }
    }
    if ($text -match 'not enough space on the disk|No space left on device') {
        return @{ Key = 'disk'; Problem = 'The disk is full.'; Fix = $null
            Advice = "Free a few GB on the drive with $Repo (and on the drive with $GradleHome), then build again." }
    }
    if ($text -match 'Filename too long|path too long|filename or extension is too long') {
        return @{ Key = 'path'; Problem = 'Windows cannot handle file paths this long.'; Fix = $null
            Advice = "Move the project to a short folder such as D:\Android\Projects\WTMS (it is now in $Repo) and build again." }
    }
    if ($text -match 'PKIX path building failed|unable to find valid certification path|SSLHandshakeException') {
        return @{ Key = 'ssl'; Problem = 'A secure download was blocked: a proxy or antivirus is inspecting HTTPS traffic.'; Fix = $null
            Advice = 'Build on another network (for example a phone hotspot), or ask the network admin to allow services.gradle.org, dl.google.com, repo.maven.apache.org and jitpack.io.' }
    }
    if ($text -match 'Could not resolve|UnknownHostException|Connect timed out|Read timed out|Connection (refused|reset)|No route to host|Could not GET|Could not HEAD|Could not download|Network is unreachable') {
        return @{ Key = 'network'; Problem = 'Gradle could not download what it needs (the first build downloads a few hundred MB).'
            Question = 'Check your internet connection. Try again?'; Fix = { Start-Sleep -Seconds 3 }
            Advice = 'Check the internet connection (and any proxy or firewall), then build again.' }
    }
    if ($text -match '(?m)^e: |Compilation error|Unresolved reference|AAPT: error|Android resource linking failed') {
        return @{ Key = 'code'; Problem = 'The app code itself failed to compile; this is not a set-up problem.'; Fix = $null
            Advice = "Please send the log to woodyouloveit so it can be fixed: $LogFile" }
    }
    return $null
}

function Remove-GradleDistribution {
    $dists = [IO.Path]::Combine($GradleHome, 'wrapper', 'dists')
    Get-ChildItem -Path $dists -Directory -Filter "$GradleDistPrefix*" -ErrorAction SilentlyContinue | ForEach-Object { Remove-Item -Recurse -Force $_.FullName }
    Ok 'Removed the damaged Gradle download'
}

function Stop-GradleDaemons {
    Invoke-Native (Get-GradleWrapper) @('--stop') -WorkDir $AndroidDir -AllowFail -Quiet | Out-Null
    Ok 'Gradle background processes stopped'
}

function Set-GradleMemory([string]$Size) {
    $file = Join-Path $AndroidDir 'gradle.properties'
    $content = Get-Content -Raw -Path $file
    $content = $content -replace '-Xmx\d+[mMgG]', "-Xmx$Size"
    Set-Content -Path $file -Value $content.TrimEnd() -Encoding ASCII
    Ok "Gradle memory set to $Size"
}

# ---------------------------------------------------------------- finding the tools

function Get-JavaMajor([string]$JavaExe) {
    $previous = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    $text = (& $JavaExe -version 2>&1 | ForEach-Object { "$_" }) -join "`n"
    $ErrorActionPreference = $previous
    if ($text -match 'version "(\d+)(\.(\d+))?') {
        $major = [int]$Matches[1]
        if ($major -eq 1 -and $Matches[3]) { $major = [int]$Matches[3] }    # "1.8" style
        return $major
    }
    return 0
}

function Test-JavaFits([string]$JavaExe) {
    $major = Get-JavaMajor $JavaExe
    if ($major -gt $MaxJava) { $script:JavaTooNew = $major }
    return ($major -ge $MinJava -and $major -le $MaxJava)
}

function Find-Java {
    $exe = if ($OnWindows) { 'java.exe' } else { 'java' }
    $candidates = @()
    if ($env:JAVA_HOME) { $candidates += $env:JAVA_HOME }
    $candidates += $JdkDir                                                         # downloaded by this script
    $candidates += [IO.Path]::Combine($JdkDir, 'Contents', 'Home')
    if ($AndroidRoot) {
        $candidates += (Join-Path $AndroidRoot 'jdk-17')
        $candidates += (Join-Path $AndroidRoot 'Android Studio\jbr')               # Android Studio's own Java (fits only up to Java 24)
    }
    if ($OnWindows) {
        # Android Studio ships its own Java 17+ (JetBrains Runtime)
        $candidates += (Join-Path $env:ProgramFiles 'Android\Android Studio\jbr')
        $candidates += (Join-Path $env:ProgramFiles 'Android\Android Studio\jre')
        if ($env:LOCALAPPDATA) { $candidates += (Join-Path $env:LOCALAPPDATA 'Programs\Android Studio\jbr') }
    }
    foreach ($javaDir in $candidates) {
        $path = Join-Path (Join-Path $javaDir 'bin') $exe
        if ((Test-Path $path) -and (Test-JavaFits $path)) { return $javaDir }
    }
    if (-not $env:WYLI_TEST_SKIP_SYSTEM_JAVA) {
        $cmd = Get-Command $exe -ErrorAction SilentlyContinue
        if ($cmd -and (Test-JavaFits $cmd.Path)) { return (Split-Path -Parent (Split-Path -Parent $cmd.Path)) }
    }
    return $null
}

function Use-Java([string]$JavaHome) {
    $env:JAVA_HOME = $JavaHome
    $bin = Join-Path $JavaHome 'bin'
    $ext = if ($OnWindows) { '.exe' } else { '' }
    $script:Java = Join-Path $bin "java$ext"
    $script:Keytool = Join-Path $bin "keytool$ext"
    $script:Jarsigner = Join-Path $bin "jarsigner$ext"
    $env:PATH = $bin + [IO.Path]::PathSeparator + $env:PATH
}

function Write-LocalProperties {
    # Gradle reads the SDK location from android\local.properties (backslashes and colons escaped)
    $escaped = $script:Sdk.Replace('\', '\\').Replace(':', '\:')
    if (-not $DryRun) { Set-Content -Path (Join-Path $AndroidDir 'local.properties') -Value "sdk.dir=$escaped" -Encoding ASCII }
}

function Find-Sdk {
    $candidates = @()
    if ($AndroidRoot) { $candidates += (Join-Path $AndroidRoot 'Sdk') }
    $candidates += @($env:ANDROID_HOME, $env:ANDROID_SDK_ROOT)
    if ($OnWindows -and $env:LOCALAPPDATA) { $candidates += (Join-Path $env:LOCALAPPDATA 'Android\Sdk') }
    if (-not $OnWindows) { $candidates += (Join-Path $Home_ 'Android/Sdk'); $candidates += (Join-Path $Home_ 'Library/Android/sdk') }
    foreach ($c in $candidates) {
        if (-not $c) { continue }
        foreach ($part in 'platform-tools', 'platforms', 'build-tools', 'licenses') {
            if (Test-Path (Join-Path $c $part)) { return $c }
        }
    }
    return $null
}

function Find-Latest([string]$Dir, [string]$File) {
    if (-not (Test-Path $Dir)) { return $null }
    $found = Get-ChildItem -Path $Dir -Directory | Sort-Object { try { [version]$_.Name } catch { [version]'0.0' } } -Descending |
        ForEach-Object { Join-Path $_.FullName $File } | Where-Object { Test-Path $_ } | Select-Object -First 1
    return $found
}

function Initialize-Tools([switch]$NeedSdk, [switch]$NeedAdb) {
    Step 'Checking the set-up'
    if ($AndroidRoot) { Ok "Android folder: $AndroidRoot" }
    Ok "Gradle folder: $GradleHome"

    # Java 17 to 24
    $script:JavaTooNew = 0
    $javaHome = Find-Java
    if (-not $javaHome) {
        if ($script:JavaTooNew) { Warn "Java $($script:JavaTooNew) is installed (Android Studio's own), but this project's Gradle ($GradleVersion) runs on Java $MinJava to $MaxJava only." }
        else { Warn "Java $MinJava to $MaxJava (needed to build Android apps) was not found." }
        if ($NoAutoFix) { Fail "Install Java $JdkVersion, or set JAVA_HOME to a JDK $MinJava to $MaxJava folder." }
        $javaHome = Install-Jdk
    }
    Use-Java $javaHome
    Ok "Java $(Get-JavaMajor $script:Java): $javaHome"

    if ($NeedSdk) {
        $script:Sdk = Find-Sdk
        if (-not $script:Sdk) {
            if ($NoAutoFix) { Fail 'The Android SDK was not found. Open Android Studio once (it installs the SDK) or set ANDROID_HOME to the SDK folder.' }
            $script:Sdk = Install-AndroidSdk
        }
        $env:ANDROID_HOME = $script:Sdk
        Ok "Android SDK: $($script:Sdk)"
        Write-LocalProperties

        # Without an accepted licence Gradle stops half-way: settle it now
        if (-not (Test-SdkLicences $script:Sdk)) {
            if ($NoAutoFix) { Fail 'The Android SDK licence has not been accepted on this PC.' }
            Approve-SdkLicences $script:Sdk
        }
        else { Ok 'Android SDK licence accepted' }

        $adbName = if ($OnWindows) { 'adb.exe' } else { 'adb' }
        $adbPath = Join-Path (Join-Path $script:Sdk 'platform-tools') $adbName
        if (-not (Test-Path $adbPath) -and $NeedAdb -and -not $NoAutoFix) {
            Warn "Google's platform tools (adb, needed to install on a phone) are missing."
            if (Confirm-Fix 'Install them now (about 10 MB)?') { Install-SdkPackages $script:Sdk @('platform-tools') }
        }
        if (Test-Path $adbPath) { $script:Adb = $adbPath; Ok 'adb (phone connection) found' }
        elseif ($NeedAdb) { Warn 'adb is missing, so the app cannot be installed on a phone from here.' }

        $signerName = if ($OnWindows) { 'apksigner.bat' } else { 'apksigner' }
        $script:Apksigner = Find-Latest (Join-Path $script:Sdk 'build-tools') $signerName
        if ($script:Apksigner) { Ok "apksigner: $(Split-Path -Leaf (Split-Path -Parent $script:Apksigner))" }
        else { Info 'apksigner appears after the first build (Gradle downloads the build tools).' }
    }

}

# ---------------------------------------------------------------- version

function Get-AppVersion {
    $props = Read-Properties (Join-Path $AndroidDir 'version.properties')
    $name = if ($VersionName) { $VersionName } elseif ($props['versionName']) { $props['versionName'] } else { '1.0.0' }
    $code = if ($VersionCode -gt 0) { $VersionCode } elseif ($props['versionCode']) { [int]$props['versionCode'] } else { 1 }
    return @{ Name = $name; Code = $code }
}

# Saves a new version in android\version.properties (the menu asks before a release)
function Save-AppVersion([string]$Name, [int]$Code) {
    $file = Join-Path $AndroidDir 'version.properties'
    if ($DryRun) { Info "[dry run] version $Name (code $Code)"; return }
    Set-Content -Path $file -Encoding ASCII -Value @(
        '# App version shown in the Play Store (versionName) and the number Google Play checks (versionCode).',
        '# versionCode must go up for every upload to Google Play.',
        "versionName=$Name",
        "versionCode=$Code")
    Ok "App version set to $Name (code $Code)"
}

# ---------------------------------------------------------------- Gradle

function Get-GradleWrapper {
    if ($env:WYLI_TEST_GRADLE) { return $env:WYLI_TEST_GRADLE }
    $wrapper = if ($OnWindows) { Join-Path $AndroidDir 'gradlew.bat' } else { Join-Path $AndroidDir 'gradlew' }
    if (-not (Test-Path $wrapper)) { Fail "Gradle wrapper not found: $wrapper" }
    return $wrapper
}

# Runs Gradle; when it fails for a known reason, explains it and (with your permission) fixes it and tries again
function Invoke-Gradle([string[]]$Tasks) {
    $wrapper = Get-GradleWrapper
    $applied = @{}
    for ($attempt = 1; $attempt -le 5; $attempt++) {
        $version = Get-AppVersion
        $arguments = @() + $Tasks + @("-PappVersionCode=$($version.Code)", "-PappVersionName=$($version.Name)", '--console=plain')
        $out = Invoke-Native $wrapper $arguments -WorkDir $AndroidDir -AllowFail
        if ($script:LastExit -eq 0) { return }

        Write-Host ''
        $diagnosis = Get-GradleDiagnosis $out
        if (-not $diagnosis) { Fail "The build failed. The messages above show why (full log: $LogFile). If it is not clear, send the log to woodyouloveit." }
        Warn $diagnosis.Problem
        $repeat = $applied.ContainsKey($diagnosis.Key) -and $diagnosis.Key -ne 'network'
        if (-not $diagnosis.Fix -or $NoAutoFix -or $repeat -or ($diagnosis.Key -eq 'network' -and $applied['network'] -ge 2)) { Fail $diagnosis.Advice }
        if (-not (Confirm-Fix $diagnosis.Question)) { Fail $diagnosis.Advice }
        $applied[$diagnosis.Key] = 1 + [int]$applied[$diagnosis.Key]
        & $diagnosis.Fix
        Step "Building again (attempt $($attempt + 1))"
    }
    Fail "The build still fails after the automatic fixes. See the log: $LogFile"
}

# ---------------------------------------------------------------- signing

function Get-KeystorePassword([switch]$New) {
    $plain = Get-KeystorePasswordInner -New:$New
    if ($plain -and $plain.Length -ge 4) { $Secrets.Add($plain) }
    return $plain
}

function Get-KeystorePasswordInner([switch]$New) {
    if ($env:ANDROID_KEYSTORE_PASSWORD) { return $env:ANDROID_KEYSTORE_PASSWORD }
    if (-not $New -and $Legacy['storePassword'] -and (Test-Path $Keystore)) {
        Info "Using the keystore password saved with the key ($LegacySigning)."
        return $Legacy['storePassword']
    }
    if (-not $New -and $OnWindows -and (Test-Path $SecretFile)) {
        try {
            $plain = Read-Plain (Get-Content $SecretFile | ConvertTo-SecureString)
            Info 'Using the remembered keystore password.'
            return $plain
        } catch { Warn 'The remembered password could not be read; please type it.' }
    }
    if ($DryRun) { return 'dry-run' }

    $first = Read-Host -AsSecureString '    Keystore password (at least 6 characters)'
    if ($New) {
        $second = Read-Host -AsSecureString '    Type it again'
        if ((Read-Plain $first) -ne (Read-Plain $second)) { Fail 'The passwords do not match.' }
    }
    $plain = Read-Plain $first
    if ($plain.Length -lt 6) { Fail 'The password needs at least 6 characters.' }

    if ($RememberPassword -and $OnWindows) {
        New-Item -ItemType Directory -Force -Path $ToolsDir | Out-Null
        $first | ConvertFrom-SecureString | Set-Content -Path $SecretFile
        Ok "Password remembered for your Windows account only ($SecretFile)."
    }
    return $plain
}

function New-Keystore {
    Initialize-Tools
    Step 'Creating the release signing key'
    if (Test-Path $Keystore) { Fail "A key already exists: $Keystore. Keep using it: Google Play only accepts updates signed with the same key." }

    Warn 'This key signs every future update. If it is lost, you cannot update the app on Google Play.'
    Warn 'Back it up (and the password) somewhere safe outside this PC.'
    $name = Read-Host '    Name on the certificate [Chanchal Sakarde]'
    if (-not $name) { $name = 'Chanchal Sakarde' }
    $password = Get-KeystorePassword -New

    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Keystore) | Out-Null
    $dname = "CN=$($name -replace ',', ' '), O=woodyouloveit, L=Pune, ST=Maharashtra, C=IN"
    Invoke-Native $Keytool @('-genkeypair', '-v', '-keystore', $Keystore, '-storetype', 'PKCS12', '-keyalg', 'RSA', '-keysize', '4096',
        '-validity', '10000', '-alias', $KeyAlias, '-dname', $dname, '-storepass', $password, '-keypass', $password) -Quiet | Out-Null
    Ok "Key created: $Keystore"
    Show-KeyFingerprint $password
}

function Show-KeyFingerprint([string]$Password) {
    $out = Invoke-Native $Keytool @('-list', '-v', '-keystore', $Keystore, '-alias', $KeyAlias, '-storepass', $Password) -Quiet
    $sha = $out | Where-Object { $_ -match 'SHA256:' } | Select-Object -First 1
    if ($sha) { Info "Certificate fingerprint ($($sha.Trim()))" }
}

function Set-SigningEnvironment {
    if (-not (Test-Path $Keystore) -and -not $DryRun) {
        Fail "No release signing key yet ($Keystore). Create one first: menu option 6, or  .\tools\build-android.ps1 -Task Keystore"
    }
    $password = Get-KeystorePassword
    $env:ANDROID_KEYSTORE_FILE = $Keystore
    $env:ANDROID_KEYSTORE_PASSWORD = $password
    $env:ANDROID_KEY_ALIAS = $KeyAlias
    $env:ANDROID_KEY_PASSWORD = $password      # PKCS12 keys use the same password for the store and the key
    if (-not $DryRun) {
        # Wrong password? Find out now rather than after a long build
        $check = Invoke-Native $Keytool @('-list', '-keystore', $Keystore, '-alias', $KeyAlias, '-storepass', $password) -AllowFail -Quiet
        if ($LASTEXITCODE -ne 0) { Clear-SigningEnvironment; Fail 'The keystore password (or key alias) is not right.' }
    }
    Ok "Signing with $Keystore (alias $KeyAlias)"
}

function Clear-SigningEnvironment {
    foreach ($name in 'ANDROID_KEYSTORE_FILE', 'ANDROID_KEYSTORE_PASSWORD', 'ANDROID_KEY_ALIAS', 'ANDROID_KEY_PASSWORD') {
        Remove-Item "Env:$name" -ErrorAction SilentlyContinue
    }
}

# ---------------------------------------------------------------- outputs

function Get-OutputDir {
    $version = Get-AppVersion
    $dir = [IO.Path]::Combine($Repo, 'release', 'android', $version.Name)
    if (-not $DryRun) { New-Item -ItemType Directory -Force -Path $dir | Out-Null }
    return $dir
}

function Copy-Output([string]$Source, [string]$Kind, [string]$Extension) {
    $version = Get-AppVersion
    $target = Join-Path (Get-OutputDir) "$FilePrefix-$($version.Name)-$Kind.$Extension"
    if ($DryRun) { Info "[dry run] copy $Source -> $target"; return $target }
    if (-not (Test-Path $Source)) { Fail "Expected build output is missing: $Source" }
    Copy-Item -Path $Source -Destination $target -Force
    return $target
}

function Write-Checksums([string]$Dir) {
    if ($DryRun) { return }
    $lines = Get-ChildItem -Path $Dir -File | Where-Object { $_.Extension -in '.apk', '.aab' } | ForEach-Object {
        "$((Get-FileHash -Algorithm SHA256 -Path $_.FullName).Hash.ToLower())  $($_.Name)"
    }
    Set-Content -Path (Join-Path $Dir 'SHA256SUMS.txt') -Value $lines -Encoding ASCII
}

function Test-ApkSignature([string]$Apk) {
    if ($DryRun) { return }
    if (-not $script:Apksigner) { $script:Apksigner = Find-Latest (Join-Path $script:Sdk 'build-tools') $(if ($OnWindows) { 'apksigner.bat' } else { 'apksigner' }) }
    if (-not $script:Apksigner) { Warn 'apksigner not found: skipping the signature check.'; return }
    $out = Invoke-Native $script:Apksigner @('verify', '--print-certs', $Apk) -Quiet
    $owner = $out | Where-Object { $_ -match 'certificate DN:' } | Select-Object -First 1
    Ok "Signature verified: $(Split-Path -Leaf $Apk)$(if ($owner) { ' (' + ($owner -replace '.*certificate DN:\s*', '') + ')' })"
}

function Test-AabSignature([string]$Aab) {
    if ($DryRun) { return }
    $out = Invoke-Native $script:Jarsigner @('-verify', $Aab) -AllowFail -Quiet
    if (($out -join ' ') -match 'jar verified') { Ok "Signature verified: $(Split-Path -Leaf $Aab)" }
    else { Fail "The .aab is not signed correctly: $Aab" }
}

# ---------------------------------------------------------------- phone

function Get-ConnectedPhones {
    if (-not $script:Adb) { return @() }
    $out = Invoke-Native $script:Adb @('devices') -Quiet -AllowFail
    $phones = @($out | Where-Object { $_ -match '^\S+\s+device$' } | ForEach-Object { ($_ -split '\s+')[0] })
    $unauthorized = @($out | Where-Object { $_ -match '\sunauthorized$' })
    if ($unauthorized.Count -gt 0) { Warn 'A phone is connected but not authorised: unlock it and tap "Allow" on the USB debugging prompt.' }
    return $phones
}

function Install-OnPhone([string]$Apk, [string]$Package = $AppId) {
    Step 'Installing on the phone'
    if ($DryRun) { Info "[dry run] adb install -r $Apk"; return }
    $phones = @(Get-ConnectedPhones)
    if ($phones.Count -eq 0) {
        Warn 'No phone found. Connect it by USB, turn on Developer options > USB debugging, and allow this computer.'
        return
    }
    $out = Invoke-Native $script:Adb @('install', '-r', $Apk) -AllowFail
    if (($out -join ' ') -match 'INSTALL_FAILED_UPDATE_INCOMPATIBLE') {
        Warn 'The phone has a version signed with a different key (for example debug vs release).'
        $answer = Read-Host '    Uninstall it first? Its saved settings on the phone will be removed. [y/N]'
        if ($answer -match '^[yY]') {
            Invoke-Native $script:Adb @('uninstall', $Package) -AllowFail | Out-Null
            $out = Invoke-Native $script:Adb @('install', $Apk) -AllowFail
        }
    }
    if (($out -join ' ') -match 'INSTALL_FAILED_USER_RESTRICTED|INSTALL_FAILED_ABORTED') {
        Fail 'The phone blocked the install. Turn on "Install via USB" in Developer options (Xiaomi, Oppo, Vivo and Realme need this), then try again.'
    }
    if (($out -join ' ') -match 'Success') { Ok "Installed on $($phones[0])" } else { Fail 'Installing on the phone failed (see the messages above).' }
    if ($Launch) { Start-OnPhone $Package }
}

function Start-OnPhone([string]$Package = $AppId) {
    if ($DryRun) { Info '[dry run] start the app'; return }
    Invoke-Native $script:Adb @('shell', 'monkey', '-p', $Package, '-c', 'android.intent.category.LAUNCHER', '1') -Quiet -AllowFail | Out-Null
    Ok 'App started on the phone'
}

# ---------------------------------------------------------------- tasks

function Invoke-DebugBuild {
    Initialize-Tools -NeedSdk -NeedAdb:$Install
    Step 'Compiling the debug APK'
    Invoke-Gradle @('assembleDebug')
    $apk = Copy-Output ([IO.Path]::Combine($AndroidDir, 'app', 'build', 'outputs', 'apk', 'debug', 'app-debug.apk')) 'debug' 'apk'
    Ok "Debug APK: $apk"
    Write-Checksums (Get-OutputDir)
    if ($Install) { Install-OnPhone $apk $DebugAppId }
    return $apk
}

function Invoke-ReleaseBuild {
    Initialize-Tools -NeedSdk -NeedAdb:$Install
    Step 'Release signing'
    Set-SigningEnvironment
    try {
        Step 'Compiling the signed release APK and AAB'
        Invoke-Gradle @('assembleRelease', 'bundleRelease')
    }
    finally { Clear-SigningEnvironment }

    $apk = Copy-Output ([IO.Path]::Combine($AndroidDir, 'app', 'build', 'outputs', 'apk', 'release', 'app-release.apk')) 'release' 'apk'
    $aab = Copy-Output ([IO.Path]::Combine($AndroidDir, 'app', 'build', 'outputs', 'bundle', 'release', 'app-release.aab')) 'release' 'aab'
    Step 'Checking the signatures'
    Test-ApkSignature $apk
    Test-AabSignature $aab
    Write-Checksums (Get-OutputDir)
    Ok "Release APK: $apk"
    Ok "Release AAB (upload this to Google Play): $aab"
    if ($Install) { Install-OnPhone $apk $AppId }
    return $aab
}

function Invoke-TestAab {
    Initialize-Tools -NeedSdk -NeedAdb
    $version = Get-AppVersion
    $aab = [IO.Path]::Combine($Repo, 'release', 'android', $version.Name, "$FilePrefix-$($version.Name)-release.aab")
    if (-not (Test-Path $aab) -and -not $DryRun) { Fail "No release .aab for $($version.Name) yet. Build it first:  -Task Release" }

    Step 'Installing the .aab on the phone, the way Google Play does (bundletool)'
    New-Item -ItemType Directory -Force -Path $ToolsDir | Out-Null
    $jar = Join-Path $ToolsDir "bundletool-all-$BundletoolVersion.jar"
    if (-not (Test-Path $jar)) {
        $url = "https://github.com/google/bundletool/releases/download/$BundletoolVersion/bundletool-all-$BundletoolVersion.jar"
        Info "Downloading bundletool $BundletoolVersion ..."
        if (-not $DryRun) {
            [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
            Invoke-WebRequest -Uri $url -OutFile $jar -UseBasicParsing
        }
    }
    if (-not $DryRun -and @(Get-ConnectedPhones).Count -eq 0) { Fail 'No phone found. Connect it by USB with USB debugging on.' }

    $password = Get-KeystorePassword
    $passFile = Join-Path $ToolsDir "pass-$Stamp.tmp"
    $apks = Join-Path $ToolsDir "test-$Stamp.apks"
    try {
        if (-not $DryRun) { Set-Content -Path $passFile -Value $password -NoNewline -Encoding ASCII }
        Invoke-Native $script:Java @('-jar', $jar, 'build-apks', "--bundle=$aab", "--output=$apks", '--connected-device', "--adb=$($script:Adb)",
            "--ks=$Keystore", "--ks-key-alias=$KeyAlias", "--ks-pass=file:$passFile", "--key-pass=file:$passFile") | Out-Null
        Invoke-Native $script:Java @('-jar', $jar, 'install-apks', "--apks=$apks", "--adb=$($script:Adb)") | Out-Null
    }
    finally {
        Remove-Item $passFile, $apks -ErrorAction SilentlyContinue
    }
    Ok 'Installed from the .aab'
    if ($Launch) { Start-OnPhone $AppId }
}

function Invoke-Doctor {
    Initialize-Tools -NeedSdk -NeedAdb
    Step 'Phones'
    $phones = @(Get-ConnectedPhones)
    if ($phones.Count -gt 0) { $phones | ForEach-Object { Ok "Connected: $_" } } else { Info 'No phone connected (only needed for -Install / TestAab).' }
    Step 'Signing key'
    if (Test-Path $Keystore) { Ok "Release key: $Keystore" } else { Info 'No release key yet (create it with -Task Keystore before the first release).' }
    if ($Legacy['storePassword']) { Info "The key's password is saved next to it ($LegacySigning): back up the Keys folder and keep it private." }
    elseif (Test-Path $SecretFile) { Info 'A remembered password is stored for this Windows account.' }
    $version = Get-AppVersion
    Step "App version: $($version.Name) (code $($version.Code))"
}

function Invoke-Clean {
    Initialize-Tools -NeedSdk
    Step 'Cleaning build outputs'
    Invoke-Gradle @('clean')
    Ok 'Cleaned android\app\build (release\ and your key are kept)'
}

# woodyouloveit, written the way the logo reads: wood (you) love it, with "love" in the logo's red
function Show-Banner {
    Write-Host ''
    Write-Host '  wood' -NoNewline -ForegroundColor White
    Write-Host 'you' -NoNewline -ForegroundColor Gray
    Write-Host 'love' -NoNewline -ForegroundColor Red
    Write-Host 'it' -NoNewline -ForegroundColor White
    Write-Host '   woodyouloveit.com | wooduloveit.com' -ForegroundColor DarkGray
}

function Show-Menu {
    Show-Banner
    $v = Get-AppVersion
    $where = if ($AndroidRoot) { $AndroidRoot } else { 'standard Windows folders' }
    Write-Host "  $AppName - Android build" -ForegroundColor White
    Write-Host "  App $($v.Name) (code $($v.Code))   Android folder: $where" -ForegroundColor Gray
    Write-Host '  ----------------------------------------'
    Write-Host '   1  Debug APK'
    Write-Host '   2  Debug APK, install on my phone and start it'
    Write-Host '   3  Release APK + AAB for Google Play (signed)'
    Write-Host '   4  Release, then install the APK on my phone'
    Write-Host '   5  Test the release .aab on my phone (as Google Play installs it)'
    Write-Host '   6  Create the release signing key (first time only)'
    Write-Host '   7  Check and fix my set-up (installs anything missing)'
    Write-Host '   8  Clean'
    Write-Host '   0  Exit'
    $choice = Read-Host '  Choose'
    switch ($choice) {
        '1' { $script:Task = 'Debug' }
        '2' { $script:Task = 'Debug'; $script:Install = $true; $script:Launch = $true }
        '3' { $script:Task = 'Release' }
        '4' { $script:Task = 'Release'; $script:Install = $true; $script:Launch = $true }
        '5' { $script:Task = 'TestAab'; $script:Launch = $true }
        '6' { $script:Task = 'Keystore' }
        '7' { $script:Task = 'Doctor' }
        '8' { $script:Task = 'Clean' }
        default { $script:Quit = $true }
    }
    if ($script:Task -eq 'Release' -and -not $VersionName -and $VersionCode -le 0) {
        $v = Get-AppVersion
        Write-Host ''
        Write-Host "  App version $($v.Name) (code $($v.Code)). Google Play needs a higher code for every upload."
        $newName = Read-Host "  Version name, or Enter to keep $($v.Name)"
        $newCode = Read-Host "  Version code, or Enter to keep $($v.Code)"
        $code = $v.Code
        if ($newCode -and $newCode.Trim()) {
            if (-not [int]::TryParse($newCode.Trim(), [ref]$code) -or $code -lt 1) { Fail 'The version code must be a whole number.' }
        }
        $name = if ($newName -and $newName.Trim()) { $newName.Trim() } else { $v.Name }
        if ($name -ne $v.Name -or $code -ne $v.Code) { Save-AppVersion $name $code }
    }
    if ($script:Task -in 'Release', 'TestAab' -and -not (Test-Path $SecretFile) -and -not $env:ANDROID_KEYSTORE_PASSWORD -and -not $Legacy['storePassword']) {
        $remember = Read-Host '  Remember the keystore password on this PC (encrypted for your Windows account)? [y/N]'
        if ($remember -match '^[yY]') { $script:RememberPassword = $true }
    }
}

# ---------------------------------------------------------------- main

try {
    Write-Log "$AppName Android build (woodyouloveit) - $(Get-Date -Format s) - task $Task"
    if ($Task -ne 'Menu') { Show-Banner }
    if ($Task -eq 'Menu') { Show-Menu }
    if ($Quit -or $Task -eq 'Menu') { exit 0 }
    if ($DryRun) { Warn 'Dry run: nothing is built, signed or installed.' }

    switch ($Task) {
        'Debug'    { Invoke-DebugBuild | Out-Null }
        'Release'  { Invoke-ReleaseBuild | Out-Null }
        'All'      { Invoke-DebugBuild | Out-Null; Invoke-ReleaseBuild | Out-Null }
        'Keystore' { New-Keystore }
        'TestAab'  { Invoke-TestAab }
        'Doctor'   { Invoke-Doctor }
        'Clean'    { Invoke-Clean }
    }

    $elapsed = (Get-Date) - $Started
    Write-Host ''
    Write-Host ("Done in {0:mm\:ss}. Log: {1}" -f $elapsed, $LogFile) -ForegroundColor Green
    if ($Task -in 'Debug', 'Release', 'All') {
        $dir = Get-OutputDir
        Write-Host "Files: $dir" -ForegroundColor Green
        if ($Open -and $OnWindows -and -not $DryRun) { Start-Process explorer.exe $dir }
    }
    exit 0
}
catch {
    Clear-SigningEnvironment
    Write-Host ''
    Write-Host "FAILED: $($_.Exception.Message)" -ForegroundColor Red
    Write-Log "FAILED: $($_.Exception.Message)"
    Write-Host "Log: $LogFile" -ForegroundColor Yellow
    exit 1
}
