#include <PiDxe.h>
#include <Uefi.h>

#include <Configuration/Hob.h>
#include <Pi/PiFirmwareFile.h>

#include <Library/BaseLib.h>
#include <Library/BmpSupportLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/EfiResetSystemLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/Bmp.h>
#include <Protocol/AcpiTable.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/GraphicsOutput.h>

#include <Library/BgraRgbaConvert.h>
#include <Library/lodepng.h>
#include <LittleVgl/core/lv_core/lv_refr.h>
#include <LittleVgl/core/lvgl.h>
#include <LittleVgl/lv_conf.h>

STATIC EFI_GUID gUnsupportedImageGuid = {
    0x5fdf5e3c,
    0x6b70,
    0x4acc,
    {
        0x83,
        0x30,
        0x63,
        0xa6,
        0x73,
        0x92,
        0x4a,
        0x46,
    },
};

STATIC EFI_GUID gAcpiTablesPsci = {
    0x254e377e,
    0x018e,
    0xee4f,
    {
        0x87,
        0xf2,
        0x39,
        0xc,
        0x23,
        0xc6,
        0x6,
        0xb1,
    },
};

STATIC EFI_GUID gAcpiTablesMpPark = {
    0x254e377e,
    0x018e,
    0xee4f,
    {
        0x87,
        0xf2,
        0x39,
        0xc,
        0x23,
        0xc6,
        0x6,
        0xb2,
    },
};

STATIC EFI_GUID gAcpiTablesMpParkQuadCore = {
    0x254e377e,
    0x018e,
    0xee4f,
    {
        0x87,
        0xf2,
        0x39,
        0xc,
        0x23,
        0xc6,
        0x6,
        0xb3,
    },
};

STATIC EFI_GUID gAcpiTablesEmmcBoot = {
    0x254e377e,
    0x018e,
    0xee4f,
    {
        0x87,
        0xf2,
        0x39,
        0xc,
        0x23,
        0xc6,
        0x6,
        0xb4,
    },
};

STATIC EFI_GUID gAcpiTablesSdBoot = {
    0x254e377e,
    0x018e,
    0xee4f,
    {
        0x87,
        0xf2,
        0x39,
        0xc,
        0x23,
        0xc6,
        0x6,
        0xb5,
    },
};

extern EFI_GUID gPreLoaderProtocolGuid;

EFI_GRAPHICS_OUTPUT_PROTOCOL *mGop;
lv_disp_drv_t                 mDispDrv;
lv_indev_drv_t                mFakeInputDrv;

static void EfiGopBltFlush(
    int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t *color_p)
{
  mGop->Blt(
      mGop, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)color_p, EfiBltBufferToVideo, 0, 0,
      x1, y1, x2 - x1 + 1, y2 - y1 + 1, 0);

  lv_flush_ready();
}

static bool FakeInputRead(lv_indev_data_t *data) { return false; }

VOID EFIAPI BootShimVersionCheckFail(VOID)
{
  EFI_STATUS Status;
  VOID *     ImageData;
  UINTN      ImageSize;

  lv_img_dsc_t   png_dsc;
  unsigned char *DecodedData;
  unsigned int   Width;
  unsigned int   Height;
  UINT32         DecoderError;

  Status = GetSectionFromAnyFv(
      &gUnsupportedImageGuid, EFI_SECTION_RAW, 0, &ImageData, &ImageSize);
  ASSERT_EFI_ERROR(Status);

  // Prepare LittleVGL
  lv_init();

  lv_disp_drv_init(&mDispDrv);
  mDispDrv.disp_flush = EfiGopBltFlush;
  lv_disp_drv_register(&mDispDrv);

  lv_indev_drv_init(&mFakeInputDrv);
  mFakeInputDrv.type = LV_INDEV_TYPE_POINTER;
  mFakeInputDrv.read = FakeInputRead;
  lv_indev_drv_register(&mFakeInputDrv);

  // Decode PNG
  DecoderError = lodepng_decode32(
      &DecodedData, &Width, &Height, (unsigned char *)ImageData,
      (size_t)ImageSize);
  if (!DecoderError) {
    ConvertBetweenBGRAandRGBA(DecodedData, Width, Height);
    png_dsc.header.always_zero = 0;                 /*It must be zero*/
    png_dsc.header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA; /*Set the color format*/
    png_dsc.header.w  = Width;
    png_dsc.header.h  = Height;
    png_dsc.data_size = Width * Height * 4;
    png_dsc.data      = DecodedData;

    // Present image
    lv_obj_t *img1 = lv_img_create(lv_scr_act(), NULL);
    lv_img_set_src(img1, &png_dsc);
    lv_obj_set_size(img1, 1080, 1920);
    lv_obj_align(img1, NULL, LV_ALIGN_CENTER, 0, 0);
  }

  // Refresh screen
  while (TRUE) {
    lv_tick_inc(1);
    lv_task_handler();
    gBS->Stall(EFI_TIMER_PERIOD_MILLISECONDS(1));
  }
}

/**
  Locate the first instance of a protocol.  If the protocol requested is an
  FV protocol, then it will return the first FV that contains the ACPI table
  storage file.
  @param  Guid          The guid of the ACPI storage file
  @param  Instance      Return pointer to the first instance of the protocol
  @return EFI_SUCCESS           The function completed successfully.
  @return EFI_NOT_FOUND         The protocol could not be located.
  @return EFI_OUT_OF_RESOURCES  There are not enough resources to find the
protocol.
**/
EFI_STATUS
LocateFvInstanceWithTables(
    IN EFI_GUID Guid, OUT EFI_FIRMWARE_VOLUME2_PROTOCOL **Instance)
{
  EFI_STATUS                     Status;
  EFI_HANDLE *                   HandleBuffer;
  UINTN                          NumberOfHandles;
  EFI_FV_FILETYPE                FileType;
  UINT32                         FvStatus;
  EFI_FV_FILE_ATTRIBUTES         Attributes;
  UINTN                          Size;
  UINTN                          Index;
  EFI_FIRMWARE_VOLUME2_PROTOCOL *FvInstance;

  FvStatus = 0;

  DEBUG((EFI_D_ERROR, "Locating gEfiFirmwareVolume2ProtocolGuid protocol\n"));
  //
  // Locate protocol.
  //
  Status = gBS->LocateHandleBuffer(
      ByProtocol, &gEfiFirmwareVolume2ProtocolGuid, NULL, &NumberOfHandles,
      &HandleBuffer);
  if (EFI_ERROR(Status)) {
    //
    // Defined errors at this time are not found and out of resources.
    //
    return Status;
  }

  //
  // Looking for FV with ACPI storage file
  //

  for (Index = 0; Index < NumberOfHandles; Index++) {
    DEBUG((EFI_D_ERROR, "Handle gEfiFirmwareVolume2ProtocolGuid protocol\n"));
    //
    // Get the protocol on this handle
    // This should not fail because of LocateHandleBuffer
    //
    Status = gBS->HandleProtocol(
        HandleBuffer[Index], &gEfiFirmwareVolume2ProtocolGuid,
        (VOID **)&FvInstance);
    ASSERT_EFI_ERROR(Status);

    DEBUG((EFI_D_ERROR, "Read file\n"));
    //
    // See if it has the ACPI storage file
    //
    Status = FvInstance->ReadFile(
        FvInstance, &Guid, NULL, &Size, &FileType, &Attributes, &FvStatus);

    //
    // If we found it, then we are done
    //
    if (Status == EFI_SUCCESS) {
      *Instance = FvInstance;
      break;
    }
  }

  //
  // Our exit status is determined by the success of the previous operations
  // If the protocol was found, Instance already points to it.
  //

  //
  // Free any allocated buffers
  //
  gBS->FreePool(HandleBuffer);

  return Status;
}

/**
  This function calculates and updates an UINT8 checksum.
  @param  Buffer          Pointer to buffer to checksum
  @param  Size            Number of bytes to checksum
**/
VOID AcpiPlatformChecksum(IN UINT8 *Buffer, IN UINTN Size)
{
  UINTN ChecksumOffset;

  ChecksumOffset = OFFSET_OF(EFI_ACPI_DESCRIPTION_HEADER, Checksum);

  //
  // Set checksum to 0 first
  //
  Buffer[ChecksumOffset] = 0;

  //
  // Update checksum value
  //
  Buffer[ChecksumOffset] = CalculateCheckSum8(Buffer, Size);
}

EFI_STATUS
EFIAPI
LoadAcpiTablesFromGuid(IN EFI_GUID Guid)
{
  EFI_STATUS                     Status;
  EFI_ACPI_TABLE_PROTOCOL *      AcpiTable;
  EFI_FIRMWARE_VOLUME2_PROTOCOL *FwVol;
  INTN                           Instance;
  EFI_ACPI_COMMON_HEADER *       CurrentTable;
  UINTN                          TableHandle;
  UINT32                         FvStatus;
  UINTN                          TableSize;
  UINTN                          Size;

  Instance     = 0;
  CurrentTable = NULL;
  TableHandle  = 0;

  DEBUG((EFI_D_ERROR, "Locating AcpiTable protocol\n"));
  //
  // Find the AcpiTable protocol
  //
  Status = gBS->LocateProtocol(
      &gEfiAcpiTableProtocolGuid, NULL, (VOID **)&AcpiTable);
  if (EFI_ERROR(Status)) {
  DEBUG((EFI_D_ERROR, "Locating AcpiTable protocol failed\n"));
    return EFI_ABORTED;
  }

  DEBUG((EFI_D_ERROR, "Locating firmware volume protocol\n"));
  //
  // Locate the firmware volume protocol
  //
  Status = LocateFvInstanceWithTables(Guid, &FwVol);
  if (EFI_ERROR(Status)) {
  DEBUG((EFI_D_ERROR, "Locate the firmware volume protocol failed\n"));
    return EFI_ABORTED;
  }
  //
  // Read tables from the storage file.
  //
  while (Status == EFI_SUCCESS) {

  DEBUG((EFI_D_ERROR, "Read section\n"));
    Status = FwVol->ReadSection(
        FwVol, &Guid, EFI_SECTION_RAW, Instance, (VOID **)&CurrentTable, &Size,
        &FvStatus);
    if (!EFI_ERROR(Status)) {
      //
      // Add the table
      //
      TableHandle = 0;

      TableSize = ((EFI_ACPI_DESCRIPTION_HEADER *)CurrentTable)->Length;
      ASSERT(Size >= TableSize);

      //
      // Checksum ACPI table
      //
      AcpiPlatformChecksum((UINT8 *)CurrentTable, TableSize);

  DEBUG((EFI_D_ERROR, "Install table\n"));
      //
      // Install ACPI table
      //
      Status = AcpiTable->InstallAcpiTable(
          AcpiTable, CurrentTable, TableSize, &TableHandle);

      //
      // Free memory allocated by ReadSection
      //
      gBS->FreePool(CurrentTable);

      if (EFI_ERROR(Status)) {
  DEBUG((EFI_D_ERROR, "Install table failed\n"));
        return EFI_ABORTED;
      }

      //
      // Increment the instance
      //
      Instance++;
      CurrentTable = NULL;
    }
  }

  //
  // The driver does not require to be kept loaded.
  //
  return EFI_REQUEST_UNLOAD_IMAGE;
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

  switch (Env->BootMode) {
  case BOOT_MODE_MPPARK:
  case BOOT_MODE_MPPARK_EL2: {
    if (Env->UseQuadCoreConfiguration) {
      Status = LoadAcpiTablesFromGuid(gAcpiTablesMpParkQuadCore);
      ASSERT_EFI_ERROR(Status);
    }
    else {
      Status = LoadAcpiTablesFromGuid(gAcpiTablesMpPark);
      ASSERT_EFI_ERROR(Status);
    }
    break;
  }
  case BOOT_MODE_PSCI:
  default: {
    Status = LoadAcpiTablesFromGuid(gAcpiTablesPsci);
    ASSERT_EFI_ERROR(Status);
    break;
  }
  }

  if (Env->EnablePlatformSdCardBoot) {
    Status = LoadAcpiTablesFromGuid(gAcpiTablesSdBoot);
    ASSERT_EFI_ERROR(Status);
  }
  else {
    Status = LoadAcpiTablesFromGuid(gAcpiTablesEmmcBoot);
    ASSERT_EFI_ERROR(Status);
  }

  // Install protocol
  Status = gBS->InstallProtocolInterface(
      &ImageHandle, &gPreLoaderProtocolGuid, EFI_NATIVE_INTERFACE,
      (void *)&ImageHandle);
  ASSERT_EFI_ERROR(Status);

exit:
  return Status;
}
