#ifndef __LOCAL_HOB_H__
#define __LOCAL_HOB_H__

#define PRELOADER_ENV_ADDR 0xb0000000
#define PRELOADER_VERSION_MIN 0x1001

#define PRELOADER_HEADER SIGNATURE_32('B', 'S', 'E', 'N')

#define BOOT_MODE_PSCI       0
#define BOOT_MODE_MPPARK     1
#define BOOT_MODE_MPPARK_EL2 2

typedef struct _PRELOADER_ENVIRONMENT {
  UINT32   Header;
  UINT32   PreloaderVersion;
  CHAR8    PreloaderRelease[64];
  EFI_TIME BootTimeEpoch;
  UINT32   UefiDisplayInfo[30];
  UINT32   BootMode;
  UINT32   EnablePlatformSdCardBoot;
  UINT32   UseQuadCoreConfiguration;
  UINT32   Crc32;
} PRELOADER_ENVIRONMENT, *PPRELOADER_ENVIRONMENT;

#endif