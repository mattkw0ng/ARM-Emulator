.global sort_s

@ r0 - array
@ r1 - start value
@ r2 - len (originally at r1)
@ r3 - current index
@ r4 - output of find_max_index
@ r5 - value at current index
@ r6 - value at max index

sort_s:
    mov r3, #0                  @ set index to 0
    mov r2, r1                  @ move len to r2 to match find_max_index
    mov r1, #1                  @ set start value to 1
    sub sp, sp, #32
    str lr, [sp]
    str r4, [sp, #16]
    str r5, [sp, #20]
    str r6, [sp, #24]

sort_s_loop:
    add r1, r3, #1              @ set start to current index + 1
    cmp r3, r2
    bge sort_s_exit
    
    str r0, [sp, #4]            @ preserve caller registers
    str r1, [sp, #8]
    str r2, [sp, #28]           @ EDIT - preserving r2 as well
    str r3, [sp, #12]

    bl find_max_index_s
    mov r4, r0                  @ move output to r4
    
    ldr r0, [sp, #4]            @ retrieve registers
    ldr r1, [sp, #8]
    ldr r2, [sp, #28]
    ldr r3, [sp, #12]

    ldr r5, [r0, r3, lsl #2]    @ get values at current and max index
    ldr r6, [r0, r4, lsl #2]
    
    cmp r6, r5                  @ compare values and swap if needed
    bge sort_s_swap

    add r3, r3, #1              @ increment current index
    b sort_s_loop
    
sort_s_swap:
    str r5, [r0, r4, lsl #2]    @ store current value at previous max index
    str r6, [r0, r3, lsl #2]    @ store the max value at current spot
    add r3, r3, #1
    b sort_s_loop

sort_s_exit:
    ldr lr, [sp]
    add sp, sp, #32             @ reallocate memory
    
    bx lr
