
#############################################################################################
# FS main thread hook
#############################################################################################
#.extern createDevThread_hookC
#    .globl createDevThread_hook
#createDevThread_hook:
#	push {r0}
#	mov r0, r4
#	bl createDevThread_hookC
#	pop {r0,r4-r8,pc}


#############################################################################################
# devices handle hooks
#############################################################################################
.extern getMdDeviceById
    .globl getMdDeviceById_hook
getMdDeviceById_hook:
	mov r4, r0
    push {lr}
	bl getMdDeviceById
    pop {lr}
	cmp r0, #0
	moveq r0, r4
	bxeq lr
	pop {r4,r5,pc}
