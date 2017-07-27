default rel
global alg0
global alg1
global alg2
global alg3
global alg4
global alg5
global alg6
global alg7
section .text

alg0:
; fetch working code from memory, load into ymm0-ymm7
vmovdqa     ymm0,[rcx]
vmovdqa     ymm1,[rcx+32]
vmovdqa     ymm2,[rcx+64]
vmovdqa     ymm3,[rcx+96]
vmovdqa     ymm4,[rcx+128]
vmovdqa     ymm5,[rcx+160]
vmovdqa     ymm6,[rcx+192]
vmovdqa     ymm7,[rcx+224]

; DO WORK

; Get rng seed 
movzx       r10,word  [r8]

; Compute rng start
shl         r10,8
add         r10,rdx

; Load ff_mask
vmovdqa     ymm11,[ff_mask]

; Load rng value
vmovdqa     ymm8,[r10]

; Get high bits of ymm0
vperm2f128  ymm9,ymm0,ymm0,1

; Do 128 bit shift on high bits
vpsllw      xmm9,1

; Do 128 bit shift on low bits
vpsllw      xmm0,1

; Reinsert high bits
vinsertf128 ymm0,ymm0,xmm9,1
 
; OR in the rng value
vorpd       ymm0,ymm0,ymm8

; Mask it
vandpd      ymm0,ymm0,ymm11



; Repeat for the rest
vmovdqa     ymm8,[r10+32]
vperm2f128  ymm9,ymm1,ymm1,1
vpsllw      xmm9,1
vpsllw      xmm1,1
vinsertf128 ymm1,ymm1,xmm9,1
vorpd       ymm1,ymm1,ymm8
vandpd      ymm1,ymm1,ymm11

vmovdqa     ymm8,[r10++64]
vperm2f128  ymm9,ymm2,ymm2,1
vpsllw      xmm9,1
vpsllw      xmm2,1
vinsertf128 ymm2,ymm2,xmm9,1
vorpd       ymm2,ymm2,ymm8
vandpd      ymm2,ymm2,ymm11

vmovdqa     ymm8,[r10+96]
vperm2f128  ymm9,ymm3,ymm3,1
vpsllw      xmm9,1
vpsllw      xmm3,1
vinsertf128 ymm3,ymm3,xmm9,1
vorpd       ymm3,ymm3,ymm8
vandpd      ymm3,ymm3,ymm11

vmovdqa     ymm8,[r10+128]
vperm2f128  ymm9,ymm4,ymm4,1
vpsllw      xmm9,1
vpsllw      xmm4,1
vinsertf128 ymm4,ymm4,xmm9,1
vorpd       ymm4,ymm4,ymm8
vandpd      ymm4,ymm4,ymm11

vmovdqa     ymm8,[r10+160]
vperm2f128  ymm9,ymm5,ymm5,1
vpsllw      xmm9,1
vpsllw      xmm5,1
vinsertf128 ymm5,ymm5,xmm9,1
vorpd       ymm5,ymm5,ymm8
vandpd      ymm5,ymm5,ymm11

vmovdqa     ymm8,[r10+192]
vperm2f128  ymm9,ymm6,ymm6,1
vpsllw      xmm9,1
vpsllw      xmm6,1
vinsertf128 ymm6,ymm6,xmm9,1
vorpd       ymm6,ymm6,ymm8
vandpd      ymm6,ymm6,ymm11

vmovdqa     ymm8,[r10+224]
vperm2f128  ymm9,ymm7,ymm7,1
vpsllw      xmm9,1
vpsllw      xmm7,1
vinsertf128 ymm7,ymm7,xmm9,1
vorpd       ymm7,ymm7,ymm8
vandpd      ymm7,ymm7,ymm11


movzx         r10,word  [r8]

; Fetch next seed value
movzx         r10,word  [rax+r10*2]
mov           [r8],r10

; return working code from ymm0-ymm7
vmovdqa     [rcx],ymm0
vmovdqa     [rcx+32],ymm1
vmovdqa     [rcx+64],ymm2
vmovdqa     [rcx+96],ymm3
vmovdqa     [rcx+128],ymm4
vmovdqa     [rcx+160],ymm5
vmovdqa     [rcx+192],ymm6
vmovdqa     [rcx+224],ymm7

vzeroupper

ret


alg1:
; fetch working code from memory, load into ymm0-ymm7
vmovdqa     ymm0,[rcx]
vmovdqa     ymm1,[rcx+32]
vmovdqa     ymm2,[rcx+64]
vmovdqa     ymm3,[rcx+96]
vmovdqa     ymm4,[rcx+128]
vmovdqa     ymm5,[rcx+160]
vmovdqa     ymm6,[rcx+192]
vmovdqa     ymm7,[rcx+224]

; DO WORK

; Get rng seed 
movzx       r10,word  [r8]

; Compute rng start
shl         r10,8
add         r10,rdx

; Load rng value
vmovdqa     ymm8,[r10]

; Do 128 bit add on low bits
vpaddw      xmm9,xmm0,xmm8

; Swap the 128 bit lanes
vperm2f128  ymm0,ymm0,ymm0,1
vperm2f128  ymm8,ymm8,ymm8,1

; Do 128 bit add on low bits
vpaddw      xmm10,xmm0,xmm8

vinsertf128 ymm0,ymm9,xmm10,1 



; Repeat for the rest
vmovdqa     ymm8,[r10+32]
vpaddw      xmm9,xmm1,xmm8
vperm2f128  ymm1,ymm1,ymm1,1
vperm2f128  ymm8,ymm8,ymm8,1
vpaddw      xmm10,xmm1,xmm8
vinsertf128 ymm1,ymm9,xmm10,1 

vmovdqa     ymm8,[r10+64]
vpaddw      xmm9,xmm2,xmm8
vperm2f128  ymm2,ymm2,ymm2,1
vperm2f128  ymm8,ymm8,ymm8,1
vpaddw      xmm10,xmm2,xmm8
vinsertf128 ymm2,ymm9,xmm10,1 

vmovdqa     ymm8,[r10+96]
vpaddw      xmm9,xmm3,xmm8
vperm2f128  ymm3,ymm3,ymm3,1
vperm2f128  ymm8,ymm8,ymm8,1
vpaddw      xmm10,xmm3,xmm8
vinsertf128 ymm3,ymm9,xmm10,1  

vmovdqa     ymm8,[r10+128]
vpaddw      xmm9,xmm4,xmm8
vperm2f128  ymm4,ymm4,ymm4,1
vperm2f128  ymm8,ymm8,ymm8,1
vpaddw      xmm10,xmm4,xmm8
vinsertf128 ymm4,ymm9,xmm10,1 

vmovdqa     ymm8,[r10+160]
vpaddw      xmm9,xmm5,xmm8
vperm2f128  ymm5,ymm5,ymm5,1
vperm2f128  ymm8,ymm8,ymm8,1
vpaddw      xmm10,xmm5,xmm8
vinsertf128 ymm5,ymm9,xmm10,1 

vmovdqa     ymm8,[r10+192]
vpaddw      xmm9,xmm6,xmm8
vperm2f128  ymm6,ymm6,ymm6,1
vperm2f128  ymm8,ymm8,ymm8,1
vpaddw      xmm10,xmm6,xmm8
vinsertf128 ymm6,ymm9,xmm10,1  

vmovdqa     ymm8,[r10+224]
vpaddw      xmm9,xmm7,xmm8
vperm2f128  ymm7,ymm7,ymm7,1
vperm2f128  ymm8,ymm8,ymm8,1
vpaddw      xmm10,xmm7,xmm8
vinsertf128 ymm7,ymm9,xmm10,1 


vandpd      ymm0,ymm0,[ff_mask]
vandpd      ymm1,ymm1,[ff_mask]
vandpd      ymm2,ymm2,[ff_mask]
vandpd      ymm3,ymm3,[ff_mask]
vandpd      ymm4,ymm4,[ff_mask]
vandpd      ymm5,ymm5,[ff_mask]
vandpd      ymm6,ymm6,[ff_mask]
vandpd      ymm7,ymm7,[ff_mask]


movzx         r10,word  [r8]

; Fetch next seed value
movzx         r10,word  [rax+r10*2]
mov           [r8],r10

; return working code from ymm0-ymm7
vmovdqa     [rcx],ymm0
vmovdqa     [rcx+32],ymm1
vmovdqa     [rcx+64],ymm2
vmovdqa     [rcx+96],ymm3
vmovdqa     [rcx+128],ymm4
vmovdqa     [rcx+160],ymm5
vmovdqa     [rcx+192],ymm6
vmovdqa     [rcx+224],ymm7

vzeroupper

ret


alg2:
; fetch working code from memory, load into ymm0-ymm7
vmovdqa     ymm0,[rcx]
vmovdqa     ymm1,[rcx+32]
vmovdqa     ymm2,[rcx+64]
vmovdqa     ymm3,[rcx+96]
vmovdqa     ymm4,[rcx+128]
vmovdqa     ymm5,[rcx+160]
vmovdqa     ymm6,[rcx+192]
vmovdqa     ymm7,[rcx+224]

; Get rng seed 
movzx       r10,word  [r8]

; Compute rng start
shl         r10,5
add         r10,rdx

; Load rng value
;;; should i use a set value? is it faster to clear and then load a single?
vmovdqa     ymm8,[r10]
;carry = ymm8

; Get a top bit mask
vmovdqa     ymm15,[alg2_mask1]
vmovdqa     ymm14,[alg2_mask2]

; Get next carry
; Shift current value
vpslldq     xmm9,xmm7,14
; Swap lanes
vperm2f128  ymm9,ymm9,ymm9,1
; Mask to get the carry bit
vandpd      ymm9,ymm9,ymm15
; next carry = ymm9

; Shift current value down 1 byte cell
vpsrldq     xmm10,xmm7,2
; Get high
vperm2f128  ymm13,ymm7,ymm7,1
; get carry across lanes
vpslldq     xmm12,xmm13,14
; OR it with the low bits
vorpd       xmm10,xmm10,xmm12
; Shift high bits
vpsrldq     xmm11,xmm13,2
; Reassemble into a 256
vinsertf128 ymm10,ymm10,xmm11,1 
; cur_val srl = ymm10

; Get extra carry
vandpd      ymm11,ymm10,ymm14
; or with carry
vorpd       ymm8,ymm8,ymm11

; Mask srl value
vandpd      ymm10,ymm10,[alg2_mask3]
; combine with carry
vorpd       ymm8,ymm8,ymm10

; Make part 1
;lo
vpsrlw      xmm10,xmm7,1
;high
vpsrlw      xmm11,xmm13,1
vinsertf128 ymm10,ymm10,xmm11,1 
vandpd      ymm11,ymm10,[alg2_mask4]
; or with carry
vorpd       ymm8,ymm8,ymm11

; make part 2
;lo
vpsllw      xmm7,xmm7,1
;high
vpsllw      xmm10,xmm13,1
vinsertf128 ymm7,ymm7,xmm10,1 
vandpd      ymm7,ymm7,[alg2_mask5]
; or in carry
vorpd       ymm7,ymm7,ymm8

; carry/next carry now swapped



vpslldq     xmm8,xmm6,14
vperm2f128  ymm8,ymm8,ymm8,1
vandpd      ymm8,ymm8,ymm15
vpsrldq     xmm10,xmm6,2
vperm2f128  ymm13,ymm6,ymm6,1
vpslldq     xmm12,xmm13,14
vorpd       xmm10,xmm10,xmm12
vpsrldq     xmm11,xmm13,2
vinsertf128 ymm10,ymm10,xmm11,1 
vandpd      ymm11,ymm10,ymm14
vorpd       ymm9,ymm9,ymm11
vandpd      ymm10,ymm10,[alg2_mask3]
vorpd       ymm9,ymm9,ymm10
vpsrlw      xmm10,xmm6,1
vpsrlw      xmm11,xmm13,1
vinsertf128 ymm10,ymm10,xmm11,1 
vandpd      ymm11,ymm10,[alg2_mask4]
vorpd       ymm9,ymm9,ymm11
vpsllw      xmm6,xmm6,1
vpsllw      xmm10,xmm13,1
vinsertf128 ymm6,ymm6,xmm10,1 
vandpd      ymm6,ymm6,[alg2_mask5]
vorpd       ymm6,ymm6,ymm9


vpslldq     xmm9,xmm5,14
vperm2f128  ymm9,ymm9,ymm9,1
vandpd      ymm9,ymm9,ymm15
vpsrldq     xmm10,xmm5,2
vperm2f128  ymm13,ymm5,ymm5,1
vpslldq     xmm12,xmm13,14
vorpd       xmm10,xmm10,xmm12
vpsrldq     xmm11,xmm13,2
vinsertf128 ymm10,ymm10,xmm11,1 
vandpd      ymm11,ymm10,ymm14
vorpd       ymm8,ymm8,ymm11
vandpd      ymm10,ymm10,[alg2_mask3]
vorpd       ymm8,ymm8,ymm10
vpsrlw      xmm10,xmm5,1
vpsrlw      xmm11,xmm13,1
vinsertf128 ymm10,ymm10,xmm11,1 
vandpd      ymm11,ymm10,[alg2_mask4]
vorpd       ymm8,ymm8,ymm11
vpsllw      xmm5,xmm5,1
vpsllw      xmm10,xmm13,1
vinsertf128 ymm5,ymm5,xmm10,1 
vandpd      ymm5,ymm5,[alg2_mask5]
vorpd       ymm5,ymm5,ymm8


vpslldq     xmm8,xmm4,14
vperm2f128  ymm8,ymm8,ymm8,1
vandpd      ymm8,ymm8,ymm15
vpsrldq     xmm10,xmm4,2
vperm2f128  ymm13,ymm4,ymm4,1
vpslldq     xmm12,xmm13,14
vorpd       xmm10,xmm10,xmm12
vpsrldq     xmm11,xmm13,2
vinsertf128 ymm10,ymm10,xmm11,1 
vandpd      ymm11,ymm10,ymm14
vorpd       ymm9,ymm9,ymm11
vandpd      ymm10,ymm10,[alg2_mask3]
vorpd       ymm9,ymm9,ymm10
vpsrlw      xmm10,xmm4,1
vpsrlw      xmm11,xmm13,1
vinsertf128 ymm10,ymm10,xmm11,1 
vandpd      ymm11,ymm10,[alg2_mask4]
vorpd       ymm9,ymm9,ymm11
vpsllw      xmm4,xmm4,1
vpsllw      xmm10,xmm13,1
vinsertf128 ymm4,ymm4,xmm10,1 
vandpd      ymm4,ymm4,[alg2_mask5]
vorpd       ymm4,ymm4,ymm9


vpslldq     xmm9,xmm3,14
vperm2f128  ymm9,ymm9,ymm9,1
vandpd      ymm9,ymm9,ymm15
vpsrldq     xmm10,xmm3,2
vperm2f128  ymm13,ymm3,ymm3,1
vpslldq     xmm12,xmm13,14
vorpd       xmm10,xmm10,xmm12
vpsrldq     xmm11,xmm13,2
vinsertf128 ymm10,ymm10,xmm11,1 
vandpd      ymm11,ymm10,ymm14
vorpd       ymm8,ymm8,ymm11
vandpd      ymm10,ymm10,[alg2_mask3]
vorpd       ymm8,ymm8,ymm10
vpsrlw      xmm10,xmm3,1
vpsrlw      xmm11,xmm13,1
vinsertf128 ymm10,ymm10,xmm11,1 
vandpd      ymm11,ymm10,[alg2_mask4]
vorpd       ymm8,ymm8,ymm11
vpsllw      xmm3,xmm3,1
vpsllw      xmm10,xmm13,1
vinsertf128 ymm3,ymm3,xmm10,1 
vandpd      ymm3,ymm3,[alg2_mask5]
vorpd       ymm3,ymm3,ymm8


vpslldq     xmm8,xmm2,14
vperm2f128  ymm8,ymm8,ymm8,1
vandpd      ymm8,ymm8,ymm15
vpsrldq     xmm10,xmm2,2
vperm2f128  ymm13,ymm2,ymm2,1
vpslldq     xmm12,xmm13,14
vorpd       xmm10,xmm10,xmm12
vpsrldq     xmm11,xmm13,2
vinsertf128 ymm10,ymm10,xmm11,1 
vandpd      ymm11,ymm10,ymm14
vorpd       ymm9,ymm9,ymm11
vandpd      ymm10,ymm10,[alg2_mask3]
vorpd       ymm9,ymm9,ymm10
vpsrlw      xmm10,xmm2,1
vpsrlw      xmm11,xmm13,1
vinsertf128 ymm10,ymm10,xmm11,1 
vandpd      ymm11,ymm10,[alg2_mask4]
vorpd       ymm9,ymm9,ymm11
vpsllw      xmm2,xmm2,1
vpsllw      xmm10,xmm13,1
vinsertf128 ymm2,ymm2,xmm10,1 
vandpd      ymm2,ymm2,[alg2_mask5]
vorpd       ymm2,ymm2,ymm9


vpslldq     xmm9,xmm1,14
vperm2f128  ymm9,ymm9,ymm9,1
vandpd      ymm9,ymm9,ymm15
vpsrldq     xmm10,xmm1,2
vperm2f128  ymm13,ymm1,ymm1,1
vpslldq     xmm12,xmm13,14
vorpd       xmm10,xmm10,xmm12
vpsrldq     xmm11,xmm13,2
vinsertf128 ymm10,ymm10,xmm11,1 
vandpd      ymm11,ymm10,ymm14
vorpd       ymm8,ymm8,ymm11
vandpd      ymm10,ymm10,[alg2_mask3]
vorpd       ymm8,ymm8,ymm10
vpsrlw      xmm10,xmm1,1
vpsrlw      xmm11,xmm13,1
vinsertf128 ymm10,ymm10,xmm11,1 
vandpd      ymm11,ymm10,[alg2_mask4]
vorpd       ymm8,ymm8,ymm11
vpsllw      xmm1,xmm1,1
vpsllw      xmm10,xmm13,1
vinsertf128 ymm1,ymm1,xmm10,1 
vandpd      ymm1,ymm1,[alg2_mask5]
vorpd       ymm1,ymm1,ymm8

vpsrldq     xmm10,xmm0,2
vperm2f128  ymm13,ymm0,ymm0,1
vpslldq     xmm12,xmm13,14
vorpd       xmm10,xmm10,xmm12
vpsrldq     xmm11,xmm13,2
vinsertf128 ymm10,ymm10,xmm11,1 
vandpd      ymm11,ymm10,ymm14
vorpd       ymm9,ymm9,ymm11
vandpd      ymm10,ymm10,[alg2_mask3]
vorpd       ymm9,ymm9,ymm10
vpsrlw      xmm10,xmm0,1
vpsrlw      xmm11,xmm13,1
vinsertf128 ymm10,ymm10,xmm11,1 
vandpd      ymm11,ymm10,[alg2_mask4]
vorpd       ymm9,ymm9,ymm11
vpsllw      xmm0,xmm0,1
vpsllw      xmm10,xmm13,1
vinsertf128 ymm0,ymm0,xmm10,1 
vandpd      ymm0,ymm0,[alg2_mask5]
vorpd       ymm0,ymm0,ymm9


movzx         r10,word  [r8]

; Fetch next seed value
movzx         r10,word  [rax+r10*2]
mov           [r8],r10


; return working code from ymm0-ymm7
vmovdqa     [rcx],ymm0
vmovdqa     [rcx+32],ymm1
vmovdqa     [rcx+64],ymm2
vmovdqa     [rcx+96],ymm3
vmovdqa     [rcx+128],ymm4
vmovdqa     [rcx+160],ymm5
vmovdqa     [rcx+192],ymm6
vmovdqa     [rcx+224],ymm7

vzeroupper

ret

alg3:
; fetch working code from memory, load into ymm0-ymm7
vmovdqa     ymm0,[rcx]
vmovdqa     ymm1,[rcx+32]
vmovdqa     ymm2,[rcx+64]
vmovdqa     ymm3,[rcx+96]
vmovdqa     ymm4,[rcx+128]
vmovdqa     ymm5,[rcx+160]
vmovdqa     ymm6,[rcx+192]
vmovdqa     ymm7,[rcx+224]

; DO WORK

; Get rng seed 
movzx       r10,word  [r8]

; Compute rng start
shl         r10,8
add         r10,rdx

; Do alg3
vxorpd      ymm0,ymm0,[r10]
vxorpd      ymm1,ymm1,[r10+32]
vxorpd      ymm2,ymm2,[r10+64]
vxorpd      ymm3,ymm3,[r10+96]
vxorpd      ymm4,ymm4,[r10+128]
vxorpd      ymm5,ymm5,[r10+160]
vxorpd      ymm6,ymm6,[r10+192]
vxorpd      ymm7,ymm7,[r10+224]


movzx         r10,word  [r8]

; Fetch next seed value
movzx         r10,word  [rax+r10*2]
mov           [r8],r10

; return working code from ymm0-ymm7
vmovdqa     [rcx],ymm0
vmovdqa     [rcx+32],ymm1
vmovdqa     [rcx+64],ymm2
vmovdqa     [rcx+96],ymm3
vmovdqa     [rcx+128],ymm4
vmovdqa     [rcx+160],ymm5
vmovdqa     [rcx+192],ymm6
vmovdqa     [rcx+224],ymm7

vzeroupper

ret


alg4:
; fetch working code from memory, load into ymm0-ymm7
vmovdqa     ymm0,[rcx]
vmovdqa     ymm1,[rcx+32]
vmovdqa     ymm2,[rcx+64]
vmovdqa     ymm3,[rcx+96]
vmovdqa     ymm4,[rcx+128]
vmovdqa     ymm5,[rcx+160]
vmovdqa     ymm6,[rcx+192]
vmovdqa     ymm7,[rcx+224]

; DO WORK

; Get rng seed 
movzx       r10,word  [r8]

; Compute rng start
shl         r10,8
add         r10,rdx

; Load ff_mask
vmovdqa     ymm11,[ff_mask]
; Load one mask
vmovdqa     ymm12,[one_mask]


; Load rng value
vmovdqa     ymm8,[r10]

; Invert it
vxorpd      ymm8,ymm8,ymm11

; Do 128 bit add on low bits
vpaddw      xmm9,xmm0,xmm8

; Add 1 
vpaddw      xmm9,xmm9,xmm12

; Swap the 128 bit lanes
vperm2f128  ymm0,ymm0,ymm0,1
vperm2f128  ymm8,ymm8,ymm8,1

; Do 128 bit add on low bits
vpaddw      xmm10,xmm0,xmm8

; Add 1
vpaddw      xmm10,xmm10,xmm12

; Reassemble
vinsertf128 ymm0,ymm9,xmm10,1 



; Repeat for the rest
vmovdqa     ymm8,[r10+32]
vxorpd      ymm8,ymm8,ymm11
vpaddw      xmm9,xmm1,xmm8
vpaddw      xmm9,xmm9,xmm12
vperm2f128  ymm1,ymm1,ymm1,1
vperm2f128  ymm8,ymm8,ymm8,1
vpaddw      xmm10,xmm1,xmm8
vpaddw      xmm10,xmm10,xmm12
vinsertf128 ymm1,ymm9,xmm10,1 

vmovdqa     ymm8,[r10+64]
vxorpd      ymm8,ymm8,ymm11
vpaddw      xmm9,xmm2,xmm8
vpaddw      xmm9,xmm9,xmm12
vperm2f128  ymm2,ymm2,ymm2,1
vperm2f128  ymm8,ymm8,ymm8,1
vpaddw      xmm10,xmm2,xmm8
vpaddw      xmm10,xmm10,xmm12
vinsertf128 ymm2,ymm9,xmm10,1 

vmovdqa     ymm8,[r10+96]
vxorpd      ymm8,ymm8,ymm11
vpaddw      xmm9,xmm3,xmm8
vpaddw      xmm9,xmm9,xmm12
vperm2f128  ymm3,ymm3,ymm3,1
vperm2f128  ymm8,ymm8,ymm8,1
vpaddw      xmm10,xmm3,xmm8
vpaddw      xmm10,xmm10,xmm12
vinsertf128 ymm3,ymm9,xmm10,1 

vmovdqa     ymm8,[r10+128]
vxorpd      ymm8,ymm8,ymm11
vpaddw      xmm9,xmm4,xmm8
vpaddw      xmm9,xmm9,xmm12
vperm2f128  ymm4,ymm4,ymm4,1
vperm2f128  ymm8,ymm8,ymm8,1
vpaddw      xmm10,xmm4,xmm8
vpaddw      xmm10,xmm10,xmm12
vinsertf128 ymm4,ymm9,xmm10,1 

vmovdqa     ymm8,[r10+160]
vxorpd      ymm8,ymm8,ymm11
vpaddw      xmm9,xmm5,xmm8
vpaddw      xmm9,xmm9,xmm12
vperm2f128  ymm5,ymm5,ymm5,1
vperm2f128  ymm8,ymm8,ymm8,1
vpaddw      xmm10,xmm5,xmm8
vpaddw      xmm10,xmm10,xmm12
vinsertf128 ymm5,ymm9,xmm10,1 

vmovdqa     ymm8,[r10+192]
vxorpd      ymm8,ymm8,ymm11
vpaddw      xmm9,xmm6,xmm8
vpaddw      xmm9,xmm9,xmm12
vperm2f128  ymm6,ymm6,ymm6,1
vperm2f128  ymm8,ymm8,ymm8,1
vpaddw      xmm10,xmm6,xmm8
vpaddw      xmm10,xmm10,xmm12
vinsertf128 ymm6,ymm9,xmm10,1 

vmovdqa     ymm8,[r10+224]
vxorpd      ymm8,ymm8,ymm11
vpaddw      xmm9,xmm7,xmm8
vpaddw      xmm9,xmm9,xmm12
vperm2f128  ymm7,ymm7,ymm7,1
vperm2f128  ymm8,ymm8,ymm8,1
vpaddw      xmm10,xmm7,xmm8
vpaddw      xmm10,xmm10,xmm12
vinsertf128 ymm7,ymm9,xmm10,1 






vandpd      ymm0,ymm0,ymm11
vandpd      ymm1,ymm1,ymm11
vandpd      ymm2,ymm2,ymm11
vandpd      ymm3,ymm3,ymm11
vandpd      ymm4,ymm4,ymm11
vandpd      ymm5,ymm5,ymm11
vandpd      ymm6,ymm6,ymm11
vandpd      ymm7,ymm7,ymm11


movzx         r10,word  [r8]

; Fetch next seed value
movzx         r10,word  [rax+r10*2]
mov           [r8],r10

; return working code from ymm0-ymm7
vmovdqa     [rcx],ymm0
vmovdqa     [rcx+32],ymm1
vmovdqa     [rcx+64],ymm2
vmovdqa     [rcx+96],ymm3
vmovdqa     [rcx+128],ymm4
vmovdqa     [rcx+160],ymm5
vmovdqa     [rcx+192],ymm6
vmovdqa     [rcx+224],ymm7

vzeroupper

ret


alg5:
; fetch working code from memory, load into ymm0-ymm7
vmovdqa     ymm0,[rcx]
vmovdqa     ymm1,[rcx+32]
vmovdqa     ymm2,[rcx+64]
vmovdqa     ymm3,[rcx+96]
vmovdqa     ymm4,[rcx+128]
vmovdqa     ymm5,[rcx+160]
vmovdqa     ymm6,[rcx+192]
vmovdqa     ymm7,[rcx+224]

; Get rng seed 
movzx       r10,word  [r8]

; Compute rng start
shl         r10,5
add         r10,rdx

; Load rng value
;;; should i use a set value? is it faster to clear and then load a single?
vmovdqa     ymm8,[r10]
;carry = ymm8

; Get a top bit mask
vmovdqa     ymm15,[alg5_mask1]
vmovdqa     ymm14,[alg5_mask2]

; Get next carry
; Shift current value
vpslldq     xmm9,xmm7,14
; Swap lanes
vperm2f128  ymm9,ymm9,ymm9,1
; Mask to get the carry bit
vandpd      ymm9,ymm9,ymm15
; next carry = ymm9

; Shift current value down 1 byte cell
vpsrldq     xmm10,xmm7,2
; Get high
vperm2f128  ymm13,ymm7,ymm7,1
; get carry across lanes
vpslldq     xmm12,xmm13,14
; OR it with the low bits
vorpd       xmm10,xmm10,xmm12
; Shift high bits
vpsrldq     xmm11,xmm13,2
; Reassemble into a 256
vinsertf128 ymm10,ymm10,xmm11,1 
; cur_val srl = ymm10

; Get extra carry
vandpd      ymm11,ymm10,ymm14
; or with carry
vorpd       ymm8,ymm8,ymm11

; Mask srl value
vandpd      ymm10,ymm10,[alg5_mask3]
; combine with carry
vorpd       ymm8,ymm8,ymm10

; Make part 1
;lo
vpsrlw      xmm10,xmm7,1
;high
vpsrlw      xmm11,xmm13,1
vinsertf128 ymm10,ymm10,xmm11,1 
vandpd      ymm11,ymm10,[alg5_mask4]
; or with carry
vorpd       ymm8,ymm8,ymm11

; make part 2
;lo
vpsllw      xmm7,xmm7,1
;high
vpsllw      xmm10,xmm13,1
vinsertf128 ymm7,ymm7,xmm10,1 
vandpd      ymm7,ymm7,[alg5_mask5]
; or in carry
vorpd       ymm7,ymm7,ymm8

; carry/next carry now swapped



vpslldq     xmm8,xmm6,14
vperm2f128  ymm8,ymm8,ymm8,1
vandpd      ymm8,ymm8,ymm15
vpsrldq     xmm10,xmm6,2
vperm2f128  ymm13,ymm6,ymm6,1
vpslldq     xmm12,xmm13,14
vorpd       xmm10,xmm10,xmm12
vpsrldq     xmm11,xmm13,2
vinsertf128 ymm10,ymm10,xmm11,1 
vandpd      ymm11,ymm10,ymm14
vorpd       ymm9,ymm9,ymm11
vandpd      ymm10,ymm10,[alg5_mask3]
vorpd       ymm9,ymm9,ymm10
vpsrlw      xmm10,xmm6,1
vpsrlw      xmm11,xmm13,1
vinsertf128 ymm10,ymm10,xmm11,1 
vandpd      ymm11,ymm10,[alg5_mask4]
vorpd       ymm9,ymm9,ymm11
vpsllw      xmm6,xmm6,1
vpsllw      xmm10,xmm13,1
vinsertf128 ymm6,ymm6,xmm10,1 
vandpd      ymm6,ymm6,[alg5_mask5]
vorpd       ymm6,ymm6,ymm9


vpslldq     xmm9,xmm5,14
vperm2f128  ymm9,ymm9,ymm9,1
vandpd      ymm9,ymm9,ymm15
vpsrldq     xmm10,xmm5,2
vperm2f128  ymm13,ymm5,ymm5,1
vpslldq     xmm12,xmm13,14
vorpd       xmm10,xmm10,xmm12
vpsrldq     xmm11,xmm13,2
vinsertf128 ymm10,ymm10,xmm11,1 
vandpd      ymm11,ymm10,ymm14
vorpd       ymm8,ymm8,ymm11
vandpd      ymm10,ymm10,[alg5_mask3]
vorpd       ymm8,ymm8,ymm10
vpsrlw      xmm10,xmm5,1
vpsrlw      xmm11,xmm13,1
vinsertf128 ymm10,ymm10,xmm11,1 
vandpd      ymm11,ymm10,[alg5_mask4]
vorpd       ymm8,ymm8,ymm11
vpsllw      xmm5,xmm5,1
vpsllw      xmm10,xmm13,1
vinsertf128 ymm5,ymm5,xmm10,1 
vandpd      ymm5,ymm5,[alg5_mask5]
vorpd       ymm5,ymm5,ymm8


vpslldq     xmm8,xmm4,14
vperm2f128  ymm8,ymm8,ymm8,1
vandpd      ymm8,ymm8,ymm15
vpsrldq     xmm10,xmm4,2
vperm2f128  ymm13,ymm4,ymm4,1
vpslldq     xmm12,xmm13,14
vorpd       xmm10,xmm10,xmm12
vpsrldq     xmm11,xmm13,2
vinsertf128 ymm10,ymm10,xmm11,1 
vandpd      ymm11,ymm10,ymm14
vorpd       ymm9,ymm9,ymm11
vandpd      ymm10,ymm10,[alg5_mask3]
vorpd       ymm9,ymm9,ymm10
vpsrlw      xmm10,xmm4,1
vpsrlw      xmm11,xmm13,1
vinsertf128 ymm10,ymm10,xmm11,1 
vandpd      ymm11,ymm10,[alg5_mask4]
vorpd       ymm9,ymm9,ymm11
vpsllw      xmm4,xmm4,1
vpsllw      xmm10,xmm13,1
vinsertf128 ymm4,ymm4,xmm10,1 
vandpd      ymm4,ymm4,[alg5_mask5]
vorpd       ymm4,ymm4,ymm9


vpslldq     xmm9,xmm3,14
vperm2f128  ymm9,ymm9,ymm9,1
vandpd      ymm9,ymm9,ymm15
vpsrldq     xmm10,xmm3,2
vperm2f128  ymm13,ymm3,ymm3,1
vpslldq     xmm12,xmm13,14
vorpd       xmm10,xmm10,xmm12
vpsrldq     xmm11,xmm13,2
vinsertf128 ymm10,ymm10,xmm11,1 
vandpd      ymm11,ymm10,ymm14
vorpd       ymm8,ymm8,ymm11
vandpd      ymm10,ymm10,[alg5_mask3]
vorpd       ymm8,ymm8,ymm10
vpsrlw      xmm10,xmm3,1
vpsrlw      xmm11,xmm13,1
vinsertf128 ymm10,ymm10,xmm11,1 
vandpd      ymm11,ymm10,[alg5_mask4]
vorpd       ymm8,ymm8,ymm11
vpsllw      xmm3,xmm3,1
vpsllw      xmm10,xmm13,1
vinsertf128 ymm3,ymm3,xmm10,1 
vandpd      ymm3,ymm3,[alg5_mask5]
vorpd       ymm3,ymm3,ymm8


vpslldq     xmm8,xmm2,14
vperm2f128  ymm8,ymm8,ymm8,1
vandpd      ymm8,ymm8,ymm15
vpsrldq     xmm10,xmm2,2
vperm2f128  ymm13,ymm2,ymm2,1
vpslldq     xmm12,xmm13,14
vorpd       xmm10,xmm10,xmm12
vpsrldq     xmm11,xmm13,2
vinsertf128 ymm10,ymm10,xmm11,1 
vandpd      ymm11,ymm10,ymm14
vorpd       ymm9,ymm9,ymm11
vandpd      ymm10,ymm10,[alg5_mask3]
vorpd       ymm9,ymm9,ymm10
vpsrlw      xmm10,xmm2,1
vpsrlw      xmm11,xmm13,1
vinsertf128 ymm10,ymm10,xmm11,1 
vandpd      ymm11,ymm10,[alg5_mask4]
vorpd       ymm9,ymm9,ymm11
vpsllw      xmm2,xmm2,1
vpsllw      xmm10,xmm13,1
vinsertf128 ymm2,ymm2,xmm10,1 
vandpd      ymm2,ymm2,[alg5_mask5]
vorpd       ymm2,ymm2,ymm9


vpslldq     xmm9,xmm1,14
vperm2f128  ymm9,ymm9,ymm9,1
vandpd      ymm9,ymm9,ymm15
vpsrldq     xmm10,xmm1,2
vperm2f128  ymm13,ymm1,ymm1,1
vpslldq     xmm12,xmm13,14
vorpd       xmm10,xmm10,xmm12
vpsrldq     xmm11,xmm13,2
vinsertf128 ymm10,ymm10,xmm11,1 
vandpd      ymm11,ymm10,ymm14
vorpd       ymm8,ymm8,ymm11
vandpd      ymm10,ymm10,[alg5_mask3]
vorpd       ymm8,ymm8,ymm10
vpsrlw      xmm10,xmm1,1
vpsrlw      xmm11,xmm13,1
vinsertf128 ymm10,ymm10,xmm11,1 
vandpd      ymm11,ymm10,[alg5_mask4]
vorpd       ymm8,ymm8,ymm11
vpsllw      xmm1,xmm1,1
vpsllw      xmm10,xmm13,1
vinsertf128 ymm1,ymm1,xmm10,1 
vandpd      ymm1,ymm1,[alg5_mask5]
vorpd       ymm1,ymm1,ymm8

vpsrldq     xmm10,xmm0,2
vperm2f128  ymm13,ymm0,ymm0,1
vpslldq     xmm12,xmm13,14
vorpd       xmm10,xmm10,xmm12
vpsrldq     xmm11,xmm13,2
vinsertf128 ymm10,ymm10,xmm11,1 
vandpd      ymm11,ymm10,ymm14
vorpd       ymm9,ymm9,ymm11
vandpd      ymm10,ymm10,[alg5_mask3]
vorpd       ymm9,ymm9,ymm10
vpsrlw      xmm10,xmm0,1
vpsrlw      xmm11,xmm13,1
vinsertf128 ymm10,ymm10,xmm11,1 
vandpd      ymm11,ymm10,[alg5_mask4]
vorpd       ymm9,ymm9,ymm11
vpsllw      xmm0,xmm0,1
vpsllw      xmm10,xmm13,1
vinsertf128 ymm0,ymm0,xmm10,1 
vandpd      ymm0,ymm0,[alg5_mask5]
vorpd       ymm0,ymm0,ymm9


movzx         r10,word  [r8]

; Fetch next seed value
movzx         r10,word  [rax+r10*2]
mov           [r8],r10


; return working code from ymm0-ymm7
vmovdqa     [rcx],ymm0
vmovdqa     [rcx+32],ymm1
vmovdqa     [rcx+64],ymm2
vmovdqa     [rcx+96],ymm3
vmovdqa     [rcx+128],ymm4
vmovdqa     [rcx+160],ymm5
vmovdqa     [rcx+192],ymm6
vmovdqa     [rcx+224],ymm7

vzeroupper

ret



alg6:
; fetch working code from memory, load into ymm0-ymm7
vmovdqa     ymm0,[rcx]
vmovdqa     ymm1,[rcx+32]
vmovdqa     ymm2,[rcx+64]
vmovdqa     ymm3,[rcx+96]
vmovdqa     ymm4,[rcx+128]
vmovdqa     ymm5,[rcx+160]
vmovdqa     ymm6,[rcx+192]
vmovdqa     ymm7,[rcx+224]

; DO WORK

; Get rng seed 
movzx       r10,word  [r8]

; Compute rng start
shl         r10,8
add         r10,rdx

; Load ff_mask
vmovdqa     ymm11,[ff_mask]

; Load rng value
vmovdqa     ymm8,[r10]

; Get high bits of ymm0
vperm2f128  ymm9,ymm0,ymm0,1

; Do 128 bit shift on high bits
vpsrlw      xmm9,1

; Do 128 bit shift on low bits
vpsrlw      xmm0,1

; Reinsert high bits
vinsertf128 ymm0,ymm0,xmm9,1
 
; OR in the rng value
vorpd       ymm0,ymm0,ymm8

; Mask it
vandpd      ymm0,ymm0,ymm11



; Repeat for the rest
vmovdqa     ymm8,[r10+32]
vperm2f128  ymm9,ymm1,ymm1,1
vpsrlw      xmm9,1
vpsrlw      xmm1,1
vinsertf128 ymm1,ymm1,xmm9,1
vorpd       ymm1,ymm1,ymm8
vandpd      ymm1,ymm1,ymm11

vmovdqa     ymm8,[r10++64]
vperm2f128  ymm9,ymm2,ymm2,1
vpsrlw      xmm9,1
vpsrlw      xmm2,1
vinsertf128 ymm2,ymm2,xmm9,1
vorpd       ymm2,ymm2,ymm8
vandpd      ymm2,ymm2,ymm11

vmovdqa     ymm8,[r10+96]
vperm2f128  ymm9,ymm3,ymm3,1
vpsrlw      xmm9,1
vpsrlw      xmm3,1
vinsertf128 ymm3,ymm3,xmm9,1
vorpd       ymm3,ymm3,ymm8
vandpd      ymm3,ymm3,ymm11

vmovdqa     ymm8,[r10+128]
vperm2f128  ymm9,ymm4,ymm4,1
vpsrlw      xmm9,1
vpsrlw      xmm4,1
vinsertf128 ymm4,ymm4,xmm9,1
vorpd       ymm4,ymm4,ymm8
vandpd      ymm4,ymm4,ymm11

vmovdqa     ymm8,[r10+160]
vperm2f128  ymm9,ymm5,ymm5,1
vpsrlw      xmm9,1
vpsrlw      xmm5,1
vinsertf128 ymm5,ymm5,xmm9,1
vorpd       ymm5,ymm5,ymm8
vandpd      ymm5,ymm5,ymm11

vmovdqa     ymm8,[r10+192]
vperm2f128  ymm9,ymm6,ymm6,1
vpsrlw      xmm9,1
vpsrlw      xmm6,1
vinsertf128 ymm6,ymm6,xmm9,1
vorpd       ymm6,ymm6,ymm8
vandpd      ymm6,ymm6,ymm11

vmovdqa     ymm8,[r10+224]
vperm2f128  ymm9,ymm7,ymm7,1
vpsrlw      xmm9,1
vpsrlw      xmm7,1
vinsertf128 ymm7,ymm7,xmm9,1
vorpd       ymm7,ymm7,ymm8
vandpd      ymm7,ymm7,ymm11


movzx         r10,word  [r8]

; Fetch next seed value
movzx         r10,word  [rax+r10*2]
mov           [r8],r10

; return working code from ymm0-ymm7
vmovdqa     [rcx],ymm0
vmovdqa     [rcx+32],ymm1
vmovdqa     [rcx+64],ymm2
vmovdqa     [rcx+96],ymm3
vmovdqa     [rcx+128],ymm4
vmovdqa     [rcx+160],ymm5
vmovdqa     [rcx+192],ymm6
vmovdqa     [rcx+224],ymm7

vzeroupper

ret



alg7:
; fetch working code from memory, load into ymm0-ymm7
vmovdqa     ymm0,[rcx]
vmovdqa     ymm1,[rcx+32]
vmovdqa     ymm2,[rcx+64]
vmovdqa     ymm3,[rcx+96]
vmovdqa     ymm4,[rcx+128]
vmovdqa     ymm5,[rcx+160]
vmovdqa     ymm6,[rcx+192]
vmovdqa     ymm7,[rcx+224]

; DO WORK

; Load ff_mask
vmovdqa     ymm11,[ff_mask]

vxorpd      ymm0,ymm0,ymm11
vxorpd      ymm1,ymm1,ymm11
vxorpd      ymm2,ymm2,ymm11
vxorpd      ymm3,ymm3,ymm11
vxorpd      ymm4,ymm4,ymm11
vxorpd      ymm5,ymm5,ymm11
vxorpd      ymm6,ymm6,ymm11
vxorpd      ymm7,ymm7,ymm11

; return working code from ymm0-ymm7
vmovdqa     [rcx],ymm0
vmovdqa     [rcx+32],ymm1
vmovdqa     [rcx+64],ymm2
vmovdqa     [rcx+96],ymm3
vmovdqa     [rcx+128],ymm4
vmovdqa     [rcx+160],ymm5
vmovdqa     [rcx+192],ymm6
vmovdqa     [rcx+224],ymm7

vzeroupper

ret

section .data
ff_mask:
align 32
dd 0x00FF00FF
dd 0x00FF00FF
dd 0x00FF00FF
dd 0x00FF00FF
dd 0x00FF00FF
dd 0x00FF00FF
dd 0x00FF00FF
dd 0x00FF00FF

one_mask:
align 32
dd 0x00010001
dd 0x00010001
dd 0x00010001
dd 0x00010001
dd 0x00010001
dd 0x00010001
dd 0x00010001
dd 0x00010001

alg2_mask1:
align 32
dd 0x00000000
dd 0x00000000
dd 0x00000000
dd 0x00000000
dd 0x00000000
dd 0x00000000
dd 0x00000000
dd 0x00010000

alg2_mask2:
align 32
dd 0x00010000
dd 0x00010000
dd 0x00010000
dd 0x00010000
dd 0x00010000
dd 0x00010000
dd 0x00010000
dd 0x00000000

alg2_mask3:
align 32
dd 0x00000080
dd 0x00000080
dd 0x00000080
dd 0x00000080
dd 0x00000080
dd 0x00000080
dd 0x00000080
dd 0x00000080

alg2_mask4:
align 32
dd 0x0000007F
dd 0x0000007F
dd 0x0000007F
dd 0x0000007F
dd 0x0000007F
dd 0x0000007F
dd 0x0000007F
dd 0x0000007F

alg2_mask5:
align 32
dd 0x00FE0000
dd 0x00FE0000
dd 0x00FE0000
dd 0x00FE0000
dd 0x00FE0000
dd 0x00FE0000
dd 0x00FE0000
dd 0x00FE0000

alg5_mask1:
align 32
dd 0x00000000
dd 0x00000000
dd 0x00000000
dd 0x00000000
dd 0x00000000
dd 0x00000000
dd 0x00000000
dd 0x00800000

alg5_mask2:
align 32
dd 0x00800000
dd 0x00800000
dd 0x00800000
dd 0x00800000
dd 0x00800000
dd 0x00800000
dd 0x00800000
dd 0x00000000

alg5_mask3:
align 32
dd 0x00000001
dd 0x00000001
dd 0x00000001
dd 0x00000001
dd 0x00000001
dd 0x00000001
dd 0x00000001
dd 0x00000001

alg5_mask4:
align 32
dd 0x007F0000
dd 0x007F0000
dd 0x007F0000
dd 0x007F0000
dd 0x007F0000
dd 0x007F0000
dd 0x007F0000
dd 0x007F0000

alg5_mask5:
align 32
dd 0x000000FE
dd 0x000000FE
dd 0x000000FE
dd 0x000000FE
dd 0x000000FE
dd 0x000000FE
dd 0x000000FE
dd 0x000000FE
