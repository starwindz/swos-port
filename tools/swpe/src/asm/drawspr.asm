; 16.09.2004. fixed bug in clipping when x was negative; parameter odd is
;             introduced when drawing starts from odd sprite pixel
; 13.09.2004. broken into two routines: 1) in swspr.c, that does clipping, and
;             2) this one, that does drawing only
; 11.09.2004. converted to NASM syntax

section .text

bits 32

global __DrawSprite@32

MAX_WIDTH  equ 320
MAX_HEIGHT equ 200

; ---------------------------
; void _DrawSprite(byte *from, byte *where, uint delta, uint spr_delta,
;                  uint width, uint height, int col, int odd);
;
;  from      - pointer to source pixels
;  where     - pointer to destination screen
;  delta     - distance between end of last pixel sprite and end of screen
;  spr_delta - length of slack space in sprite
;  width     - width of sprite to draw
;  height    - height of sprite to draw
;  col       - drawing color
;  odd       - true if sprite starts on odd pixel
;
;  Core of sprite drawing routine. If color is < 0, draws sprites normally,
;  otherwise draws solid pixels with that color, no mather what their original
;  color is (except for color 8). Clipping is assumed to be done, and values
;  are not altered. When drawing sprite normally, this procedure uses no branch
;  to test if pixel is transparent. Drawing colored sprite was also without
;  branching, but the code was overbloated, so I reverted to plain ifs.
;
;  Karakas Zlatko, 12.03.2003.


%define from      [esp + 20]
%define where     [esp + 24]
%define delta     [esp + 28]
%define spr_delta [esp + 32]
%define width     [esp + 36]
%define height    [esp + 40]
%define col       [esp + 44]
odd equ 48


__DrawSprite@32:
        push  ebp
        push  ebx
        push  esi
        push  edi

        mov   ecx, height  ; fill corresponding registers with data
        mov   edx, width
        mov   ebx, delta
        mov   edi, where
        mov   esi, from
        mov   ebp, spr_delta

        mov   eax, col     ; colored sprite request?
        test  eax, eax
        jns   .col_next_line

.next_line:
        push  ecx          ; save number of lines
        push  edx          ; save width
        push  ebx          ; save delta
        mov   ecx, edx     ; set width counter

        mov   eax, [esp + odd + 12] ; see if sprite begins with odd pixel
        test  eax, eax              ; (nibble)
        jz    .next_byte

        lodsb              ; yes, load second pixel and start with it
        and   al, 0x0f
        xor   edx, edx
        jmp   .second_pixel

.next_byte:
        xor   eax, eax     ; must clear all bits
        xor   edx, edx     ; dl = 0, for comparision
        lodsb              ; al = sprite byte (2 pixels)
        ror   eax, 4       ; al = sprite pixel, high nibble = pixel 2
        mov   bl, [edi]    ; bl = dest byte
        or    bl, al       ; bl unchanged if al = 0
        cmp   dl, al       ; cf = 1, al != 0; cf = 0, al = 0
        sbb   dl, dl       ; dl = -1, al != 0; dl = 0, al = 0
        not   dl           ; dl = 0, al != 0; dl = -1, al = 0
        or    al, dl       ; al = al != 0 ? al : -1
        xor   edx, edx     ; dl must be zero
        and   bl, al       ; bl = spr byte, al != 0; bl = dest byte, al = 0
        mov   [edi], bl    ; *where = *p ? *p : *where
        inc   edi          ; where++
        dec   ecx
        jz    .end_bytes_loop
        shr   eax, 28      ; put second pixel into position

.second_pixel:
        mov   bl, [edi]    ; bl = dest byte
        or    bl, al       ; bl unchanged if al = 0
        cmp   dl, al       ; cf = 1, al != 0; cf = 0, al = 0
        sbb   dl, dl       ; dl = -1, al != 0; dl = 0, al = 0
        not   dl           ; dl = 0, al != 0; dl = -1, al = 0
        or    al, dl       ; al = al != 0 ? al : -1
        and   bl, al       ; bl = spr byte, al != 0; bl = dest byte, al = 0
        mov   [edi], bl    ; *where = *p ? *p : *where
        inc   edi          ; where++
        dec   ecx
        jnz   .next_byte

.end_bytes_loop:
        pop   ebx          ; ebx = delta
        pop   edx          ; restore width
        pop   ecx          ; restore number of lines
        add   edi, ebx     ; where += delta
        add   esi, ebp     ; spr data += sprite delta
        dec   ecx
        jnz   .next_line

.out:
        pop   edi
        pop   esi
        pop   ebx
        pop   ebp
        ret   32


.col_next_line:
        push  eax          ; save color
        push  ecx          ; save number of lines
        push  edx          ; save width
        push  ebx          ; save delta
        mov   ecx, edx     ; set width counter
        mov   dh, al       ; dh = color

        mov   eax, [esp + odd + 16]
        test  eax, eax
        jz    .col_next_byte

        lodsb
        and   al, 0x0f
        xor   edx, edx
        jmp   .next_pixel

.col_next_byte:
        xor   eax, eax     ; must clear all bits
        lodsb              ; get sprite byte
        ror   eax, 4       ; al = sprite pixel, high nibble = pixel 2
        test  al, al       ; transparent pixel?
        jz    .next_pixel

        mov   dl, dh       ; assume al != 8
        cmp   al, 8
        jnz   .write_pixel ; if al != 8 write color
        mov   dl, 8        ; if al = 8, write 8

.write_pixel:
        mov   [edi], dl

.next_pixel:
        inc   edi
        dec   ecx
        jz    .col_end_bytes_loop

        shr   eax, 28      ; put second pixel into position
        test  al, al       ; transparent pixel?
        jz    .next_pixel2
        mov   dl, dh       ; assume al != 8
        cmp   al, 8
        jnz   .write_pixel2
        mov   dl, 8

.write_pixel2:
        mov   [edi], dl

.next_pixel2:
        inc   edi
        dec   ecx
        jnz   .col_next_byte

.col_end_bytes_loop:
        pop   ebx          ; ebx = delta
        pop   edx          ; restore width
        pop   ecx          ; restore number of lines
        pop   eax          ; restore color
        add   edi, ebx     ; where += delta
        add   esi, ebp     ; spr data += sprite delta
        dec   ecx
        jnz   .col_next_line

        jmp   .out