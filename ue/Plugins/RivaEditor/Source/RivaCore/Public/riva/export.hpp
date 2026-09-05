#pragma once

// UnrealBuildTool defines RIVACORE_API when this library is built as the
// RivaCore module. Standalone CMake consumers do not need DLL annotations.
#ifndef RIVACORE_API
#define RIVACORE_API
#endif
