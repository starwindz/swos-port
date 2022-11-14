; 16.09.2004. fixed crash occuring in release version due to not saving ebx
; 11.09.2004. converted to NASM syntax

section .text

bits 32

global _ChainSprite@4
global _UnchainSprite@4

; macro to get bits from bl, bh, cl and ch and
; put it in al

%macro GetBits 0
    rcl     bl, 1
    rcl     al, 1
    rcl     bh, 1
    rcl     al, 1
    rcl     cl, 1
    rcl     al, 1
    rcl     ch, 1
    rcl     al, 1
%endmacro

; macro put put bits from al into
; bl, bh, cl and ch

%macro PutBits 0
    rcl     al, 1
    rcl     bl, 1
    rcl     al, 1
    rcl     bh, 1
    rcl     al, 1
    rcl     cl, 1
    rcl     al, 1
    rcl     ch, 1
%endmacro

line_buffer_size equ 256 - 8
%define sprite            [ebp + 8]
%define chain_buffer      [ebp - 8]
%define line_buffer       [ebp - line_buffer_size + 8]
; ---------------------------
;  ChainSprite(Sprite *spr);
;
;  Procedure for "decoding" SWOS DAT files internal format,
;  in which sprites are held.
;
;  Karakas Zlatko, 16.04.2002.

_ChainSprite@4:
        push  ebp
        mov   ebp, esp
        sub   esp, line_buffer_size + 8
        push  esi
        push  edi
        push  ebx

        mov   edi, sprite          ; check for NULL pointer
        test  edi, edi
        jz    .out

        xor   eax, eax
        mov   esi, [edi]           ; esi -> sprite data
        movzx ecx, word [edi + 12] ; ecx = nlines
        movzx edx, word [edi + 14] ; edx = wquads
        mov   ebx, edx
        shl   edx, 3               ; edx = number of bytes on screen
        shl   ebx, 1               ; ebx = number of of 4 byte units

.next_line:
        push  esi                  ; save ptr to sprite data
        push  ecx                  ; save number of lines (height)
        lea   edi, line_buffer
        mov   eax, ebx
        shr   eax, 1               ; eax - num. quads

.next_quad:
        push  ebx                  ; save number of 4b units
        push  eax                  ; save number of quads

        push  esi

        push  edi
        lea   edi, chain_buffer
        mov   ax, [esi]
        add   esi, ebx             ; += number of 4b units
        mov   [edi + 0], ax
        mov   ax, [esi]
        add   esi, ebx
        mov   [edi + 2], ax
        mov   ax, [esi]
        add   esi, ebx
        mov   [edi + 4], ax
        mov   ax, [esi]
        mov   [edi + 6], ax
        pop   edi

        lea   esi, chain_buffer

        mov   ch, [esi + 0]
        mov   cl, [esi + 2]
        mov   bh, [esi + 4]
        mov   bl, [esi + 6]
        GetBits
        GetBits
        stosb
        GetBits
        GetBits
        stosb
        GetBits
        GetBits
        stosb
        GetBits
        GetBits
        stosb

        mov   ch, [esi + 1]
        mov   cl, [esi + 3]
        mov   bh, [esi + 5]
        mov   bl, [esi + 7]
        GetBits
        GetBits
        stosb
        GetBits
        GetBits
        stosb
        GetBits
        GetBits
        stosb
        GetBits
        GetBits
        stosb

        pop   esi
        pop   eax
        pop   ebx                  ; num. of quads * 2
        add   esi, 2
        dec   eax
        jnz   .next_quad

        pop   ecx                  ; number of lines
        pop   edi                  ; ptr to sprite data (pop into edi, not esi)

        push  edi
        push  ecx

        ; copy stuff back into sprite
        lea   esi, line_buffer
        mov   ecx, edx
        shr   ecx, 2
        rep   movsd

        pop   ecx
        pop   esi

        add   esi, edx
        dec   ecx
        jnz   .next_line

.out:
        pop   ebx
        pop   edi
        pop   esi
        add   esp, line_buffer_size + 8
        pop   ebp
        retn  4


; ---------------------------
;  UnchainSprite(Sprite *spr);
;
;  Procedure for "encoding" graphics into SWOS internal format
;  Needed for replacing SWOS sprites.
;
;  Karakas Zlatko, 18.09.2002.

_UnchainSprite@4:
        push  ebp
        mov   ebp, esp
        sub   esp, line_buffer_size + 8
        push  esi
        push  edi
        push  ebx

        mov   edi, sprite          ; check for NULL pointer
        test  edi, edi
        jz    .out

        mov   esi, [edi]           ; esi -> sprite data
        movzx eax, word [edi + 12] ; eax = nlines
        movzx edx, word [edi + 14] ; edx = wquads

.next_line:
        mov   ebx, edx
        push  eax                  ; save height
        push  esi                  ; save dest ptr for starting new line
        push  esi                  ; save dest working ptr
        mov   ecx, ebx             ; copy one line to buffer
        shl   ecx, 1
        lea   edi, line_buffer
        push  edi
        rep   movsd
        pop   esi                  ; esi -> line_buffer

.next_quad:
        push  esi                  ; save current position in line buffer
        push  ebx                  ; save loop counter (number of quads left)

        lodsb
        PutBits
        PutBits
        lodsb
        PutBits
        PutBits
        lodsb
        PutBits
        PutBits
        lodsb
        PutBits
        PutBits
        mov   [edi + 0], ch
        mov   [edi + 2], cl
        mov   [edi + 4], bh
        mov   [edi + 6], bl

        lodsb
        PutBits
        PutBits
        lodsb
        PutBits
        PutBits
        lodsb
        PutBits
        PutBits
        lodsb
        PutBits
        PutBits
        mov   [edi + 1], ch
        mov   [edi + 3], cl
        mov   [edi + 5], bh
        mov   [edi + 7], bl

        pop   ebx                  ; get loop counter
        pop   ecx                  ; current position in line buffer
        pop   esi                  ; esi = dest working ptr
        push  esi                  ; push dest back
        push  ecx                  ; push current line buffer position again
        mov   ecx, edx
        shl   ecx, 1               ; num. 4b units

        mov   ax, [edi + 0]
        mov   [esi], ax
        add   esi, ecx             ; += number of 4b units
        mov   ax, [edi + 2]
        mov   [esi], ax
        add   esi, ecx
        mov   ax, [edi + 4]
        mov   [esi], ax
        add   esi, ecx
        mov   ax, [edi + 6]
        mov   [esi], ax

        pop   esi                  ; current line buffer position
        add   esi, 8               ; advance 8 bytes
        pop   ecx                  ; dest
        add   ecx, 2               ; dest += 2 (next word)
        dec   ebx                  ; decrement quads counter
        push  ecx
        jnz   .next_quad

        pop   esi                  ; dest ptr (modified)
        pop   esi                  ; dest ptr (original)
        pop   eax                  ; num. lines, outer counter
        mov   ecx, edx
        shl   ecx, 3
        add   esi, ecx
        dec   eax
        jnz   .next_line

.out:
        pop   ebx
        pop   edi
        pop   esi
        add   esp, line_buffer_size + 8
        pop   ebp
        retn  4
