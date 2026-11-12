; IM2 interrupt setup: vector table at 0xFD00 (257 x 0xFE), jump at 0xFEFE.
; The stack lives below 0xFD00 (REGISTER_SP), code must end below that.

        SECTION code_user

        PUBLIC  _irq_init
        PUBLIC  _wait_frame
        PUBLIC  _frames

        defc    VECTOR_TABLE = 0xFD00
        defc    JUMP_ADDR    = 0xFEFE

_irq_init:
        di
        ld      hl,VECTOR_TABLE
        ld      de,VECTOR_TABLE+1
        ld      bc,256
        ld      (hl),0xFE
        ldir
        ld      a,0xC3                  ; JP isr
        ld      (JUMP_ADDR),a
        ld      hl,isr
        ld      (JUMP_ADDR+1),hl
        ld      a,VECTOR_TABLE >> 8
        ld      i,a
        im      2
        ei
        ret

isr:
        push    af
        push    hl
        ld      hl,(_frames)
        inc     hl
        ld      (_frames),hl
        pop     hl
        pop     af
        ei
        reti

; Wait until the next frame interrupt has happened.
_wait_frame:
        ld      a,(_frames)
wf_loop:
        ld      hl,_frames
        cp      (hl)
        jr      z,wf_loop
        ret

        SECTION data_user
_frames:
        defw    0
