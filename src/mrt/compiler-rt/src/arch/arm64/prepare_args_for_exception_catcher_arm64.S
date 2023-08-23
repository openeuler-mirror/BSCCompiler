#define SavedRegisterSize (8 * 2)
#define PrepareArgsStructSize (8 * 4)

        .text
        .align 2
        .global PrepareArgsForExceptionCatcher
        .hidden PrepareArgsForExceptionCatcher
        .type PrepareArgsForExceptionCatcher, %function
PrepareArgsForExceptionCatcher:
        .cfi_startproc
        stp x29, x30, [sp, #-SavedRegisterSize]!
        .cfi_adjust_cfa_offset SavedRegisterSize
        .cfi_rel_offset x29, 0
        .cfi_rel_offset x30, 8

        // fp points to caller fp
        mov x29, sp
        .cfi_def_cfa_register x29

        sub sp, sp, #PrepareArgsStructSize

        mov x0, sp
        bl MRT_GetHandlerCatcherArgs

        ldr x0, [sp]
        ldr x1, [sp, #16]
        str x1, [x29, #8]
        ldr x1, [sp, #8]

        mov sp, x29
        .cfi_def_cfa_register sp
        ldp x29, x30, [sp], #SavedRegisterSize
        .cfi_adjust_cfa_offset -SavedRegisterSize
        .cfi_restore x29
        .cfi_restore x30
        ret
        .cfi_endproc
        .size PrepareArgsForExceptionCatcher, .-PrepareArgsForExceptionCatcher
