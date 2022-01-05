.global merge_s

@ r0 - arr
@ r1 - aux
@ r2 - start
@ r3 - end
@ r4 - midpoint
@ r5 - pointer_left
@ r6 - pointer_right
@ r7 - i (loop index)
@ r8 - arr[some_index] / end + 1 
@ r9 - arr[some_index] (used for comparisons)

merge_s:
    sub sp, sp, #24
    str r4, [sp]
    str r5, [sp, #4]
    str r6, [sp, #8]
    str r7, [sp, #12]
    str r8, [sp, #16]
    str r9, [sp, #20]

    add r4, r2, r3              @ Calculate midpoint
    lsr r4, r4, #1              @ mid = mid / 2
    add r4, r4, #1              @ mid = mid + 1

    mov r5, r2                  @ pointer_left = start
    mov r6, r4                  @ pointer_right = mid + 1
    mov r7, r2                  @ i = start

merge_loop:
    cmp r7, r3                  @ End condition - move to copy aux to arr
    bgt merge_copy_arr

merge_if_left_limit:
    cmp r5, r4                  @ left pointer has reached limit
    bne merge_if_right_limit
    b merge_add_right           @ take value from right subarr

merge_if_right_limit:
    add r8, r3, #1
    cmp r6, r8                  @ right pointer has reached limit
    bne merge_compare_values
    b merge_add_left            @ take value from left subarr
    
merge_compare_values:
    ldr r8, [r0, r5, lsl #2]
    ldr r9, [r0, r6, lsl #2] 
    cmp r8, r9                  @ if arr[pointer_left] < arr[pointer_right]
    blt merge_add_left          @ take value from left subarr, else take from right

merge_add_right:
    ldr r8, [r0, r6, lsl #2]
    str r8, [r1, r7, lsl #2]    @ aux[i] = arr[pointer_right]
    add r6, r6, #1              @ increment pointer_right
    b merge_loop_end

merge_add_left:
    ldr r8, [r0, r5, lsl #2]
    str r8, [r1, r7, lsl #2]    @ aux[i] = arr[pointer_left]
    add r5, r5, #1              @ increment pointer_left

merge_loop_end:
    add r7, r7, #1              @ i ++
    b merge_loop                @ continue looping   

merge_copy_arr:
    mov r7, r2                  @ i = start
    
merge_copy_arr_loop:
    cmp r7, r3
    bgt merge_exit
    ldr r8, [r1, r7, lsl #2]
    str r8, [r0, r7, lsl #2]    @ a[i] = aux[i]
    add r7, r7, #1
    b merge_copy_arr_loop

merge_exit:
    ldr r4, [sp]
    ldr r5, [sp, #4]
    ldr r6, [sp, #8]
    ldr r7, [sp, #12]
    ldr r8, [sp, #16]
    ldr r9, [sp, #20]
    add sp, sp, #24
    
    bx lr
