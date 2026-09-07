// HvcPatch.c: hypervisor patch that traps SMC call and handles PSCI requests
// Copyright (c) 2020 Sarah Purohit, Bingxing Wang

// Some background story: it seems that Windows build higher than 19041 (not
// including 19041) Messed up the HVC-based PSCI call, although it boots fine on
// Qemu/KVM platform. There are undisclosed platforms also reporting issues with
// HVC-based PSCI call. But on 8992/8994 devices, it completely hangs the boot
// process (ACPI.sys refused to enumerate devices)

// While the root cause is not fully investigated, the patch enables booting
// Windows versions higher than 19041. The patch assumes BSP version 1078, which
// is the last production release and should be present on most firmwares.
// Due to the nature of engineering device, Hapenaro might have multiple
// development firmware around the world. I hope they are compatible with the
// research conducted on prod 8994 BSP. If the patch blows up on Hapenaro, users
// are expected to fix by themselves (prototype users are advanced users, aren't
// they?)

#ifndef __HVCPATCH_H__
#define __HVCPATCH_H__

#if defined(SILICON_PLATFORM)

// Patch location are identical between platforms.
#define WAKE_FROM_POWERGATE_PATCH_ADDR 0x6C001F8
#define LOWER_EL_SYNC_EXC_64B_PATCH_ADDR 0x6C08C24
#define LOWER_EL_SYNC_EXC_32B_PATCH_ADDR 0x6C08E24
#define LSE_PATCH_CODE_ADDR 0x6C0A1BC

UINT32 LsePatchCode[] = {
    // LsePatchStart:
    0xA8C13FF0, // ldp	x16, x15, [sp], #0x10
    0xD10403FF, // sub	sp, sp, #0x100
    0xA90007E0, // stp	x0, x1, [sp]
    0xA9010FE2, // stp	x2, x3, [sp, #0x10]
    0xA90217E4, // stp	x4, x5, [sp, #0x20]
    0xA9031FE6, // stp	x6, x7, [sp, #0x30]
    0xA90427E8, // stp	x8, x9, [sp, #0x40]
    0xA9052FEA, // stp	x10, x11, [sp, #0x50]
    0xA90637EC, // stp	x12, x13, [sp, #0x60]
    0xA9073FEE, // stp	x14, x15, [sp, #0x70]
    0xA90847F0, // stp	x16, x17, [sp, #0x80]
    0xA9094FF2, // stp	x18, x19, [sp, #0x90]
    0xA90A57F4, // stp	x20, x21, [sp, #0xa0]
    0xA90B5FF6, // stp	x22, x23, [sp, #0xb0]
    0xA90C67F8, // stp	x24, x25, [sp, #0xc0]
    0xA90D6FFA, // stp	x26, x27, [sp, #0xd0]
    0xA90E77FC, // stp	x28, x29, [sp, #0xe0]
    0xF9007BFE, // str	x30, [sp, #0xf0]
    0x910003F1, // mov	x17, sp
    0xD53C4033, // mrs	x19, ELR_EL2
    0xD5087813, // at	s1e1r, x19
    0xD5387414, // mrs	x20, PAR_EL1
    0x37002F94, // tbnz	w20, #0x0, 0x648 <LsePatchFail>
    0xD34CBE94, // ubfx	x20, x20, #12, #36
    0xD374CE94, // lsl	x20, x20, #12
    0x92402E73, // and	x19, x19, #0xfff
    0xAA130294, // orr	x20, x20, x19
    0xB9400293, // ldr	w19, [x20]
    0x529F8014, // mov	w20, #0xfc00            // =64512
    0x72A7E414, // movk	w20, #0x3f20, lsl #16
    0x0A140275, // and	w21, w19, w20
    0x529F8014, // mov	w20, #0xfc00            // =64512
    0x72A11414, // movk	w20, #0x8a0, lsl #16
    0x6B1402BF, // cmp	w21, w20
    0x54000061, // b.ne	0x94 <LsePatchRmwCheck>
    0x36B81773, // tbz	w19, #0x17, 0x378 <LsePatchCasPair>
    0x14000108, // b	0x4b0 <LsePatchCas>
    // LsePatchRmwCheck:
    0x529F8014, // mov	w20, #0xfc00            // =64512
    0x72A70414, // movk	w20, #0x3820, lsl #16
    0x0A140275, // and	w21, w19, w20
    0x6B1402BF, // cmp	w21, w20
    0x54002D21, // b.ne	0x648 <LsePatchFail>
    0x530C3E72, // ubfx	w18, w19, #12, #4
    0x531E7E75, // lsr	w21, w19, #30
    0x53105276, // ubfx	w22, w19, #16, #5
    0x53052677, // ubfx	w23, w19, #5, #5
    0x53001278, // ubfx	w24, w19, #0, #5
    0x71007EFF, // cmp	w23, #0x1f
    0x54000060, // b.eq	0xcc <LsePatchRmwSp>
    0xF8775A39, // ldr	x25, [x17, w23, uxtw #3]
    0x14000002, // b	0xd0 <LsePatchRmwTranslate>
    // LsePatchRmwSp:
    0xD53C4119, // mrs	x25, SP_EL1
    // LsePatchRmwTranslate:
    0xD5087819, // at	s1e1r, x25
    0xD5387414, // mrs	x20, PAR_EL1
    0x37002B94, // tbnz	w20, #0x0, 0x648 <LsePatchFail>
    0xD34CBE94, // ubfx	x20, x20, #12, #36
    0xD374CE94, // lsl	x20, x20, #12
    0x92402F39, // and	x25, x25, #0xfff
    0xAA140339, // orr	x25, x25, x20
    0x71007EDF, // cmp	w22, #0x1f
    0x54000060, // b.eq	0xfc <LsePatchRmwZero>
    0xF8765A3A, // ldr	x26, [x17, w22, uxtw #3]
    0x14000002, // b	0x100 <LsePatchRmwMask>
    // LsePatchRmwZero:
    0xAA1F03FA, // mov	x26, xzr
    // LsePatchRmwMask:
    0x710002BF, // cmp	w21, #0x0
    0x540000C0, // b.eq	0x11c <LsePatchRmwMask8>
    0x710006BF, // cmp	w21, #0x1
    0x540000C0, // b.eq	0x124 <LsePatchRmwMask16>
    0x71000ABF, // cmp	w21, #0x2
    0x540000C0, // b.eq	0x12c <LsePatchRmwMask32>
    0x14000006, // b	0x130 <LsePatchRmwRetry>
    // LsePatchRmwMask8:
    0x92401F5A, // and	x26, x26, #0xff
    0x14000004, // b	0x130 <LsePatchRmwRetry>
    // LsePatchRmwMask16:
    0x92403F5A, // and	x26, x26, #0xffff
    0x14000002, // b	0x130 <LsePatchRmwRetry>
    // LsePatchRmwMask32:
    0xD3407F5A, // ubfx	x26, x26, #0, #32
    // LsePatchRmwRetry:
    0x710002BF, // cmp	w21, #0x0
    0x54000140, // b.eq	0x15c <LsePatchLoad8>
    0x710006BF, // cmp	w21, #0x1
    0x540001A0, // b.eq	0x170 <LsePatchLoad16>
    0x71000ABF, // cmp	w21, #0x2
    0x54000200, // b.eq	0x184 <LsePatchLoad32>
    0x36B80073, // tbz	w19, #0x17, 0x154 <LsePatchLoad64Relaxed>
    0xC85FFF3B, // ldaxr	x27, [x25]
    0x14000011, // b	0x194 <LsePatchRmwOp>
    // LsePatchLoad64Relaxed:
    0xC85F7F3B, // ldxr	x27, [x25]
    0x1400000F, // b	0x194 <LsePatchRmwOp>
    // LsePatchLoad8:
    0x36B80073, // tbz	w19, #0x17, 0x168 <LsePatchLoad8Relaxed>
    0x085FFF3B, // ldaxrb	w27, [x25]
    0x1400000C, // b	0x194 <LsePatchRmwOp>
    // LsePatchLoad8Relaxed:
    0x085F7F3B, // ldxrb	w27, [x25]
    0x1400000A, // b	0x194 <LsePatchRmwOp>
    // LsePatchLoad16:
    0x36B80073, // tbz	w19, #0x17, 0x17c <LsePatchLoad16Relaxed>
    0x485FFF3B, // ldaxrh	w27, [x25]
    0x14000007, // b	0x194 <LsePatchRmwOp>
    // LsePatchLoad16Relaxed:
    0x485F7F3B, // ldxrh	w27, [x25]
    0x14000005, // b	0x194 <LsePatchRmwOp>
    // LsePatchLoad32:
    0x36B80073, // tbz	w19, #0x17, 0x190 <LsePatchLoad32Relaxed>
    0x885FFF3B, // ldaxr	w27, [x25]
    0x14000002, // b	0x194 <LsePatchRmwOp>
    // LsePatchLoad32Relaxed:
    0x885F7F3B, // ldxr	w27, [x25]
    // LsePatchRmwOp:
    0x7100025F, // cmp	w18, #0x0
    0x54000220, // b.eq	0x1dc <LsePatchAdd>
    0x7100065F, // cmp	w18, #0x1
    0x54000220, // b.eq	0x1e4 <LsePatchClr>
    0x71000A5F, // cmp	w18, #0x2
    0x54000220, // b.eq	0x1ec <LsePatchEor>
    0x71000E5F, // cmp	w18, #0x3
    0x54000220, // b.eq	0x1f4 <LsePatchSet>
    0x7100125F, // cmp	w18, #0x4
    0x54000220, // b.eq	0x1fc <LsePatchSmax>
    0x7100165F, // cmp	w18, #0x5
    0x540004E0, // b.eq	0x25c <LsePatchSmin>
    0x71001A5F, // cmp	w18, #0x6
    0x540007A0, // b.eq	0x2bc <LsePatchUmax>
    0x71001E5F, // cmp	w18, #0x7
    0x540007C0, // b.eq	0x2c8 <LsePatchUmin>
    0xAA1A03FC, // mov	x28, x26
    0x1400004A, // b	0x300 <LsePatchStore>
    // LsePatchAdd:
    0x8B1A037C, // add	x28, x27, x26
    0x1400003C, // b	0x2d0 <LsePatchNormalize>
    // LsePatchClr:
    0x8A3A037C, // bic	x28, x27, x26
    0x1400003A, // b	0x2d0 <LsePatchNormalize>
    // LsePatchEor:
    0xCA1A037C, // eor	x28, x27, x26
    0x14000038, // b	0x2d0 <LsePatchNormalize>
    // LsePatchSet:
    0xAA1A037C, // orr	x28, x27, x26
    0x14000036, // b	0x2d0 <LsePatchNormalize>
    // LsePatchSmax:
    0x710002BF, // cmp	w21, #0x0
    0x54000100, // b.eq	0x220 <LsePatchSmax8>
    0x710006BF, // cmp	w21, #0x1
    0x54000160, // b.eq	0x234 <LsePatchSmax16>
    0x71000ABF, // cmp	w21, #0x2
    0x540001C0, // b.eq	0x248 <LsePatchSmax32>
    0xEB1A037F, // cmp	x27, x26
    0x9A9AC37C, // csel	x28, x27, x26, gt
    0x14000039, // b	0x300 <LsePatchStore>
    // LsePatchSmax8:
    0x93401F7D, // sxtb	x29, w27
    0x93401F5E, // sxtb	x30, w26
    0xEB1E03BF, // cmp	x29, x30
    0x9A9AC37C, // csel	x28, x27, x26, gt
    0x14000034, // b	0x300 <LsePatchStore>
    // LsePatchSmax16:
    0x93403F7D, // sxth	x29, w27
    0x93403F5E, // sxth	x30, w26
    0xEB1E03BF, // cmp	x29, x30
    0x9A9AC37C, // csel	x28, x27, x26, gt
    0x1400002F, // b	0x300 <LsePatchStore>
    // LsePatchSmax32:
    0x93407F7D, // sxtw	x29, w27
    0x93407F5E, // sxtw	x30, w26
    0xEB1E03BF, // cmp	x29, x30
    0x9A9AC37C, // csel	x28, x27, x26, gt
    0x1400002A, // b	0x300 <LsePatchStore>
    // LsePatchSmin:
    0x710002BF, // cmp	w21, #0x0
    0x54000100, // b.eq	0x280 <LsePatchSmin8>
    0x710006BF, // cmp	w21, #0x1
    0x54000160, // b.eq	0x294 <LsePatchSmin16>
    0x71000ABF, // cmp	w21, #0x2
    0x540001C0, // b.eq	0x2a8 <LsePatchSmin32>
    0xEB1A037F, // cmp	x27, x26
    0x9A9AB37C, // csel	x28, x27, x26, lt
    0x14000021, // b	0x300 <LsePatchStore>
    // LsePatchSmin8:
    0x93401F7D, // sxtb	x29, w27
    0x93401F5E, // sxtb	x30, w26
    0xEB1E03BF, // cmp	x29, x30
    0x9A9AB37C, // csel	x28, x27, x26, lt
    0x1400001C, // b	0x300 <LsePatchStore>
    // LsePatchSmin16:
    0x93403F7D, // sxth	x29, w27
    0x93403F5E, // sxth	x30, w26
    0xEB1E03BF, // cmp	x29, x30
    0x9A9AB37C, // csel	x28, x27, x26, lt
    0x14000017, // b	0x300 <LsePatchStore>
    // LsePatchSmin32:
    0x93407F7D, // sxtw	x29, w27
    0x93407F5E, // sxtw	x30, w26
    0xEB1E03BF, // cmp	x29, x30
    0x9A9AB37C, // csel	x28, x27, x26, lt
    0x14000012, // b	0x300 <LsePatchStore>
    // LsePatchUmax:
    0xEB1A037F, // cmp	x27, x26
    0x9A9A837C, // csel	x28, x27, x26, hi
    0x1400000F, // b	0x300 <LsePatchStore>
    // LsePatchUmin:
    0xEB1A037F, // cmp	x27, x26
    0x9A9A337C, // csel	x28, x27, x26, lo
    // LsePatchNormalize:
    0x710002BF, // cmp	w21, #0x0
    0x540000C0, // b.eq	0x2ec <LsePatchNormalize8>
    0x710006BF, // cmp	w21, #0x1
    0x540000C0, // b.eq	0x2f4 <LsePatchNormalize16>
    0x71000ABF, // cmp	w21, #0x2
    0x540000C0, // b.eq	0x2fc <LsePatchNormalize32>
    0x14000006, // b	0x300 <LsePatchStore>
    // LsePatchNormalize8:
    0x92401F9C, // and	x28, x28, #0xff
    0x14000004, // b	0x300 <LsePatchStore>
    // LsePatchNormalize16:
    0x92403F9C, // and	x28, x28, #0xffff
    0x14000002, // b	0x300 <LsePatchStore>
    // LsePatchNormalize32:
    0xD3407F9C, // ubfx	x28, x28, #0, #32
    // LsePatchStore:
    0x710002BF, // cmp	w21, #0x0
    0x54000140, // b.eq	0x32c <LsePatchStore8>
    0x710006BF, // cmp	w21, #0x1
    0x540001A0, // b.eq	0x340 <LsePatchStore16>
    0x71000ABF, // cmp	w21, #0x2
    0x54000200, // b.eq	0x354 <LsePatchStore32>
    0x36B00073, // tbz	w19, #0x16, 0x324 <LsePatchStore64Relaxed>
    0xC814FF3C, // stlxr	w20, x28, [x25]
    0x14000011, // b	0x364 <LsePatchStoreStatus>
    // LsePatchStore64Relaxed:
    0xC8147F3C, // stxr	w20, x28, [x25]
    0x1400000F, // b	0x364 <LsePatchStoreStatus>
    // LsePatchStore8:
    0x36B00073, // tbz	w19, #0x16, 0x338 <LsePatchStore8Relaxed>
    0x0814FF3C, // stlxrb	w20, w28, [x25]
    0x1400000C, // b	0x364 <LsePatchStoreStatus>
    // LsePatchStore8Relaxed:
    0x08147F3C, // stxrb	w20, w28, [x25]
    0x1400000A, // b	0x364 <LsePatchStoreStatus>
    // LsePatchStore16:
    0x36B00073, // tbz	w19, #0x16, 0x34c <LsePatchStore16Relaxed>
    0x4814FF3C, // stlxrh	w20, w28, [x25]
    0x14000007, // b	0x364 <LsePatchStoreStatus>
    // LsePatchStore16Relaxed:
    0x48147F3C, // stxrh	w20, w28, [x25]
    0x14000005, // b	0x364 <LsePatchStoreStatus>
    // LsePatchStore32:
    0x36B00073, // tbz	w19, #0x16, 0x360 <LsePatchStore32Relaxed>
    0x8814FF3C, // stlxr	w20, w28, [x25]
    0x14000002, // b	0x364 <LsePatchStoreStatus>
    // LsePatchStore32Relaxed:
    0x88147F3C, // stxr	w20, w28, [x25]
    // LsePatchStoreStatus:
    0x35FFEE74, // cbnz	w20, 0x130 <LsePatchRmwRetry>
    0x71007F1F, // cmp	w24, #0x1f
    0x54001660, // b.eq	0x638 <LsePatchAdvance>
    0xF8385A3B, // str	x27, [x17, w24, uxtw #3]
    0x140000B1, // b	0x638 <LsePatchAdvance>
    // LsePatchCasPair:
    0x531E7E75, // lsr	w21, w19, #30
    0x53105276, // ubfx	w22, w19, #16, #5
    0x53052677, // ubfx	w23, w19, #5, #5
    0x53001278, // ubfx	w24, w19, #0, #5
    0x36000056, // tbz	w22, #0x0, 0x390 <LsePatchCasPairExpectedEven>
    0x140000AF, // b	0x648 <LsePatchFail>
    // LsePatchCasPairExpectedEven:
    0x71007ADF, // cmp	w22, #0x1e
    0x540015A8, // b.hi	0x648 <LsePatchFail>
    0x36000058, // tbz	w24, #0x0, 0x3a0 <LsePatchCasPairNewEven>
    0x140000AB, // b	0x648 <LsePatchFail>
    // LsePatchCasPairNewEven:
    0x71007B1F, // cmp	w24, #0x1e
    0x54001528, // b.hi	0x648 <LsePatchFail>
    0x71007EFF, // cmp	w23, #0x1f
    0x54000060, // b.eq	0x3b8 <LsePatchCasPairSp>
    0xF8775A39, // ldr	x25, [x17, w23, uxtw #3]
    0x14000002, // b	0x3bc <LsePatchCasPairTranslate>
    // LsePatchCasPairSp:
    0xD53C4119, // mrs	x25, SP_EL1
    // LsePatchCasPairTranslate:
    0xD5087819, // at	s1e1r, x25
    0xD5387414, // mrs	x20, PAR_EL1
    0x37001434, // tbnz	w20, #0x0, 0x648 <LsePatchFail>
    0xD34CBE94, // ubfx	x20, x20, #12, #36
    0xD374CE94, // lsl	x20, x20, #12
    0x92402F39, // and	x25, x25, #0xfff
    0xAA140339, // orr	x25, x25, x20
    0x710002BF, // cmp	w21, #0x0
    0x54000100, // b.eq	0x3fc <LsePatchCasPair32>
    0x710006BF, // cmp	w21, #0x1
    0x54001321, // b.ne	0x648 <LsePatchFail>
    // LsePatchCasPair64Retry:
    0x36B00073, // tbz	w19, #0x16, 0x3f4 <LsePatchCasPair64Relaxed>
    0xC87FF33B, // ldaxp	x27, x28, [x25]
    0x14000007, // b	0x40c <LsePatchCasPairCompare>
    // LsePatchCasPair64Relaxed:
    0xC87F733B, // ldxp	x27, x28, [x25]
    0x14000005, // b	0x40c <LsePatchCasPairCompare>
    // LsePatchCasPair32:
    0x36B00073, // tbz	w19, #0x16, 0x408 <LsePatchCasPair32Relaxed>
    0x887FF33B, // ldaxp	w27, w28, [x25]
    0x14000002, // b	0x40c <LsePatchCasPairCompare>
    // LsePatchCasPair32Relaxed:
    0x887F733B, // ldxp	w27, w28, [x25]
    // LsePatchCasPairCompare:
    0x71007EDF, // cmp	w22, #0x1f
    0x540011C0, // b.eq	0x648 <LsePatchFail>
    0xF8765A3A, // ldr	x26, [x17, w22, uxtw #3]
    0x110006D4, // add	w20, w22, #0x1
    0xF8745A3D, // ldr	x29, [x17, w20, uxtw #3]
    0x71007F1F, // cmp	w24, #0x1f
    0x54001120, // b.eq	0x648 <LsePatchFail>
    0xF8785A3E, // ldr	x30, [x17, w24, uxtw #3]
    0x11000714, // add	w20, w24, #0x1
    0xF8745A30, // ldr	x16, [x17, w20, uxtw #3]
    0x710002BF, // cmp	w21, #0x0
    0x54000140, // b.eq	0x460 <LsePatchCasPairCompare32>
    0xEB1A037F, // cmp	x27, x26
    0x54000301, // b.ne	0x4a0 <LsePatchCasPairNoStore>
    0xEB1D039F, // cmp	x28, x29
    0x540002C1, // b.ne	0x4a0 <LsePatchCasPairNoStore>
    0x36780073, // tbz	w19, #0xf, 0x458 <LsePatchCasPair64RelaxedStore>
    0xC834C33E, // stlxp	w20, x30, x16, [x25]
    0x1400000B, // b	0x480 <LsePatchCasPairStatus>
    // LsePatchCasPair64RelaxedStore:
    0xC834433E, // stxp	w20, x30, x16, [x25]
    0x14000009, // b	0x480 <LsePatchCasPairStatus>
    // LsePatchCasPairCompare32:
    0x6B1A037F, // cmp	w27, w26
    0x540001E1, // b.ne	0x4a0 <LsePatchCasPairNoStore>
    0x6B1D039F, // cmp	w28, w29
    0x540001A1, // b.ne	0x4a0 <LsePatchCasPairNoStore>
    0x36780073, // tbz	w19, #0xf, 0x47c <LsePatchCasPair32RelaxedStore>
    0x8834C33E, // stlxp	w20, w30, w16, [x25]
    0x14000002, // b	0x480 <LsePatchCasPairStatus>
    // LsePatchCasPair32RelaxedStore:
    0x8834433E, // stxp	w20, w30, w16, [x25]
    // LsePatchCasPairStatus:
    0x35000054, // cbnz	w20, 0x488 <LsePatchCasPairRetry>
    0x14000007, // b	0x4a0 <LsePatchCasPairNoStore>
    // LsePatchCasPairRetry:
    0x710002BF, // cmp	w21, #0x0
    0x54000040, // b.eq	0x494 <LsePatchCasPair32Retry>
    0x17FFFFD6, // b	0x3e8 <LsePatchCasPair64Retry>
    // LsePatchCasPair32Retry:
    0x36B7FBB3, // tbz	w19, #0x16, 0x408 <LsePatchCasPair32Relaxed>
    0x887FF33B, // ldaxp	w27, w28, [x25]
    0x17FFFFDC, // b	0x40c <LsePatchCasPairCompare>
    // LsePatchCasPairNoStore:
    0xF8365A3B, // str	x27, [x17, w22, uxtw #3]
    0x110006D4, // add	w20, w22, #0x1
    0xF8345A3C, // str	x28, [x17, w20, uxtw #3]
    0x14000063, // b	0x638 <LsePatchAdvance>
    // LsePatchCas:
    0x531E7E75, // lsr	w21, w19, #30
    0x53105276, // ubfx	w22, w19, #16, #5
    0x53052677, // ubfx	w23, w19, #5, #5
    0x53001278, // ubfx	w24, w19, #0, #5
    0x71007EFF, // cmp	w23, #0x1f
    0x54000060, // b.eq	0x4d0 <LsePatchCasSp>
    0xF8775A39, // ldr	x25, [x17, w23, uxtw #3]
    0x14000002, // b	0x4d4 <LsePatchCasTranslate>
    // LsePatchCasSp:
    0xD53C4119, // mrs	x25, SP_EL1
    // LsePatchCasTranslate:
    0xD5087819, // at	s1e1r, x25
    0xD5387414, // mrs	x20, PAR_EL1
    0x37000B74, // tbnz	w20, #0x0, 0x648 <LsePatchFail>
    0xD34CBE94, // ubfx	x20, x20, #12, #36
    0xD374CE94, // lsl	x20, x20, #12
    0x92402F39, // and	x25, x25, #0xfff
    0xAA140339, // orr	x25, x25, x20
    0x71007EDF, // cmp	w22, #0x1f
    0x54000060, // b.eq	0x500 <LsePatchCasExpectedZero>
    0xF8765A3A, // ldr	x26, [x17, w22, uxtw #3]
    0x14000002, // b	0x504 <LsePatchCasExpectedMask>
    // LsePatchCasExpectedZero:
    0xAA1F03FA, // mov	x26, xzr
    // LsePatchCasExpectedMask:
    0x71007F1F, // cmp	w24, #0x1f
    0x54000060, // b.eq	0x514 <LsePatchCasNewZero>
    0xF8785A3C, // ldr	x28, [x17, w24, uxtw #3]
    0x14000002, // b	0x518 <LsePatchCasNewMask>
    // LsePatchCasNewZero:
    0xAA1F03FC, // mov	x28, xzr
    // LsePatchCasNewMask:
    0x710002BF, // cmp	w21, #0x0
    0x54000140, // b.eq	0x544 <LsePatchCas8>
    0x710006BF, // cmp	w21, #0x1
    0x540001A0, // b.eq	0x558 <LsePatchCas16>
    0x71000ABF, // cmp	w21, #0x2
    0x54000200, // b.eq	0x56c <LsePatchCas32>
    0x36B00073, // tbz	w19, #0x16, 0x53c <LsePatchCas64Relaxed>
    0xC85FFF3B, // ldaxr	x27, [x25]
    0x14000011, // b	0x57c <LsePatchCasCompare>
    // LsePatchCas64Relaxed:
    0xC85F7F3B, // ldxr	x27, [x25]
    0x1400000F, // b	0x57c <LsePatchCasCompare>
    // LsePatchCas8:
    0x36B00073, // tbz	w19, #0x16, 0x550 <LsePatchCas8Relaxed>
    0x085FFF3B, // ldaxrb	w27, [x25]
    0x1400000C, // b	0x57c <LsePatchCasCompare>
    // LsePatchCas8Relaxed:
    0x085F7F3B, // ldxrb	w27, [x25]
    0x1400000A, // b	0x57c <LsePatchCasCompare>
    // LsePatchCas16:
    0x36B00073, // tbz	w19, #0x16, 0x564 <LsePatchCas16Relaxed>
    0x485FFF3B, // ldaxrh	w27, [x25]
    0x14000007, // b	0x57c <LsePatchCasCompare>
    // LsePatchCas16Relaxed:
    0x485F7F3B, // ldxrh	w27, [x25]
    0x14000005, // b	0x57c <LsePatchCasCompare>
    // LsePatchCas32:
    0x36B00073, // tbz	w19, #0x16, 0x578 <LsePatchCas32Relaxed>
    0x885FFF3B, // ldaxr	w27, [x25]
    0x14000002, // b	0x57c <LsePatchCasCompare>
    // LsePatchCas32Relaxed:
    0x885F7F3B, // ldxr	w27, [x25]
    // LsePatchCasCompare:
    0x710002BF, // cmp	w21, #0x0
    0x54000100, // b.eq	0x5a0 <LsePatchCasCompare8>
    0x710006BF, // cmp	w21, #0x1
    0x54000120, // b.eq	0x5ac <LsePatchCasCompare16>
    0x71000ABF, // cmp	w21, #0x2
    0x54000140, // b.eq	0x5b8 <LsePatchCasCompare32>
    0xEB1A037F, // cmp	x27, x26
    0x54000481, // b.ne	0x628 <LsePatchCasNoStore>
    0x14000009, // b	0x5c0 <LsePatchCasStore>
    // LsePatchCasCompare8:
    0x6B1A037F, // cmp	w27, w26
    0x54000421, // b.ne	0x628 <LsePatchCasNoStore>
    0x14000006, // b	0x5c0 <LsePatchCasStore>
    // LsePatchCasCompare16:
    0x6B1A037F, // cmp	w27, w26
    0x540003C1, // b.ne	0x628 <LsePatchCasNoStore>
    0x14000003, // b	0x5c0 <LsePatchCasStore>
    // LsePatchCasCompare32:
    0x6B1A037F, // cmp	w27, w26
    0x54000361, // b.ne	0x628 <LsePatchCasNoStore>
    // LsePatchCasStore:
    0x710002BF, // cmp	w21, #0x0
    0x54000140, // b.eq	0x5ec <LsePatchCasStore8>
    0x710006BF, // cmp	w21, #0x1
    0x540001A0, // b.eq	0x600 <LsePatchCasStore16>
    0x71000ABF, // cmp	w21, #0x2
    0x54000200, // b.eq	0x614 <LsePatchCasStore32>
    0x36780073, // tbz	w19, #0xf, 0x5e4 <LsePatchCasStore64Relaxed>
    0xC814FF3C, // stlxr	w20, x28, [x25]
    0x14000011, // b	0x624 <LsePatchCasStatus>
    // LsePatchCasStore64Relaxed:
    0xC8147F3C, // stxr	w20, x28, [x25]
    0x1400000F, // b	0x624 <LsePatchCasStatus>
    // LsePatchCasStore8:
    0x36780073, // tbz	w19, #0xf, 0x5f8 <LsePatchCasStore8Relaxed>
    0x0814FF3C, // stlxrb	w20, w28, [x25]
    0x1400000C, // b	0x624 <LsePatchCasStatus>
    // LsePatchCasStore8Relaxed:
    0x08147F3C, // stxrb	w20, w28, [x25]
    0x1400000A, // b	0x624 <LsePatchCasStatus>
    // LsePatchCasStore16:
    0x36780073, // tbz	w19, #0xf, 0x60c <LsePatchCasStore16Relaxed>
    0x4814FF3C, // stlxrh	w20, w28, [x25]
    0x14000007, // b	0x624 <LsePatchCasStatus>
    // LsePatchCasStore16Relaxed:
    0x48147F3C, // stxrh	w20, w28, [x25]
    0x14000005, // b	0x624 <LsePatchCasStatus>
    // LsePatchCasStore32:
    0x36780073, // tbz	w19, #0xf, 0x620 <LsePatchCasStore32Relaxed>
    0x8814FF3C, // stlxr	w20, w28, [x25]
    0x14000002, // b	0x624 <LsePatchCasStatus>
    // LsePatchCasStore32Relaxed:
    0x88147F3C, // stxr	w20, w28, [x25]
    // LsePatchCasStatus:
    0x35FFF474, // cbnz	w20, 0x4b0 <LsePatchCas>
    // LsePatchCasNoStore:
    0x71007EDF, // cmp	w22, #0x1f
    0x54000060, // b.eq	0x638 <LsePatchAdvance>
    0xF8365A3B, // str	x27, [x17, w22, uxtw #3]
    0x14000001, // b	0x638 <LsePatchAdvance>
    // LsePatchAdvance:
    0xD53C4034, // mrs	x20, ELR_EL2
    0x91001294, // add	x20, x20, #0x4
    0xD51C4034, // msr	ELR_EL2, x20
    0x14000016, // b	0x69c <LsePatchRestore>
    // LsePatchFail:
    0xA94007E0, // ldp	x0, x1, [sp]
    0xA9410FE2, // ldp	x2, x3, [sp, #0x10]
    0xA94217E4, // ldp	x4, x5, [sp, #0x20]
    0xA9431FE6, // ldp	x6, x7, [sp, #0x30]
    0xA94427E8, // ldp	x8, x9, [sp, #0x40]
    0xA9452FEA, // ldp	x10, x11, [sp, #0x50]
    0xA94637EC, // ldp	x12, x13, [sp, #0x60]
    0xA9473FEE, // ldp	x14, x15, [sp, #0x70]
    0xA94847F0, // ldp	x16, x17, [sp, #0x80]
    0xA9494FF2, // ldp	x18, x19, [sp, #0x90]
    0xA94A57F4, // ldp	x20, x21, [sp, #0xa0]
    0xA94B5FF6, // ldp	x22, x23, [sp, #0xb0]
    0xA94C67F8, // ldp	x24, x25, [sp, #0xc0]
    0xA94D6FFA, // ldp	x26, x27, [sp, #0xd0]
    0xA94E77FC, // ldp	x28, x29, [sp, #0xe0]
    0xF9407BFE, // ldr	x30, [sp, #0xf0]
    0x910403FF, // add	sp, sp, #0x100
    0xD28EC110, // mov	x16, #0x7608            // =30216
    0xF2A0D810, // movk	x16, #0x6c0, lsl #16
    0xA9BF3FF0, // stp	x16, x15, [sp, #-0x10]!
    0xD61F0200, // br	x16
    // LsePatchRestore:
    0xA94007E0, // ldp	x0, x1, [sp]
    0xA9410FE2, // ldp	x2, x3, [sp, #0x10]
    0xA94217E4, // ldp	x4, x5, [sp, #0x20]
    0xA9431FE6, // ldp	x6, x7, [sp, #0x30]
    0xA94427E8, // ldp	x8, x9, [sp, #0x40]
    0xA9452FEA, // ldp	x10, x11, [sp, #0x50]
    0xA94637EC, // ldp	x12, x13, [sp, #0x60]
    0xA9473FEE, // ldp	x14, x15, [sp, #0x70]
    0xA94847F0, // ldp	x16, x17, [sp, #0x80]
    0xA9494FF2, // ldp	x18, x19, [sp, #0x90]
    0xA94A57F4, // ldp	x20, x21, [sp, #0xa0]
    0xA94B5FF6, // ldp	x22, x23, [sp, #0xb0]
    0xA94C67F8, // ldp	x24, x25, [sp, #0xc0]
    0xA94D6FFA, // ldp	x26, x27, [sp, #0xd0]
    0xA94E77FC, // ldp	x28, x29, [sp, #0xe0]
    0xF9407BFE, // ldr	x30, [sp, #0xf0]
    0x910403FF, // add	sp, sp, #0x100
    0xD69F03E0, // eret
};

// Set HCR_EL2.TSC upon powergate wake-up.
// This patch is shared between platforms.
UINT32 WakeFromPowerGatePatchHandler[] = {
    0xd53c1108, // mrs  x8, HCR_EL2
    0xb26d0108, // orr  x8, x8, #(HCR_EL2.TSC)
    0xd51c1108, // msr  HCR_EL2, x8
    0xd5033fdf, // isb
};

#if SILICON_PLATFORM == 8992
UINT32 LowerELSynchronous64PatchHandler[] = {
    0xd53c1110, // mrs  x16, HCR_EL2
    0xb26d0210, // orr  x16, x16, #(HCR_EL2.TSC)
    0xd51c1110, // msr  HCR_EL2, x16
    0xd5033fdf, // isb
    0xf10059ff, // cmp  x15, #0x16             6'b010110 = HVC 64bit
    0x54ff2a60, // b.eq HvcHandlerEntry
    0xf1005dff, // cmp  x15, #0x17             6'b010111 = SMC 64bit trap
    0x54000040, // b.eq El2TrapSmcHandler
    0x17fff991, // b    OtherExceptionHandler
    // El2TrapSmcHandler:
    0xd53c4030, // mrs  x16, ELR_EL2
    0x91001210, // add  x16, x16, #4
    0xd51c4030, // msr  ELR_EL2, x16
    0x121b6810, // and  w16, w0, #0xffffffe0
    0x32020210, // orr  w16, w16, #0x40000000
    0x52b8800f, // mov  w15, #0xc4000000
    0x6b0f021f, // cmp  w16, w15
    0x54000041, // b.ne El2TrapInvokeSmc
    0x17fff947, // b    HvcHandlerEntry
    // El2TrapInvokeSmc:
    0xa8c143ef, // ldp  x15, x16, [sp], #0x10
    0xd4000003, // smc  #0
    0xd69f03e0, // eret
                // Yay there are two instructions space left
};

UINT32 LowerELSynchronous32PatchHandler[] = {
    0xd53c1110, // mrs  x16, HCR_EL2
    0xb26d0210, // orr  x16, x16, #(HCR_EL2.TSC)
    0xd51c1110, // msr  HCR_EL2, x16
    0xd5033fdf, // isb
    0xf10049ff, // cmp  x15, #0x12           6'b010010 = HVC 32bit
    0x54ff1580, // b.eq HvcHandler32Entry
    0xf1004dff, // cmp  x15, #0x13           6'b010011 = SMC 32bit trap
    0x54000040, // b.eq El2TrapSmcHandler32
    0x17fff911, // b    OtherExceptionHandler
    // El2TrapSmcHandler32:
    0xd53c4030, // mrs  x16, ELR_EL2
    0x91001210, // add  x16, x16, #4
    0xd51c4030, // msr  ELR_EL2, x16
    0x121b6810, // and  w16, w0, #0xffffffe0
    0x32020210, // orr  w16, w16, #0x40000000
    0x52b8800f, // mov  w15, #0xc4000000
    0x6b0f021f, // cmp  w16, w15
    0x54000041, // b.ne El2TrapInvokeSmc32
    0x17fff8a0, // b    OtherExceptionHandler
    // El2TrapInvokeSmc32:
    0xa8c143ef, // ldp  x15, x16, [sp], #0x10
    0xd4000003, // smc  #0
    0xd69f03e0, // eret
                // Yay there are two instructions space left
};
#elif SILICON_PLATFORM == 8994
UINT32 LowerELSynchronous64PatchHandler[] = {
    0xd53c1110, // mrs  x16, HCR_EL2
    0xb26d0210, // orr  x16, x16, #(HCR_EL2.TSC)
    0xd51c1110, // msr  HCR_EL2, x16
    0xd5033fdf, // isb
    0xf10059ff, // cmp  x15, #0x16             6'b010110 = HVC 64bit
    0x54ff4660, // b.eq HvcHandlerEntry
    0xf1005dff, // cmp  x15, #0x17             6'b010111 = SMC 64bit trap
    0x54000040, // b.eq El2TrapSmcHandler
    0x17fffa71, // b    OtherExceptionHandler
    // El2TrapSmcHandler:
    0xd53c4030, // mrs  x16, ELR_EL2
    0x91001210, // add  x16, x16, #4
    0xd51c4030, // msr  ELR_EL2, x16
    0x121b6810, // and  w16, w0, #0xffffffe0
    0x32020210, // orr  w16, w16, #0x40000000
    0x52b8800f, // mov  w15, #0xc4000000
    0x6b0f021f, // cmp  w16, w15
    0x54000041, // b.ne El2TrapInvokeSmc
    0x17fffa27, // b    HvcHandlerEntry
    // El2TrapInvokeSmc:
    0xa8c143ef, // ldp  x15, x16, [sp], #0x10
    0xd4000003, // smc  #0
    0xd69f03e0, // eret
                // Yay there are two instructions space left
};

UINT32 LowerELSynchronous32PatchHandler[] = {
    0xd53c1110, // mrs  x16, HCR_EL2
    0xb26d0210, // orr  x16, x16, #(HCR_EL2.TSC)
    0xd51c1110, // msr  HCR_EL2, x16
    0xd5033fdf, // isb
    0xf10049ff, // cmp  x15, #0x12           6'b010010 = HVC 32bit
    0x54ff3180, // b.eq HvcHandler32Entry
    0xf1004dff, // cmp  x15, #0x13           6'b010011 = SMC 32bit trap
    0x54000040, // b.eq El2TrapSmcHandler32
    0x17fff9f1, // b    OtherExceptionHandler
    // El2TrapSmcHandler32:
    0xd53c4030, // mrs  x16, ELR_EL2
    0x91001210, // add  x16, x16, #4
    0xd51c4030, // msr  ELR_EL2, x16
    0x121b6810, // and  w16, w0, #0xffffffe0
    0x32020210, // orr  w16, w16, #0x40000000
    0x52b8800f, // mov  w15, #0xc4000000
    0x6b0f021f, // cmp  w16, w15
    0x54000041, // b.ne El2TrapInvokeSmc32
    0x17fff980, // b    OtherExceptionHandler
    // El2TrapInvokeSmc32:
    0xa8c143ef, // ldp  x15, x16, [sp], #0x10
    0xd4000003, // smc  #0
    0xd69f03e0, // eret
                // Yay there are two instructions space left
};
#else
#error Unsupported silicon platform
#endif // SILICON_PLATFORM

#else
#error Undefined silicon platform
#endif // defined(SILICON_PLATFORM)

#endif // __HVCPATCH_H__
