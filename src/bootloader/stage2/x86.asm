; Summary: handling disk operations and switching between real mode and protected mode
; Functions that call BIOS interrupts need real mode, so they:
; 1. Switch to real mode using x86_EnterRealMode.
; 2. Execute BIOS interrupt 13h for disk operations.
; 3. Store results in memory.
; 4. Switch back to protected mode using x86_EnterProtectedMode.


; x86_EnterRealMode: Switches from protected mode (32-bit) to real mode (16-bit).
; Disables the protected mode bit in CR0.
; Sets up the segment registers and enables interrupts.
;
; Possible problems: Not Fully Resetting All Registers
; When switching back to real mode, only ds and ss are set to zero.
; Other segment registers (es, fs, gs) are not reset.
; Some BIOS calls might rely on a properly initialized es register.
; Fix: Add mov es, ax to ensure es is reset
%macro x86_EnterRealMode 0
    [bits 32]
    jmp word 18h:.pmode16         ; 1 - jump to 16-bit protected mode segment

.pmode16:
    [bits 16]
    ; 2 - disable protected mode bit in cr0
    mov eax, cr0
    and al, ~1
    mov cr0, eax

    ; 3 - jump to real mode
    jmp word 00h:.rmode

.rmode:
    ; 4 - setup segments
    mov ax, 0
    mov ds, ax
    mov ss, ax

    ; 5 - enable interrupts
    sti

%endmacro

; Switches from real mode (16-bit) to protected mode (32-bit).
; Sets the protection enable flag in CR0.
; Performs a far jump to reload segment registers for protected mode.
%macro x86_EnterProtectedMode 0
    cli

    ; 4 - set protection enable flag in CR0
    mov eax, cr0
    or al, 1
    mov cr0, eax

    ; 5 - far jump into protected mode
    jmp dword 08h:.pmode


.pmode:
    ; we are now in protected mode!
    [bits 32]
    
    ; 6 - setup segment registers
    mov ax, 0x10
    mov ds, ax
    mov ss, ax

%endmacro

; Convert linear address to segment:offset address
; Args:
;    1 - linear address
;    2 - (out) target segment (e.g. es)
;    3 - target 32-bit register to use (e.g. eax)
;    4 - target lower 16-bit half of #3 (e.g. ax)
;
; Potential Register Corruption
; Problem: This macro modifies %3 (a general-purpose register).
;   If %3 is a caller-saved register (eax, ecx, etc.), it might corrupt other parts of the code.
; Fix: Preserve the original value.
%macro LinearToSegOffset 4

    mov %3, %1      ; linear address to eax
    shr %3, 4
    mov %2, %4
    mov %3, %1      ; linear address to eax
    and %3, 0xf

%endmacro

; Writes a byte to a specified I/O port using out dx, al
global x86_outb
x86_outb:
    [bits 32]
    mov dx, [esp + 4]
    mov al, [esp + 8]
    out dx, al
    ret

; Reads a byte from a specified I/O port using in al, dx
global x86_inb
x86_inb:
    [bits 32]
    mov dx, [esp + 4]
    xor eax, eax
    in al, dx
    ret

;
; Disk Operations (via BIOS Interrupt 13h)
; These functions use BIOS interrupt 13h to interact with the disk drive.
;

; Retrieves drive parameters (type, cylinders, sectors, heads).
; Uses int 13h, AH=08h to get drive geometry.
; Stores results in a buffer.
;
; Possibe problems: Incorrect Cylinder Calculation
; The cylinder number is stored across ch (lower 8 bits) and cl (upper 2 bits).
; The code extracts bh (upper 2 bits of cl) using:
; Problem: cl is not masked before shifting
; If cl has unexpected bits set (beyond 6-7), this might corrupt the value.
global x86_Disk_GetDriveParams
x86_Disk_GetDriveParams:
    [bits 32]

    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp         ; initialize new call frame

    x86_EnterRealMode

    [bits 16]

    ; save regs
    push es
    push bx
    push esi
    push di

    ; call int13h
    mov dl, [bp + 8]    ; dl - disk drive
    mov ah, 08h
    mov di, 0           ; es:di - 0000:0000
    mov es, di
    stc
    int 13h

    ; out params
    mov eax, 1
    sbb eax, 0

    ; drive type from bl
    LinearToSegOffset [bp + 12], es, esi, si
    mov [es:si], bl

    ; cylinders
    mov bl, ch          ; cylinders - lower bits in ch
    mov bh, cl          ; cylinders - upper bits in cl (6-7)
    shr bh, 6
    inc bx

    LinearToSegOffset [bp + 16], es, esi, si
    mov [es:si], bx

    ; sectors
    xor ch, ch          ; sectors - lower 5 bits in cl
    and cl, 3Fh
    
    LinearToSegOffset [bp + 20], es, esi, si
    mov [es:si], cx

    ; heads
    mov cl, dh          ; heads - dh
    inc cx

    LinearToSegOffset [bp + 24], es, esi, si
    mov [es:si], cx

    ; restore regs
    pop di
    pop esi
    pop bx
    pop es

    ; return

    push eax

    x86_EnterProtectedMode

    [bits 32]

    pop eax

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret


; Resets the disk drive.
; Uses int 13h, AH=00h.
; Returns 1 on success, 0 on failure.
global x86_Disk_Reset
x86_Disk_Reset:
    [bits 32]

    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp          ; initialize new call frame


    x86_EnterRealMode

    mov ah, 0
    mov dl, [bp + 8]    ; dl - drive
    stc
    int 13h

    mov eax, 1
    sbb eax, 0           ; 1 on success, 0 on fail   

    push eax

    x86_EnterProtectedMode

    pop eax

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret


; Reads sectors from the disk.
; Uses int 13h, AH=02h.
; Takes drive, cylinder, sector, head, count, and buffer address as arguments.
;
; Possible problems: No Check for Read Errors
; Problem: The function assumes int 13h, AH=02h will succeed.
; Fix: Add error checking using the carry flag (CF):
global x86_Disk_Read
x86_Disk_Read:

    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp          ; initialize new call frame

    x86_EnterRealMode

    ; save modified regs
    push ebx
    push es

    ; setup args
    mov dl, [bp + 8]    ; dl - drive

    mov ch, [bp + 12]    ; ch - cylinder (lower 8 bits)
    mov cl, [bp + 13]    ; cl - cylinder to bits 6-7
    shl cl, 6
    
    mov al, [bp + 16]    ; cl - sector to bits 0-5
    and al, 3Fh
    or cl, al

    mov dh, [bp + 20]   ; dh - head

    mov al, [bp + 24]   ; al - count

    LinearToSegOffset [bp + 28], es, ebx, bx

    ; call int13h
    mov ah, 02h
    stc
    int 13h

    ; set return value
    mov eax, 1
    sbb eax, 0           ; 1 on success, 0 on fail   

    ; restore regs
    pop es
    pop ebx

    push eax

    x86_EnterProtectedMode

    pop eax

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret
