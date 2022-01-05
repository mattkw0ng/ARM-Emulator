.global merge_sort_s

@ r0 - arr
@ r1 - aux
@ r2 - start
@ r3 - end
@ r12 - mid

merge_sort_s:
    sub sp, sp, #16
    str lr, [sp]

    cmp r3, r2                  @ if the subsection is empty or single item, return
    ble merge_sort_exit
    
    add r12, r2, r3             @ calculate midpoint
    lsr r12, r12, #1
    
    str r12, [sp, #4]           @ preserve variables
    str r2, [sp, #8]
    str r3, [sp, #12]

    mov r3, r12
    bl merge_sort_s             @ merge_sort_s(arr, aux, start, mid)

    ldr r3, [sp, #12]           @ restore end from stack
    ldr r12, [sp, #4]           @ restore midpoint from stack and add 1
    add r12, r12, #1
    mov r2, r12
    bl merge_sort_s             @ merge_sort_s(arr, aux, mid + 1, end)

    ldr r2, [sp, #8]            @ restore original start and end values from stack
    ldr r3, [sp, #12]

    bl merge_s                  @ merge two subarrays together

merge_sort_exit:
    ldr lr, [sp]
    add sp, sp, #16
    
    bx lr
