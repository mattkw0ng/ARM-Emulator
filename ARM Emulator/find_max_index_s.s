.global find_max_index_s

@ r0 - arr
@ r1 - start / max index
@ r2 - len
@ r3 - index
@ r4 - current num
@ r5 - current max

find_max_index_s:
    mov r3, r1                  @ start search at start
    sub sp, sp, #8              @ allocate space
    str r4, [sp]
    str r5, [sp, #4]
    ldr r5, [r0, r1, lsl #2]    @ load first value to r5

find_max_index_loop:
    cmp r3, r2
    beq find_max_index_exit

    ldr r4, [r0, r3, lsl #2]
    cmp r4, r5
    bge find_max_index_update
    
    add r3, r3, #1
    b find_max_index_loop
    
find_max_index_update:
    mov r1, r3
    mov r5, r4
    add r3, r3, #1
    b find_max_index_loop

find_max_index_exit:
    ldr r4, [sp]
    ldr r5, [sp, #4]
    add sp, sp, #8              @ return space on stack
    
    mov r0, r1
    bx lr
