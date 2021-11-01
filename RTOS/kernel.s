; Author: Sarker Nadir Afridi Azmi
; We want to use PSP because no other process will have access to that process memory
; That is the natural place to store it because we have a unique process stack for each stack
; Using PUSH and POP in assembly will not work as they will be push to MSP in handler mode

	.def pushR4ToR11Psp
	.def popR4ToR11Psp
	.def pushPsp

.thumb

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

.endm
