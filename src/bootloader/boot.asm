org 0x7c00     ; (directive) means "the program is loaded to 0x7c00, so add 0x7c00 to each pointer"
bits 16        ; 16 bits mode (same as use16)

%define ENDL 0x0D, 0x0A

;
; FAT12 header
;
jmp short start
nop

bdb_oem:                      db 'MSWIN4.1'               ; 8 bytes
bdb_bytes_per_sector:         dw 512
bdb_sectors_per_cluster:      db 1
bdb_reserved_sectors:         dw 1
bdb_fat_count:                db 2
bdb_dir_entries_count:        dw 0E0h
bdb_total_sectors:            dw 2880                     ; 2880 * 515 = 1.44MB
bdb_media_descriptor_type:    db 0F0h                     ; F0 = 3.5" floppy disk
bdb_sectors_per_fat:          dw 9                        ; 9 sectors/fat
bdb_sectors_per_track:        dw 18
bdb_heads:                    dw 2
bdb_hidden_sectors:           dd 0
bdb_large_sector_count:       dd 0

; extended boot record
erb_drive_number:             db 0                        ; 0x00 floppt, 0x80 hdd, useless
                              db 0                        ; reserved
erb_signature:                db 029h
erb_volume_id:                db 033h, 0c0h, 0a6h, 0b8h   ; 4B serial number, value doesn't matter
erb_volume_label:             db 'FOOBAR     '            ; 11B Volume label, padded with spaces
erb_system_id:                db 'FAT12   '               ; 8B, padded with spaces

;
; Code goes here
;

start:
    jmp main

; print a string to the screen
; Params:
;    - ds:si points to string
;
puts:
    ; save registers we will modify
    push si
    push ax

.loop:
    lodsb ; address at si, store its value in al, increment si by 1
    or al, al
    jz .done
  
    mov ah, 0x0e     ; call bios interrupt
    mov bh, 0        ; page_number = 0 (optional in here)
    int 0x10
    jmp .loop

.done:
    pop ax
    pop si
    ret

main:
    mov ax, 0003h
    int 10h           ; clear screen + set videomode to 3 (80 x 25 16 color)

    ; setup data segments
    mov ax, 0         ; can't write to ds/es directly
    mov ds, ax
    mov es, ax

    ; setup stack
    mov ss, ax
    mov sp, 0x7c00    ; stack grows downwards from where we are loaded in memory

    ; read something from floppy disk
    ; BIOS should set DL to drive number
    ; (TODO: In the lesson it's 'mov [erb_drive_number], dl')
    ; but the lecturer says 'copy TO dl'
    ; so I assume he had a typo
    mov dl, [erb_drive_number]

    mov ax, 1         ; LBA=1, second sector from disk
    mov cl, 1         ; 1 sector to read
    mov bx, 0x7e00    ; data should be after the bootloader
    call disk_read

    ; print message
    mov si, msg_hello
    call puts

    cli
    hlt

;
; Error handlers
;

floppy_error:
    mov si, msg_read_failed
    call puts
    jmp wait_key_and_reboot

wait_key_and_reboot:
    mov ah, 0
    int 016h                        ; wait for keypress
    jmp 0FFFFh:0                    ; jump to beginning of BIOS, should reboot

.halt:
    cli                             ; disable interrupts, this way the CPU can't get out of "halt" state
    hlt


;
; Disk routines
;

;
; Converts an LBA address to a CHS address
; Parameters:
;   - ax: LBA address
; Returns:
;   - cx [bits 0-5]: sector number
;   - cs [bits 6-15]: cylinder
;   - dh: head
;

lba_to_chs:
    push ax
    push dx

    xor dx, dx                            ; dx = 0
    div word [bdb_sectors_per_track]      ; ax = LBA / SectorsPerTrack
                                          ; dx = LBA % SectorsPerTrack

    inc dx                                ; dx = (LBA % SectorsPerTrack + 1) = sector
    mov cx, dx                            ; cx = sector

    xor dx, dx                            ; dx = 0
    div word [bdb_heads]                  ; ax = (LBA / SectorsPerTrack) / Heads = cylinder
                                          ; dx = (LBA / SectorsPerTrack) % Heads = head
    mov dh, dl                            ; dh = head
    mov ch, al                            ; ch = cylinder (lower 8 bits)
    shl ah, 6
    or cl, ah                             ; put upper 2 bits of cylinder in CL

    pop ax
    mov dl, al                            ; restore only DL (DH returns the head)
    pop ax
    ret

;
; Read sectors from a disk
; Parameters:
;   - ax: LBA address
;   - cl: number of sectors to read (up to 128)
;   - dl: drive number
;   - es:bx: memory address where to store read data
;
disk_read:
    push ax
    push bx
    push cx
    push dx
    push di    

    push cx                               ; temporarily save CL (number of sectors to read)
    call lba_to_chs                       ; compute CHS
    pop ax                                ; AL = number of sectors to read

    mov ah, 02h
    mov di, 3                             ; retry 3 time (due to unreliability of floppy disks)

.retry:
    pusha                                 ; save all registers, we don't know what bios modifies
    stc                                   ; set carry flag, some BIOS'es don't set it
    int 013h                              ; carry flag cleared = success
    jnc .done

    ; read failed
    popa
    call disk_reset

    dec di
    test di, di
    jnz .retry

.fail
    ; all attempts are exhausted
    jmp floppy_error

.done:
    popa

    pop di                              ; restore registers modified
    pop dx
    pop cx
    pop bx
    pop ax
    ret 

;
; Reset disk controller
; Parameters:
;   -  dl: drive number
disk_reset:
    pusha
    mov ah, 0
    stc
    int 013h
    jc floppy_error
    popa
    ret

msg_hello:         db "Hello World!", ENDL, 0  ; msg_hello - is a label/pointer to address of array of bytes
msg_read_failed:   db "Read from disk failed!", ENDL, 0 

times 510 - ($ - $$) db 0 ; $$ - beginig of current sectio, $ - current line
dw 0xaa55
