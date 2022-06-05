# Project Mu UEFI Implementation for Lumia 950 XL

## Build

Quick notes for building:

- Use Ubuntu 20.04 x64
- Generate ACPI tables with IASL
- Follow this quick draft

```
# Setup environment
./setup_env.sh

# Activate Workspace
python3 -m venv Lumia950XL
source Lumia950XL/bin/activate

# Build UEFI
pip install --upgrade -r pip-requirements.txt
./build_uefi.sh

# Generate ELF image
chmod +x ./Tools/edk2-build.ps1 && ./Tools/edk2-build.ps1
```

## Acknowledgements

- [EFIDroid Project](http://efidroid.org)
- Andrei Warkentin and his [RaspberryPiPkg](https://github.com/andreiw/RaspberryPiPkg)
- Sarah Purohit
- [Googulator](https://github.com/Googulator/)
- [Ben (Bingxing) Wang](https://github.com/imbushuo/)

## License

All code except drivers in `GPLDriver` directory are licensed under BSD 2-Clause. 
GPL Drivers are licensed under GPLv2 license.
