; Snowflake pattern generator for 6809
; ACIA at address $A000
	ORG $1024

START
	CLR ROW             ; R = 0

ROWLOOP
	LDA ROW
	CMPA #10
	BLO USEROW
	LDB #18
	SUBB ROW            ; B = 18 - r
	BRA GETPAT

USEROW
	LDB ROW             ; B = r

GETPAT
	LSLB                ; B = B * 2
	LDX #PATTERN
	LDD B,X             ; Load pattern word
	STD CURRPAT         ; Save pattern for this row

	; Process 19 columns
	CLR COL             ; C = 0

COLLOOP
	LDB COL
	CMPB #10
	BLO USECOL
	LDA #18
	SUBA COL            ; A = 18 - c
	BRA CALCSHIFT

USECOL
	LDA COL             ; A = c

CALCSHIFT
	PSHS A
	LDA #9
	SUBA ,S+            ; A = shift amount
	STA SHIFTCNT        ; Save shift count separately!

	; Load pattern and shift right
	LDD CURRPAT         ; Get the pattern (A:high, B:low)
	PSHS B              ; Preserve low byte on stack
	LDB SHIFTCNT        ; Get shift count in B
	BEQ NOSHIFT         ; If 0, skip shifting

SHIFTLOOP
	LSRA                ; Shift high byte right
	ROR ,S              ; Rotate low byte on stack right with carry
	DECB                ; Decrement counter in B
	BNE SHIFTLOOP

NOSHIFT
	LDB ,S+             ; Restore shifted low byte to B

TESTBIT
	BITB #1
	BEQ PRINTSP
	LDA #'*'
	BRA PRINTCH

PRINTSP
	LDA #' '

PRINTCH
	STA ACIA_D
	INC COL
	LDA COL
	CMPA #19
	LBLO COLLOOP

	LDA #$0A
	STA ACIA_D
	LDA #$0D
	STA ACIA_D

	INC ROW
	LDA ROW
	CMPA #19
	LBLO ROWLOOP

	SWI


PATTERN
	FDB 1,5,163,97,229,19,9,293,147,1023

; Variables
ACIA_D  	EQU $A001
CURRPAT     RMB 2
SHIFTCNT    RMB 1
ROW         RMB 1
COL         RMB 1

	END START