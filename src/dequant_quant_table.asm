; -----------------------------------------------------------------------------
; A function that dequantizes a data unit.  
; The function has signature:
;
;   int32_t dequant_data_unit(QuantTable *qt, int16_t du)
;
; Note that the parameters have already been passed in rdi, rsi, and rdx.  We
; just have to return the value in rax.
; -----------------------------------------------------------------------------

        global  dequant_data_unit
        extern zigzag

        section .text

dequant_data_unit:
; Quantable *qt - rdi
; int16_t *du   - rsi
; 
;        sub     rsp, 16
;        movdqu  xmm0, XMMWORD PTR [rsi]
;        mov     r9, rdi
;        xor     eax, eax
;        mov     rdi, rsi
;        lea     r8, [rsp-120]
;        movaps  XMMWORD PTR [rsp-120], xmm0
;        movdqu  xmm0, XMMWORD PTR [rsi+16]
;        movaps  XMMWORD PTR [rsp-104], xmm0
;        movdqu  xmm0, XMMWORD PTR [rsi+32]
;        movaps  XMMWORD PTR [rsp-88], xmm0
;        movdqu  xmm0, XMMWORD PTR [rsi+48]
;        movaps  XMMWORD PTR [rsp-72], xmm0
;        movdqu  xmm0, XMMWORD PTR [rsi+64]
;        movaps  XMMWORD PTR [rsp-56], xmm0
;        movdqu  xmm0, XMMWORD PTR [rsi+80]
;        movaps  XMMWORD PTR [rsp-40], xmm0
;        movdqu  xmm0, XMMWORD PTR [rsi+96]
;        movaps  XMMWORD PTR [rsp-24], xmm0
;        movdqu  xmm0, XMMWORD PTR [rsi+112]
;        movaps  XMMWORD PTR [rsp-8], xmm0
;.L2:
;        movzx   esi, BYTE PTR "zigzag"[rax]
;        movzx   ecx, WORD PTR [r8+rax*2]
;        add     rax, 1
;        imul    cx, WORD PTR [r9+2+rsi*2]
;        mov     WORD PTR [rdi+rsi*2], cx
;        cmp     rax, 64
;        jne     .L2
;        xor     eax, eax
;        add     rsp, 16
;        ret
;        sub     rsp, 16
;        mov     rax, 0                ; result (rax) initially holds x
;        
;        movdqu  xmm0, [rsi] ; Moves 8 
;        mov     r9, rdi
;        xor     eax, eax
;        mov     rdi, rsi
;        lea     r8, [rsp-120]
;        movaps  [rsp-120], xmm0
;        movdqu  xmm0, [rsi+16]
;        movaps  [rsp-104], xmm0
;        movdqu  xmm0, [rsi+32]
;        movaps  [rsp-88], xmm0
;        movdqu  xmm0, [rsi+48]
;        movaps  [rsp-72], xmm0
;        movdqu  xmm0, [rsi+64]
;        movaps  [rsp-56], xmm0
;        movdqu  xmm0, [rsi+80]
;        movaps  [rsp-40], xmm0
;        movdqu  xmm0, [rsi+96]
;        movaps  [rsp-24], xmm0
;        movdqu  xmm0, [rsi+112]
;        movaps  [rsp-8], xmm0
;.L2:
;        movzx   esi, byte [zigzag + rax]
;        movzx   ecx, word [r8+rax*2]
;        add     rax, 1
;        imul    cx, [r9+2+rsi*2]
;        mov     [rdi+rsi*2], cx
;        cmp     rax, 64
;        jne     .L2
;        xor     eax, eax
;        add     rsp, 16
;        ret

;        section .data 

;zigzag2: dw 0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5, 12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6, 7, 14, 21, 28, 35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51, 58, 69, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63;
