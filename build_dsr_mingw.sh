#!/bin/sh
# Alternative build of the Remastered dinput8.dll on Linux with clang + mingw-w64 (no Visual Studio).
# The official build is build_dsr.bat (MSVC). Release builds should come from MSVC; this script is for
# development and testing only.
#   Ubuntu/Debian: apt install clang lld mingw-w64
# SEH (__try/__except) needs -fasync-exceptions, which is only passed to the files that use it.
set -e
cd "$(dirname "$0")"
mkdir -p build/dsr-mingw/obj build/dsr-mingw/shim
# mingw ships the header in lower case.
echo '#include <tlhelp32.h>' > build/dsr-mingw/shim/TlHelp32.h
CXX="clang++ --target=x86_64-w64-windows-gnu -std=c++17 -O2 -fms-extensions -Wall -Wno-unknown-pragmas -Wno-unused-function -Ibuild/dsr-mingw/shim -Isrc/common -Isrc/dsr"
for f in src/dsr/*.cpp src/common/*.cpp; do
  n=$(basename "$f" .cpp)
  if grep -q __try "$f"; then fl="-fasync-exceptions -fexceptions"; else fl=""; fi
  # clang 18 hangs or crashes on inputdump.cpp's SEH blocks when inlining (an LLVM bug; MSVC is fine).
  if [ "$n" = "inputdump" ]; then fl="$fl -fno-inline"; fi
  echo "  $f"
  $CXX $fl -c "$f" -o "build/dsr-mingw/obj/$n.o"
done
clang++ --target=x86_64-w64-windows-gnu -shared -static -fuse-ld=lld -s build/dsr-mingw/obj/*.o \
  -o build/dsr-mingw/dinput8.dll -ldbghelp -lws2_32 -Wl,--kill-at
echo "Built build/dsr-mingw/dinput8.dll"
