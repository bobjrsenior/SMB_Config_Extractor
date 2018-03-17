# SMB_Config_Extractor
Used to extract config files from Super Monkey Ball (SMB1) and Super Monkey Ball 2 (SMB2) levels. Config files are used for custom level making.

## Support

### Filetypes

Supports raw lz files and compressed lz files. If a compressed lz file is passed (.lz extension), it will automatically be decompressed.

### SMB1

smbcnv Configs: Supports all current SMB1 config items.

XML configs: No support

### SMB2

smbcnv Configs: Animation isn't supported. Most other basic things are.

XML configs: Most things are supported. A few things are missing until the XML standard solidifies.

## Usage

### Non-Command Line

Just drag the file to extract from on the executable.

### Command Line
   
    Usage: ./SMB_LZ_Tool [(FLAG | FILE)...]
    Flags:
        -help      Show this help
        -h

        -legacy    Use the old config extractor for smbcnv style configs
        -l

        -new       Use the new config extractor for xml style configs (default)
        -n
