#include <Uefi.h>

#include <Configuration/Hob.h>
#include <Pi/PiFirmwareFile.h>

#include <Library/DebugLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/EfiResetSystemLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Protocol/GraphicsOutput.h>

extern EFI_GUID gPreLoaderProtocolGuid;

EFI_GRAPHICS_OUTPUT_PROTOCOL *mGop;

VOID EFIAPI BootShimVersionCheckFail(VOID)
{
  // Refresh screen
  while (TRUE) {
    gBS->Stall(EFI_TIMER_PERIOD_MILLISECONDS(1));
  }
}

EFI_STATUS
EFIAPI
PreLoaderDxeInitialize(
    IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
  PPRELOADER_ENVIRONMENT Env    = (VOID *)PRELOADER_ENV_ADDR;
  EFI_STATUS             Status = EFI_SUCCESS;

  // Protocols
  Status = gBS->LocateProtocol(
      &gEfiGraphicsOutputProtocolGuid, NULL, (VOID **)&mGop);
  ASSERT_EFI_ERROR(Status);

  // Library init
  Status = LibInitializeResetSystem(ImageHandle, SystemTable);
  ASSERT_EFI_ERROR(Status);

  // Version requirement
  if (Env->Header != PRELOADER_HEADER ||
      Env->PreloaderVersion < PRELOADER_VERSION_MIN) {
    BootShimVersionCheckFail();
    Status = EFI_UNSUPPORTED;
    goto exit;
  }

  // Install variables
  Status = gRT->SetVariable(
      L"UEFIDisplayInfo", &gEfiGraphicsOutputProtocolGuid,
      EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
      sizeof(UINT32) * 30, (VOID *)Env->UefiDisplayInfo);
  ASSERT_EFI_ERROR(Status);

  // Install protocol
  Status = gBS->InstallProtocolInterface(
      &ImageHandle, &gPreLoaderProtocolGuid, EFI_NATIVE_INTERFACE,
      (void *)&ImageHandle);
  ASSERT_EFI_ERROR(Status);

exit:
  return Status;
}
