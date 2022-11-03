; Author: Sarker Nadir Afridi Azmi
; We want to use PSP because no other process will have access to that process memory
; That is the natural place to store it because we have a unique process stack for each stack
; Using PUSH and POP in assembly will not work as they will be push to MSP in handler mode

	.def getMsp
	.def setPspMode
	.def setPsp
	.def getPsp
	.def pushR4ToR11Psp
	.def popR4ToR11Psp
	.def pushPsp
	.def disablePrivilegeMode

.thumb

getMsp:
	MRS R0, MSP
	BX LR

; Set's the threads to be run using PSP. By default, threads make use of the MSP,
; but the kernel should run using MSP and threads should run using the PSP,
; in an OS environment. This ensures a separation of kernel space and user
; space.

setPspMode:
	MRS R1, CONTROL	; CONTROL is a core register and is not memory mapped. It
    				; can be called by using its name.
	ORR R1, #2		; Set the ASP to use the PSP in thread mode.
	MSR CONTROL, R1
	ISB				; Page 88, ISB instruction is needed to ensure that all instructions
                    ; after the ISB execute using the new stack.
	BX LR

setPsp:
	MSR PSP, R0		; Move the new stack pointer into the process stack
	BX LR

getPsp:
    MRS R0, PSP
	BX LR

pushR4ToR11Psp:
	MRS R0, PSP
	SUB R0, #4
	STR R11, [R0]
	SUB R0, #4
	STR R10, [R0]
	SUB R0, #4
	STR R9, [R0]
	SUB R0, #4
	STR R8, [R0]
	SUB R0, #4
	STR R7, [R0]
	SUB R0, #4
	STR R6, [R0]
	SUB R0, #4
	STR R5, [R0]
	SUB R0, #4
	STR R4, [R0]
	MSR PSP, R0
	BX LR

popR4ToR11Psp:
	MRS R0, PSP
	LDR R4, [R0]
	ADD R0, #4
	LDR R5, [R0]
	ADD R0, #4
	LDR R6, [R0]
	ADD R0, #4
	LDR R7, [R0]
	ADD R0, #4
	LDR R8, [R0]
	ADD R0, #4
	LDR R9, [R0]
	ADD R0, #4
	LDR R10, [R0]
	ADD R0, #4
	LDR R11, [R0]
	ADD R0, #4
	MSR PSP, R0
	BX LR

pushPsp:
	MRS R1, PSP
	SUB R1, #4
	STR R0, [R1]
	MSR PSP, R1
	BX LR

; If bit 0 of the Control Register on page 88 is enabled, no changes can be made to any
; register by unprivileged software.
;
; This Raises the privilege level so that unprivileged software executes in the threads. This in
; combination with using the PSP for threads allows for separation of kernel space and user
; space.

disablePrivilegeMode:
	MRS R0, CONTROL
	ORR R0, #1		; Set the TMPL bit to turn of privilege mode for threads.
	MSR CONTROL, R0
	BX LR

.endm
