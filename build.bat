@ECHO Off
SET CURRENT_DIR=%~dp0
SET "CURRENT_DIR=%CURRENT_DIR:\=/%"

SET MSYS_BIN=%CURRENT_DIR%../../msys64/usr/bin/
SET PATH=%MSYS_BIN%;%PATH%

SET YAUL_INSTALL_ROOT=%CURRENT_DIR%../../sh2eb-elf
SET YAUL_PROG_SH_PREFIX=
SET YAUL_ARCH_SH_PREFIX=sh2eb-elf
SET YAUL_ARCH_M68K_PREFIX=m68keb-elf
SET YAUL_BUILD_ROOT=%CURRENT_DIR%/../libyaul
SET YAUL_BUILD=build
SET YAUL_CDB=0
SET YAUL_OPTION_DEV_CARTRIDGE=0
SET YAUL_OPTION_MALLOC_IMPL=tlsf
SET YAUL_OPTION_SPIN_ON_ABORT=1
SET YAUL_OPTION_BUILD_GDB=0
SET YAUL_OPTION_BUILD_ASSERT=1
SET SILENT=1
SET MAKE_ISO_XORRISO=%MSYS_BIN%/xorrisofs

make
