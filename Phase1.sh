#!/bin/bash

chmod +x ./build_releaseinfo.ps1 && ./build_releaseinfo.ps1
stuart_setup -c Platforms/Lumia950XLPkg/PlatformBuild.py TOOL_CHAIN_TAG=CLANG38
stuart_update -c Platforms/Lumia950XLPkg/PlatformBuild.py TOOL_CHAIN_TAG=CLANG38