# PowerShell script to create a standalone version of the game
# This script copies all necessary files to a "Standalone" folder

param(
    [string]$BuildConfig = "Release",
    [string]$Platform = "x64",
    [string]$OutputFolder = "Standalone"
)

Write-Host "Creating standalone version of Word Puzzle Game..." -ForegroundColor Green
Write-Host "Build Configuration: $BuildConfig" -ForegroundColor Cyan
Write-Host "Platform: $Platform" -ForegroundColor Cyan
Write-Host "Output Folder: $OutputFolder" -ForegroundColor Cyan
Write-Host ""

# Determine the executable path based on build configuration
$ExePath = "x64\$BuildConfig\SFML_TestProject.exe"

# Check if executable exists
if (-not (Test-Path $ExePath)) {
    Write-Host "ERROR: Executable not found at: $ExePath" -ForegroundColor Red
    Write-Host "Please build the project first in $BuildConfig $Platform configuration." -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Available executables:" -ForegroundColor Yellow
    Get-ChildItem -Recurse -Filter "SFML_TestProject.exe" | ForEach-Object { Write-Host "  $($_.FullName)" }
    exit 1
}

Write-Host "Found executable: $ExePath" -ForegroundColor Green

# Create output folder
if (Test-Path $OutputFolder) {
    Write-Host "Removing existing $OutputFolder folder..." -ForegroundColor Yellow
    Remove-Item -Path $OutputFolder -Recurse -Force
}
New-Item -ItemType Directory -Path $OutputFolder | Out-Null
Write-Host "Created output folder: $OutputFolder" -ForegroundColor Green

# Copy executable
Write-Host "Copying executable..." -ForegroundColor Cyan
Copy-Item -Path $ExePath -Destination "$OutputFolder\SFML_TestProject.exe" -Force

# Copy fonts folder
Write-Host "Copying fonts folder..." -ForegroundColor Cyan
if (Test-Path "fonts") {
    Copy-Item -Path "fonts" -Destination "$OutputFolder\fonts" -Recurse -Force
    Write-Host "  Fonts copied" -ForegroundColor Green
} else {
    Write-Host "  WARNING: fonts folder not found!" -ForegroundColor Red
}

# Copy assets folder
Write-Host "Copying assets folder..." -ForegroundColor Cyan
if (Test-Path "assets") {
    Copy-Item -Path "assets" -Destination "$OutputFolder\assets" -Recurse -Force
    Write-Host "  Assets copied" -ForegroundColor Green
} else {
    Write-Host "  WARNING: assets folder not found!" -ForegroundColor Red
}

# Copy data files
Write-Host "Copying data files..." -ForegroundColor Cyan
$dataFiles = @("words_processed.csv")
foreach ($file in $dataFiles) {
    if (Test-Path $file) {
        Copy-Item -Path $file -Destination "$OutputFolder\$file" -Force
        Write-Host "  $file copied" -ForegroundColor Green
    } else {
        Write-Host "  WARNING: $file not found!" -ForegroundColor Red
    }
}

# Check if we need SFML DLLs (only for Debug builds - Release uses static linking)
if ($BuildConfig -eq "Debug") {
    Write-Host ""
    Write-Host "Checking for SFML DLLs (Debug build requires DLLs)..." -ForegroundColor Cyan
    
    # Common SFML DLL locations
    $sfmlPaths = @(
        "C:\Libraries\SFML\SFML-3.0.0\bin",
        "C:\Libraries\64BitSFML\SFML-3.0.0\bin",
        "C:\SFML\bin",
        "C:\SFML-3.0.0\bin"
    )
    
    $dllsFound = $false
    foreach ($sfmlPath in $sfmlPaths) {
        if (Test-Path $sfmlPath) {
            Write-Host "  Found SFML at: $sfmlPath" -ForegroundColor Green
            $dlls = Get-ChildItem -Path $sfmlPath -Filter "*.dll"
            foreach ($dll in $dlls) {
                if ($dll.Name -match "sfml") {
                    Copy-Item -Path $dll.FullName -Destination "$OutputFolder\$($dll.Name)" -Force
                    Write-Host "    Copied $($dll.Name)" -ForegroundColor Green
                    $dllsFound = $true
                }
            }
        }
    }
    
    if (-not $dllsFound) {
        Write-Host ""
        Write-Host "  WARNING: Could not find SFML DLLs automatically!" -ForegroundColor Yellow
        Write-Host "  Debug builds require SFML DLLs. Copy these DLLs manually:" -ForegroundColor Yellow
        Write-Host "    - sfml-graphics-3.dll" -ForegroundColor Yellow
        Write-Host "    - sfml-audio-3.dll" -ForegroundColor Yellow
        Write-Host "    - sfml-window-3.dll" -ForegroundColor Yellow
        Write-Host "    - sfml-system-3.dll" -ForegroundColor Yellow
        Write-Host "    - sfml-network-3.dll (if used)" -ForegroundColor Yellow
        Write-Host ""
        Write-Host "  They should be in your SFML installation's bin folder." -ForegroundColor Yellow
        Write-Host "  Or rebuild in Release configuration for a self-contained executable." -ForegroundColor Yellow
    }
} else {
    Write-Host ""
    Write-Host "Release build detected - using static linking (no DLLs needed)" -ForegroundColor Green
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "Standalone version created successfully!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""
Write-Host "Location: $OutputFolder" -ForegroundColor Cyan
Write-Host ""
Write-Host "To run the game:" -ForegroundColor Yellow
Write-Host "  1. Navigate to the $OutputFolder folder" -ForegroundColor White
Write-Host "  2. Double-click SFML_TestProject.exe" -ForegroundColor White
Write-Host ""
Write-Host "Required files structure:" -ForegroundColor Yellow
Write-Host "  $OutputFolder\" -ForegroundColor White
Write-Host "    SFML_TestProject.exe" -ForegroundColor White
Write-Host "    fonts\arialbd.ttf" -ForegroundColor White
Write-Host "    assets\ (all images, sounds, music)" -ForegroundColor White
Write-Host "    words_processed.csv" -ForegroundColor White
Write-Host ""
