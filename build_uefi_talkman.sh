#!/bin/bash

stuart_build -c Platforms/Lumia950XLPkg/PlatformBuild.Talkman.py TOOL_CHAIN_TAG=CLANG38
chmod +x ./Tools/edk2-build.Talkman.ps1 && ./Tools/edk2-build.Talkman.ps1