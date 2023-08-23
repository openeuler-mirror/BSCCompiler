#define RegSaveAreaSize           (8 * 32)
#define DumpFrameSize             (RegSaveAreaSize + 8 * 2)

        .text
        .align 2
        .global MRT_DumpRegisters
        .type MRT_DumpRegisters, %function
MRT_DumpRegisters:
        .cfi_startproc
        stp x0, x1, [sp, #-RegSaveAreaSize + 16*0]
        stp x2, x3, [sp, #-RegSaveAreaSize + 16*1]
        stp x4, x5, [sp, #-RegSaveAreaSize + 16*2]
        stp x6, x7, [sp, #-RegSaveAreaSize + 16*3]
        stp x8, x9, [sp, #-RegSaveAreaSize + 16*4]
        stp x10, x11, [sp, #-RegSaveAreaSize + 16*5]
        stp x12, x13, [sp, #-RegSaveAreaSize + 16*6]
        stp x14, x15, [sp, #-RegSaveAreaSize + 16*7]
        stp x16, x17, [sp, #-RegSaveAreaSize + 16*8]
        stp x18, x19, [sp, #-RegSaveAreaSize + 16*9]
        stp x20, x21, [sp, #-RegSaveAreaSize + 16*10]
        stp x22, x23, [sp, #-RegSaveAreaSize + 16*11]
        stp x24, x25, [sp, #-RegSaveAreaSize + 16*12]
        stp x26, x27, [sp, #-RegSaveAreaSize + 16*13]
        str x28, [sp, #-RegSaveAreaSize + 16*14]

        str x29, [sp, #-RegSaveAreaSize + 16*14+8]
        str x30, [sp, #-RegSaveAreaSize + 16*15]
        mov x0, sp
        str x0, [sp,  #-RegSaveAreaSize + 16*15 + 8]

        // parameter for PrintAArch64RegSet
        sub x0, sp, #RegSaveAreaSize

        // prologue
        stp x29, x30, [sp, #-DumpFrameSize]!
        mov x29, sp

        bl PrintAArch64RegSet

        // epilogue
        ldp x29, x30, [sp], #DumpFrameSize

        ldp x0, x1, [sp, #-RegSaveAreaSize + 16*0]
        ldp x2, x3, [sp, #-RegSaveAreaSize + 16*1]
        ldp x4, x5, [sp, #-RegSaveAreaSize + 16*2]
        ldp x6, x7, [sp, #-RegSaveAreaSize + 16*3]
        ldp x8, x9, [sp, #-RegSaveAreaSize + 16*4]
        ldp x10, x11, [sp, #-RegSaveAreaSize + 16*5]
        ldp x12, x13, [sp, #-RegSaveAreaSize + 16*6]
        ldp x14, x15, [sp, #-RegSaveAreaSize + 16*7]
        ldp x16, x17, [sp, #-RegSaveAreaSize + 16*8]
        ldp x18, x19, [sp, #-RegSaveAreaSize + 16*9]
        ldp x20, x21, [sp, #-RegSaveAreaSize + 16*10]
        ldp x22, x23, [sp, #-RegSaveAreaSize + 16*11]
        ldp x24, x25, [sp, #-RegSaveAreaSize + 16*12]
        ldp x26, x27, [sp, #-RegSaveAreaSize + 16*13]
        ldr x28, [sp, #-RegSaveAreaSize + 16*14]

        ret
        .cfi_endproc
        .size MRT_DumpRegisters, .-MRT_DumpRegisters
