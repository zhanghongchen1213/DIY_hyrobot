param(
    [int]$Port = 8000,
    [switch]$BuildOnly
)

$ErrorActionPreference = 'Stop'
$docsDirectory = $PSScriptRoot
$venvPython = Join-Path $docsDirectory '.venv\Scripts\python.exe'

if (-not (Test-Path -LiteralPath $venvPython)) {
    & python -m venv (Join-Path $docsDirectory '.venv')
    if ($LASTEXITCODE -ne 0) { throw 'Unable to create the documentation Python environment.' }
}

& $venvPython -c "import importlib.util, sys; sys.exit(not all(importlib.util.find_spec(m) for m in ('sphinx', 'myst_parser', 'sphinx_rtd_theme', 'sphinx_markdown_tables')))"
if ($LASTEXITCODE -ne 0) {
    & $venvPython -m pip install -r (Join-Path $docsDirectory 'requirements.txt')
    if ($LASTEXITCODE -ne 0) { throw 'Documentation dependency installation failed.' }
}

$sourceDirectory = Join-Path $docsDirectory 'source'
$htmlDirectory = Join-Path $docsDirectory 'build\html'
& $venvPython -m sphinx -b html -E -a $sourceDirectory $htmlDirectory
if ($LASTEXITCODE -ne 0) { throw 'Documentation build failed; the preview was not started.' }

if (-not $BuildOnly) {
    Write-Host "Preview: http://127.0.0.1:$Port/ (Ctrl+C to stop)"
    & $venvPython -m http.server $Port --bind 127.0.0.1 --directory $htmlDirectory
}
