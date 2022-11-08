Get-ChildItem -Path ".." -r *.png | select FullName > pngs_to_process.txt
Get-ChildItem -Path ".." -r *.jpg | select FullName > jpgs_to_process.txt


foreach($line in Get-Content .\pngs_to_process.txt) {
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

foreach($line in Get-Content .\jpgs_to_process.txt) {
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

rm pngs_to_process.txt
rm jpgs_to_process.txt