# Get script directory
$scriptPath = $MyInvocation.MyCommand.Path
$dir = [IO.Path]::GetDirectoryName($scriptPath) 

# Go up one folder from script directory    --> Assuming this script is in a subfolder in the Assets folder
$realDir = (get-item $dir).parent.FullName

# Temporarily change working directory
Push-Location $dir

Write-Output "Real Dir: $dir"

# --> Assuming this script is in a subfolder in the Assets folder
Get-ChildItem -Path ".." -r *.png | Resolve-Path -Relative > pngs_to_process.txt
Get-ChildItem -Path ".." -r *.jpg | Resolve-Path -Relative > jpgs_to_process.txt

echo "Texture assets to generate have recursively been searched from directory: $realDir"

$pngContent = Get-Content .\pngs_to_process.txt
$jpgContent = Get-Content .\jpgs_to_process.txt

# Get to script directory
Push-Location $dir

foreach($line in $pngContent) 
{
    if($line -match $regex)
    {
        # Remove whitspace
        $line = $line -replace '\s',''

        $outputLine = $line
        $outputLine = [IO.Path]::ChangeExtension($outputLine, "dds")

        $ext = [IO.Path]::GetExtension($line)
        if ($ext -eq ".png")
        {
            $exists = Test-Path -Path $outputLine -PathType Leaf
            if ($exists -eq $true)
            {
                continue
            }

            $dir = [IO.Path]::GetDirectoryName($line) 
            &.\texconv -pow2 -f BC7_UNORM_SRGB $line -srgb -y -m 1 -o $dir
        }
    }
}

foreach($line in $jpgContent) 
{
    if($line -match $regex)
    {
        # Remove whitspace
        $line = $line -replace '\s',''
        $ext = [IO.Path]::GetExtension($line)

        
        $outputLine = $line
        $outputLine = [IO.Path]::ChangeExtension($outputLine, "dds")


        if ($ext -eq ".jpg")
        {
            $exists = Test-Path -Path $outputLine -PathType Leaf
            if ($exists -eq $true)
            {
                continue
            }

            $dir = [IO.Path]::GetDirectoryName($line)
            &.\texconv -pow2 -f BC7_UNORM_SRGB $line -srgb -y -m 1 -o $dir

        }
    }
}


# Clean up the temporary file list
# Temporarily change working directory
$dir = [IO.Path]::GetDirectoryName($scriptPath) 
Push-Location $dir

# If crash --> Check the file paths..
# Text files may omitt the whole path and choose to post-fix ".." and not finish the file name!
rm pngs_to_process.txt
rm jpgs_to_process.txt

echo "Generating DDS assets done recursively from directory: $realDir"

