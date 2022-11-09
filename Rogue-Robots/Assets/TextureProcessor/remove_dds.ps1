# Get script directory
$scriptPath = $MyInvocation.MyCommand.Path
$dir = [IO.Path]::GetDirectoryName($scriptPath) 

# Go up one folder from script directory
$realDir = (get-item $dir).parent.FullName
#Write-host "My directory is $real"

# temporarily change to the correct folder
Push-Location $realDir

Get-ChildItem -Path ".." -r *.dds | foreach { Remove-Item -Path $_.FullName }
echo "DDS textures deleted recursively from directory: $realDir"

# now back to previous directory
Pop-Location
