# Usage:
#   Powershell -File GenerateVersion.ps1 full\path\to\Version.h
#
# If you're having trouble building this project due to this script, read on!
#
# By default, you can't actually execute PowerShell scripts without running
# `Set-ExecutionPolicy FOO`, where FOO is "Unrestricted" to allow running
# any script, or RemoteSigned/AllSigned if we wanted to commit a signed
# version of the script to our repo.
#
# https://msdn.microsoft.com/powershell/reference/5.1/Microsoft.PowerShell.Core/about/about_Execution_Policies
#
# You only have to run this once per machine. If this proves to be a problem
# in practice, we can rewrite this in BAT or otherwise rethink it.

# Fail if any command fails. Equivalent to `set -e` in Bash.
$ErrorActionPreference = 'Stop'

$versionH = $Args[0];
echo "Generating $versionH..."

$describe = (git describe)

# We expect $describe to be of the form:
#    VERSION_W.X.Y.Z-[number of commits ahead]-[commit hash]
if (!$describe.StartsWith("VERSION_")) {
    echo "$describe doesn't start with VERSION_. Do you have some stray signed tag?"
    exit(1)
}

$version = $describe.Substring("VERSION_".length)

# We use the four integral parts of the version in the .rc file,
# which shows up in the .exe's Properties page and tooltip
$bits = $version.Split('-')[0].split('.')
$w = $bits[0]
$x = $bits[1]
$y = $bits[2]
$z = $bits[3]

$header = @"
#pragma once

#define RELEASE_VER_MAIN    $w
#define RELEASE_VER_MAIN2   $x
#define RELEASE_VER_SUB     $y
#define RELEASE_VER_SUB2    $z

#define RELEASE_VER_STR "$version"
"@

# echo $header
[IO.File]::WriteAllLines($versionH, $header);
