<#
.SYNOPSIS
    Configures and builds this checkout with Ninja and sccache (MSVC, x64).

.DESCRIPTION
    Run from any checkout (main or a worktree); it builds into <checkout>\build. All checkouts share
    one sccache cache: SCCACHE_BASEDIRS strips the checkout path before hashing, so the same source
    compiled in another worktree is a cache hit.

    Paths come from environment variables (defaults are this machine's):
      MOP_BOOST_ROOT    C:\local\boost_1_85_0
      MOP_OPENSSL_ROOT  C:\Program Files\OpenSSL-Win64
      MOP_MYSQL_ROOT    C:\Program Files\MySQL\MySQL Server 8.0
      MOP_REPO_ROOT     the main checkout (parent of .worktrees), used for SCCACHE_BASEDIRS
      SCCACHE_CACHE_SIZE  default 30G

.EXAMPLE
    tools\build\build.ps1                      # configure if needed, build everything
    tools\build\build.ps1 -Target worldserver  # only the world server
    tools\build\build.ps1 -Tools -Reconfigure  # also build the map extractors
#>
param(
    [string]$Config = 'RelWithDebInfo',
    [string]$Target = 'all',
    [switch]$Tools,
    [switch]$NoPch,
    [switch]$Reconfigure,
    [switch]$NoWait
)

# Not 'Stop': Windows PowerShell 5.1 turns any stderr line of a native tool (CMake warnings) into a
# terminating error. Native steps check $LASTEXITCODE; cmdlets that must not fail use -ErrorAction Stop.
$ErrorActionPreference = 'Continue'

$checkout = (Resolve-Path (Join-Path $PSScriptRoot '..\..') -ErrorAction Stop).Path
$buildDir = Join-Path $checkout 'build'

function Get-EnvOrDefault([string]$name, [string]$default)
{
    $value = [Environment]::GetEnvironmentVariable($name)
    if ([string]::IsNullOrEmpty($value)) { return $default }
    return $value
}

$boostRoot = Get-EnvOrDefault 'MOP_BOOST_ROOT' 'C:\local\boost_1_85_0'
$opensslRoot = Get-EnvOrDefault 'MOP_OPENSSL_ROOT' 'C:\Program Files\OpenSSL-Win64'
$mysqlRoot = Get-EnvOrDefault 'MOP_MYSQL_ROOT' 'C:\Program Files\MySQL\MySQL Server 8.0'
$repoRoot = Get-EnvOrDefault 'MOP_REPO_ROOT' ($(if ($checkout -match '^(.*)\\\.worktrees\\[^\\]+$') { $Matches[1] } else { $checkout }))

foreach ($tool in 'sccache', 'ninja', 'cmake')
{
    if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) { throw "$tool is not on PATH (scoop install sccache ninja)" }
}

# MSVC environment (cl, link, INCLUDE, LIB) from the newest Visual Studio with the C++ tools.
if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue))
{
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $vs) { throw 'No Visual Studio with the C++ x64 tools found' }
    $vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
    # vcvars64 calls vswhere itself and expects it on PATH
    $env:PATH = "$(Split-Path $vswhere);$env:PATH"
    cmd /c "`"$vcvars`" >nul 2>nul && set" | ForEach-Object {
        if ($_ -match '^([^=]+)=(.*)$') { [Environment]::SetEnvironmentVariable($Matches[1], $Matches[2]) }
    }
}

# The machine-wide BOOST_ROOT points at an older Boost; dep/boost reads the environment variable.
$env:BOOST_ROOT = $boostRoot

# sccache reads these when its server starts, so restart it if it runs with other settings.
# Every agent worktree, so the same source compiled in any of them is a cache hit in the others.
$worktreeRoot = Join-Path $repoRoot '.worktrees'
$worktrees = @(if (Test-Path $worktreeRoot) { Get-ChildItem $worktreeRoot -Directory | ForEach-Object { $_.FullName } })
$basedirs = (@($repoRoot) + $worktrees) -join ';'
$cacheSize = Get-EnvOrDefault 'SCCACHE_CACHE_SIZE' '30G'
if ($env:SCCACHE_BASEDIRS -ne $basedirs -or $env:SCCACHE_CACHE_SIZE -ne $cacheSize)
{
    $env:SCCACHE_BASEDIRS = $basedirs
    $env:SCCACHE_CACHE_SIZE = $cacheSize
    sccache --stop-server 2>$null | Out-Null
}
sccache --start-server 2>$null | Out-Null

$cache = Join-Path $buildDir 'CMakeCache.txt'
if ($Reconfigure -or -not (Test-Path $cache) -or -not (Select-String -Path $cache -Pattern '^CMAKE_GENERATOR:INTERNAL=Ninja$' -Quiet))
{
    if ((Test-Path $cache) -and -not (Select-String -Path $cache -Pattern '^CMAKE_GENERATOR:INTERNAL=Ninja$' -Quiet))
    {
        # A Visual Studio build lives here; CMake cannot switch generators in place.
        Remove-Item $cache, (Join-Path $buildDir 'CMakeFiles') -Recurse -Force -ErrorAction Stop
    }

    $libs = "$opensslRoot\lib\VC\x64".Replace('\', '/')
    $onOff = { param($b) if ($b) { 'ON' } else { 'OFF' } }
    $configureArgs = @(
        '-S', $checkout, '-B', $buildDir, '-G', 'Ninja',
        "-DCMAKE_BUILD_TYPE=$Config",
        '-DCMAKE_C_COMPILER=cl', '-DCMAKE_CXX_COMPILER=cl',
        '-DCMAKE_C_COMPILER_LAUNCHER=sccache', '-DCMAKE_CXX_COMPILER_LAUNCHER=sccache',
        # sccache cannot cache objects that share one PDB (/Zi); embed the debug info instead (/Z7).
        '-DCMAKE_POLICY_DEFAULT_CMP0141=NEW', '-DCMAKE_MSVC_DEBUG_INFORMATION_FORMAT=Embedded',
        "-DTOOLS=$(& $onOff $Tools)",
        "-DUSE_COREPCH=$(& $onOff (-not $NoPch))", "-DUSE_SCRIPTPCH=$(& $onOff (-not $NoPch))",
        "-DBOOST_ROOT=$($boostRoot.Replace('\', '/'))",
        "-DBOOST_LIBRARYDIR=$($boostRoot.Replace('\', '/'))/lib64-msvc-14.3",
        # cmake/macros/FindOpenSSL.cmake does not know the installer's lib\VC\x64\MD layout.
        "-DOPENSSL_ROOT_DIR=$($opensslRoot.Replace('\', '/'))",
        "-DLIB_EAY_RELEASE=$libs/MD/libcrypto.lib", "-DLIB_EAY_DEBUG=$libs/MDd/libcrypto.lib",
        "-DSSL_EAY_RELEASE=$libs/MD/libssl.lib", "-DSSL_EAY_DEBUG=$libs/MDd/libssl.lib"
    )
    & cmake @configureArgs
    if ($LASTEXITCODE) { throw "CMake configure failed ($LASTEXITCODE)" }
}

# One build at a time on this machine. Two overlapping builds with precompiled headers run out of memory and
# swap (6 cores, 32 GB: about 11 s per file alone, 67 s each when two overlap), so queued builds finish sooner.
# -NoWait skips the queue.
$lock = New-Object System.Threading.Mutex($false, 'Local\UltimateMoP-build')
$owned = $false
if (-not $NoWait)
{
    try { $owned = $lock.WaitOne(0) } catch [System.Threading.AbandonedMutexException] { $owned = $true }
    if (-not $owned)
    {
        Write-Host 'Another build is running on this machine; waiting for it to finish (-NoWait skips this).'
        $waitStart = Get-Date
        try { $owned = $lock.WaitOne() } catch [System.Threading.AbandonedMutexException] { $owned = $true }
        Write-Host "Waited $([math]::Round(((Get-Date) - $waitStart).TotalMinutes, 1)) min."
    }
}

$start = Get-Date
try
{
    & cmake --build $buildDir --target $Target
    $exit = $LASTEXITCODE
}
finally
{
    if ($owned) { $lock.ReleaseMutex() }
    $lock.Dispose()
}
$minutes = [math]::Round(((Get-Date) - $start).TotalMinutes, 1)

# The servers load these at run time; copy them next to the binaries.
$bin = Join-Path $buildDir 'bin'
if (Test-Path $bin)
{
    Copy-Item "$mysqlRoot\lib\libmysql.dll" $bin -Force
    # legacy.dll is the OpenSSL legacy provider (ARC4 for the world session); without it the first
    # client connection crashes while SOAP still works. The core loads providers from the exe folder.
    Copy-Item "$opensslRoot\bin\libcrypto-3-x64.dll", "$opensslRoot\bin\libssl-3-x64.dll", "$opensslRoot\bin\legacy.dll" $bin -Force
}

sccache --show-stats | Select-String -Pattern 'Compile requests|Cache hits|Cache misses|Non-cacheable|not cached|Cache size' | ForEach-Object { $_.Line }
Write-Host "Build $(if ($exit) { 'FAILED' } else { 'finished' }) in $minutes min."
exit $exit
