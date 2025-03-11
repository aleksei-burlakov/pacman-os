bits 16

section .entry

extern __bss_start
extern __end

extern start
global entry

; Summary
; 1. Set up real-mode stack.
; 2. Enable A20 line.
; 3. Load the GDT.
; 4. Switch to protected mode by setting CR0.PE = 1.
; 5. Jump to 32-bit protected mode.
; 6. Set segment registers.
; 7. Clear BSS section.
; 8. Jump to the kernel (start).

entry:
    ; The cli instruction disables interrupts
    ; to ensure no unexpected behavior during the mode switch.
    cli

    ; save boot drive (passed by the BIOS in dl)
    mov [g_BootDrive], dl

    ; Initializes the stack at 0xFFF0 for safe function calls
    mov ax, ds
    mov ss, ax
    mov sp, 0xFFF0
    mov bp, sp

    ; switch to protected mode
    call EnableA20          ; 2 - Enable A20 gate
    call LoadGDT            ; 3 - Load GDT

    ; ============================
    ; Enable VGA Mode 0x12 (640x480x16)
    ; ============================
    mov ax, 0x0012    ; VGA Mode 0x12 (640x480, 16 colors)
    int 0x10          ; Call BIOS

    ; 4 - set protection enable flag in CR0
    mov eax, cr0
    or al, 1
    mov cr0, eax

    ; 5 - far jump into protected mode
    jmp dword 08h:.pmode

.pmode:
    ; we are now in protected mode! (32-bit mode)
    [bits 32]
    
    ; 6 - setup segment registers
    mov ax, 0x10
    mov ds, ax
    mov ss, ax
    
    ; clear bss (uninitialized data)
    mov edi, __bss_start
    mov ecx, __end
    sub ecx, edi
    mov al, 0
    cld
    rep stosb               ; memory with 0x00

    ; expect boot drive in dl, send it as argument to cstart function
    ; (jump ro kernel)
    xor edx, edx
    mov dl, [g_BootDrive]
    push edx
    call start              ; main.c --> start(uint16_t bootDrive)

    ; Calls the kernel's start function, passing the boot drive number in dl
    cli
    hlt                      ; If start returns, the CPU is halted

; The A20 line allows access to memory beyond 1MB. If it’s disabled, memory wraps around.
EnableA20:
    [bits 16]   ; Must run in real mode.
    ; Disables the keyboard to prevent interference.
    call A20WaitInput
    mov al, KbdControllerDisableKeyboard
    out KbdControllerCommandPort, al

    ; read control output port
    call A20WaitInput
    mov al, KbdControllerReadCtrlOutputPort
    out KbdControllerCommandPort, al

    call A20WaitOutput
    in al, KbdControllerDataPort
    push eax

    ; write control output port
    call A20WaitInput
    mov al, KbdControllerWriteCtrlOutputPort
    out KbdControllerCommandPort, al
    
    call A20WaitInput
    pop eax
    or al, 2                                    ; bit 2 = A20 bit
    out KbdControllerDataPort, al

    ; enable keyboard (Enables the A20 line by setting bit 2 in the controller output port.)
    call A20WaitInput
    mov al, KbdControllerEnableKeyboard
    out KbdControllerCommandPort, al

    call A20WaitInput
    ret


A20WaitInput:
    [bits 16]
    ; wait until status bit 2 (input buffer) is 0
    ; by reading from command port, we read status byte
    in al, KbdControllerCommandPort
    test al, 2
    jnz A20WaitInput
    ret

A20WaitOutput:
    [bits 16]
    ; wait until status bit 1 (output buffer) is 1 so it can be read
    in al, KbdControllerCommandPort
    test al, 1
    jz A20WaitOutput
    ret


LoadGDT:
    [bits 16]
    lgdt [g_GDTDesc]
    ret



KbdControllerDataPort               equ 0x60
KbdControllerCommandPort            equ 0x64
KbdControllerDisableKeyboard        equ 0xAD
KbdControllerEnableKeyboard         equ 0xAE
KbdControllerReadCtrlOutputPort     equ 0xD0
KbdControllerWriteCtrlOutputPort    equ 0xD1

ScreenBuffer                        equ 0xB8000

g_GDT:      ; NULL descriptor
            dq 0

            ; 32-bit code segment
            dw 0FFFFh                   ; limit (bits 0-15) = 0xFFFFF for full 32-bit range
            dw 0                        ; base (bits 0-15) = 0x0
            db 0                        ; base (bits 16-23)
            db 10011010b                ; access (present, ring 0, code segment, executable, direction 0, readable)
            db 11001111b                ; granularity (4k pages, 32-bit pmode) + limit (bits 16-19)
            db 0                        ; base high

            ; 32-bit data segment
            dw 0FFFFh                   ; limit (bits 0-15) = 0xFFFFF for full 32-bit range
            dw 0                        ; base (bits 0-15) = 0x0
            db 0                        ; base (bits 16-23)
            db 10010010b                ; access (present, ring 0, data segment, executable, direction 0, writable)
            db 11001111b                ; granularity (4k pages, 32-bit pmode) + limit (bits 16-19)
            db 0                        ; base high

            ; 16-bit code segment
            dw 0FFFFh                   ; limit (bits 0-15) = 0xFFFFF
            dw 0                        ; base (bits 0-15) = 0x0
            db 0                        ; base (bits 16-23)
            db 10011010b                ; access (present, ring 0, code segment, executable, direction 0, readable)
            db 00001111b                ; granularity (1b pages, 16-bit pmode) + limit (bits 16-19)
            db 0                        ; base high

            ; 16-bit data segment
            dw 0FFFFh                   ; limit (bits 0-15) = 0xFFFFF
            dw 0                        ; base (bits 0-15) = 0x0
            db 0                        ; base (bits 16-23)
            db 10010010b                ; access (present, ring 0, data segment, executable, direction 0, writable)
            db 00001111b                ; granularity (1b pages, 16-bit pmode) + limit (bits 16-19)
            db 0                        ; base high

g_GDTDesc:  dw g_GDTDesc - g_GDT - 1    ; limit = size of GDT
            dd g_GDT                    ; address of GDT

g_BootDrive: db 0
