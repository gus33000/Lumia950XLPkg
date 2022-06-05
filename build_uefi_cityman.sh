#!/bin/bash

stuart_build -c Platforms/Lumia950XLPkg/PlatformBuild.Cityman.py TOOL_CHAIN_TAG=CLANG38
chmod +x ./Tools/edk2-build.Cityman.ps1 && ./Tools/edk2-build.Cityman.ps1