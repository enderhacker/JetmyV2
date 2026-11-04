param([string]$Compiler = 'g++.exe')

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$compiler = (Get-Command $Compiler -ErrorAction Stop).Source
$oldPath = $env:PATH
$directory = Join-Path $env:LOCALAPPDATA ('Temp\opencode\jetmy-startup-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $directory -Force | Out-Null
$target = Join-Path $directory 'startup_tests.exe'
try {
    $env:PATH = (Split-Path -Parent $compiler) + ';' + $oldPath
    # Some existing installations have had GCC's default manifest object renamed
    # by older compile.cmd versions. Omit it through temporary specs, not by
    # modifying the toolchain. The console test does not need a manifest.
    $specOutput = (& $compiler '-dumpspecs' 2>&1)
    if ($LASTEXITCODE -ne 0) { throw 'Cannot inspect GCC linker specs.' }
    $endfile = [regex]::Match(($specOutput -join "`n"), '(?ms)^\*endfile:\r?\n(.*?)\r?\n\r?\n')
    if (-not $endfile.Success) { throw 'Cannot locate GCC endfile linker spec.' }
    $localEndfile = $endfile.Groups[1].Value.Replace('%{!shared:%:if-exists(default-manifest.o%s)}', '').Replace('default-manifest.o%s', '')
    $specs = Join-Path $directory 'link.specs'
    Set-Content -LiteralPath $specs -Encoding ASCII -Value "*endfile:`n$localEndfile`n"
    $sources = @('tests/startup_gate.cpp', 'runtime/startup_gate.cpp') |
        ForEach-Object { Join-Path $root $_ }
    & $compiler '-std=c++17' '-O1' '-g' '-Wall' '-Wextra' "-specs=$specs" "-I$root" @sources '-o' $target '-luser32'
    if ($LASTEXITCODE -ne 0) { throw "Startup test compilation failed: $LASTEXITCODE" }
    & $target
    if ($LASTEXITCODE -ne 0) { throw "Startup tests failed: $LASTEXITCODE" }
}
finally {
    $env:PATH = $oldPath
    # Only the unique test directory created by this invocation is removed.
    Remove-Item -LiteralPath $directory -Recurse -Force
}
