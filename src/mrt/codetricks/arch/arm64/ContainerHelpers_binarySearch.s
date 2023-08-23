#define kArrayLengthOffset 12
#define ARRAY_START_ADDRESS_OFFSET 16

        .section        .rodata
        .align  2
.Lmethod_desc.Landroid_2Futil_2FContainerHelpers_3B_7CbinarySearch_7C_28AIII_29I__optimized:
        .word __methods_info__Landroid_2Futil_2FContainerHelpers_3B - .
        .short 16
        .short 0
        .section  .java_text,"ax"
        .global Landroid_2Futil_2FContainerHelpers_3B_7CbinarySearch_7C_28AIII_29I__optimized
        .type Landroid_2Futil_2FContainerHelpers_3B_7CbinarySearch_7C_28AIII_29I__optimized, %function
        .align 2
        .word .Lmethod_desc.Landroid_2Futil_2FContainerHelpers_3B_7CbinarySearch_7C_28AIII_29I__optimized-.
Landroid_2Futil_2FContainerHelpers_3B_7CbinarySearch_7C_28AIII_29I__optimized:
        .cfi_startproc
        .cfi_personality 155, DW.ref.__mpl_personality_v0
        stp     x29, x30, [sp,#-32]!
        .cfi_def_cfa_offset 32
        .cfi_offset 29, -32
        .cfi_offset 30, -24
        mov     x29, sp
        .cfi_def_cfa_register 29
        ldr     wzr, [x19]           // yieldpoint
                                     // x0: int[] array,  w1: size,  w2: value
        cbz     x0, .Label.Landroid_2Futil_2FContainerHelpers_3B_7CbinarySearch_7C_28AIII_29I__optimized5
        ldr     w7, [x0, #kArrayLengthOffset]        // w7: array length
        cmp     w1, w7               // check array boundry
        bgt     .Label.Landroid_2Futil_2FContainerHelpers_3B_7CbinarySearch_7C_28AIII_29I__optimized6
        add     x3, x0, #16          // x3: start address of array elements
        mov     w4, #0               // w4: lo = 0
        sub     w5, w1, #1           // w5: hi = size - 1
.Label.Landroid_2Futil_2FContainerHelpers_3B_7CbinarySearch_7C_28AIII_29I__optimized1:
        cmp     w4, w5               // lo <= hi
        bgt     .Label.Landroid_2Futil_2FContainerHelpers_3B_7CbinarySearch_7C_28AIII_29I__optimized3
        ldr     wzr, [x19]           // yieldpoint
        add     w6, w4, w5           // w6: mid = lo + hi
        lsr     w6, w6, #1           //     mid = mid >>> 1
        ldr     w0, [x3,w6,SXTW #2]  // w0: midVal = array[mid]
        cmp     w0, w2               // midVal < value
        bge     .Label.Landroid_2Futil_2FContainerHelpers_3B_7CbinarySearch_7C_28AIII_29I__optimized2
        add     w4, w6, #1           // lo = mid + 1
        b       .Label.Landroid_2Futil_2FContainerHelpers_3B_7CbinarySearch_7C_28AIII_29I__optimized1
.Label.Landroid_2Futil_2FContainerHelpers_3B_7CbinarySearch_7C_28AIII_29I__optimized2:
        sub     w5, w6, #1           // hi = mid - 1
        bgt     .Label.Landroid_2Futil_2FContainerHelpers_3B_7CbinarySearch_7C_28AIII_29I__optimized1
        mvn     w4, w6               // return mid
.Label.Landroid_2Futil_2FContainerHelpers_3B_7CbinarySearch_7C_28AIII_29I__optimized3:
        mvn     w0, w4               // return ~lo
.Label.Landroid_2Futil_2FContainerHelpers_3B_7CbinarySearch_7C_28AIII_29I__optimized4:
        ldp     x29, x30, [sp], #32
        .cfi_remember_state
        .cfi_restore 30
        .cfi_restore 29
        .cfi_def_cfa 31, 0
        ret
.Label.Landroid_2Futil_2FContainerHelpers_3B_7CbinarySearch_7C_28AIII_29I__optimized5:
        .cfi_restore_state
        bl       MCC_ThrowNullPointerException
.Label.Landroid_2Futil_2FContainerHelpers_3B_7CbinarySearch_7C_28AIII_29I__optimized6:
        bl       MCC_ThrowArrayIndexOutOfBoundsException
.Label.Landroid_2Futil_2FContainerHelpers_3B_7CbinarySearch_7C_28AIII_29I__optimized7:
        .cfi_endproc
        .size Landroid_2Futil_2FContainerHelpers_3B_7CbinarySearch_7C_28AIII_29I__optimized, .-Landroid_2Futil_2FContainerHelpers_3B_7CbinarySearch_7C_28AIII_29I__optimized
        .word 0xFFFFFFFF
        .word .Label.Landroid_2Futil_2FContainerHelpers_3B_7CbinarySearch_7C_28AIII_29I__optimized4-Landroid_2Futil_2FContainerHelpers_3B_7CbinarySearch_7C_28AIII_29I__optimized

        .section        .rodata
        .align  2
.Lmethod_desc.Landroid_2Futil_2FSparseArray_3B_7CgetDelete_7C_28_29Ljava_2Flang_2FObject_3B:
        .word __methods_info__Landroid_2Futil_2FSparseArray_3B+672-.
        .short 16
        .short 0
        .section  .java_text,"ax"
        .global  Landroid_2Futil_2FSparseArray_3B_7CgetDelete_7C_28_29Ljava_2Flang_2FObject_3B
        .type   Landroid_2Futil_2FSparseArray_3B_7CgetDelete_7C_28_29Ljava_2Flang_2FObject_3B, %function
        .align 2
        .word .Lmethod_desc.Landroid_2Futil_2FSparseArray_3B_7CgetDelete_7C_28_29Ljava_2Flang_2FObject_3B-.
Landroid_2Futil_2FSparseArray_3B_7CgetDelete_7C_28_29Ljava_2Flang_2FObject_3B:
.Label.Landroid_2Futil_2FSparseArray_3B_7CgetDelete_7C_28_29Ljava_2Flang_2FObject_3B1:          // label order 564653
        .cfi_startproc
        .cfi_personality 155, DW.ref.__mpl_personality_v0
        //      LINE SparseArray.java : 67, DEX_INSTIDX : 0 ||0000: sget-object: Landroid/util/SparseArray;.DELETED:Ljava/lang/Object; // field@37c3
        adrp    x0, :got:Landroid_2Futil_2FSparseArray_3B_7CDELETED
        ldr     x0, [x0,#:got_lo12:Landroid_2Futil_2FSparseArray_3B_7CDELETED]
        ldr     x0, [x0]
.Label.Landroid_2Futil_2FSparseArray_3B_7CgetDelete_7C_28_29Ljava_2Flang_2FObject_3B4:          // label order 564654
        ret
.Label.Landroid_2Futil_2FSparseArray_3B_7CgetDelete_7C_28_29Ljava_2Flang_2FObject_3B3:          // label order 564655
.Label.Landroid_2Futil_2FSparseArray_3B_7CgetDelete_7C_28_29Ljava_2Flang_2FObject_3B2:          // label order 564656
        .cfi_endproc
.Label.Landroid_2Futil_2FSparseArray_3B_7CgetDelete_7C_28_29Ljava_2Flang_2FObject_3B.end:
        .size   Landroid_2Futil_2FSparseArray_3B_7CgetDelete_7C_28_29Ljava_2Flang_2FObject_3B, .-Landroid_2Futil_2FSparseArray_3B_7CgetDelete_7C_28_29Ljava_2Flang_2FObject_3B

        .section        .rodata
        .align  2
.Lmethod_desc.Landroid_2Futil_2FContainerHelpers_3B_7CbinarySearch_7C_28AIII_29I__optimizedForCollection:
        .word __methods_info__Landroid_2Futil_2FContainerHelpers_3B-.
        .short 16
        .short 0
        .section  .java_text,"ax"
        .global Landroid_2Futil_2FContainerHelpers_3B_7CbinarySearch_7C_28AIII_29I__optimizedForCollection
        .type Landroid_2Futil_2FContainerHelpers_3B_7CbinarySearch_7C_28AIII_29I__optimizedForCollection, %function
        .align 2
        .word .Lmethod_desc.Landroid_2Futil_2FContainerHelpers_3B_7CbinarySearch_7C_28AIII_29I__optimizedForCollection-.
Landroid_2Futil_2FContainerHelpers_3B_7CbinarySearch_7C_28AIII_29I__optimizedForCollection:
        .cfi_startproc
        .cfi_personality 155, DW.ref.__mpl_personality_v0
        add     x3, x0, #ARRAY_START_ADDRESS_OFFSET          // x3: start address of array elements
        mov     w4, #0               // w4: lo = 0
        sub     w5, w1, #1           // w5: hi = size - 1
.Label.Landroid_2Futil_2FContainerHelpers_3B_7CbinarySearch_7C_28AIII_29I__optimizedForCollection1:
        cmp     w4, w5               // lo <= hi
        bgt     .Label.Landroid_2Futil_2FContainerHelpers_3B_7CbinarySearch_7C_28AIII_29I__optimizedForCollection3
        add     w0, w4, w5           // w0: mid = lo + hi
        lsr     w0, w0, #1           //     mid = mid >>> 1
        ldr     w6, [x3,w0,SXTW #2]  // w6: midVal = array[mid]
        cmp     w6, w2               // midVal < value
        bge     .Label.Landroid_2Futil_2FContainerHelpers_3B_7CbinarySearch_7C_28AIII_29I__optimizedForCollection2
        add     w4, w0, #1           // lo = mid + 1
        b       .Label.Landroid_2Futil_2FContainerHelpers_3B_7CbinarySearch_7C_28AIII_29I__optimizedForCollection1
.Label.Landroid_2Futil_2FContainerHelpers_3B_7CbinarySearch_7C_28AIII_29I__optimizedForCollection2:
        sub     w5, w0, #1           // hi = mid - 1
        bgt     .Label.Landroid_2Futil_2FContainerHelpers_3B_7CbinarySearch_7C_28AIII_29I__optimizedForCollection1
        ret                          // return mid
.Label.Landroid_2Futil_2FContainerHelpers_3B_7CbinarySearch_7C_28AIII_29I__optimizedForCollection3:
        mvn     w0, w4               // return ~lo
        ret
        .cfi_endproc
        .size Landroid_2Futil_2FContainerHelpers_3B_7CbinarySearch_7C_28AIII_29I__optimizedForCollection, .-Landroid_2Futil_2FContainerHelpers_3B_7CbinarySearch_7C_28AIII_29I__optimizedForCollection
       .word 0xFFFFFFFF
       .word 0

