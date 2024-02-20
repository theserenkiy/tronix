;throttle_backemf.asm

.NOLIST
;  ***************************************************************************************
;  * PWM MODEL RAILROAD THROTTLE                                                          *
;  *                                                                                      *
;  * WRITTEN BY:  PHILIP DEVRIES                                                          *
;  *                                                                                      *
;  *  Copyright (C) 2003 Philip DeVries                                                   *
;  *                                                                                      *
;  *  This program is free software; you can redistribute it and/or modify                *
;  *  it under the terms of the GNU General Public License as published by                *
;  *  the Free Software Foundation; either version 2 of the License, or                   *
;  *  (at your option) any later version.                                                 *
;  *                                                                                      *
;  *  This program is distributed in the hope that it will be useful,                     *
;  *  but WITHOUT ANY WARRANTY; without even the implied warranty of                      *
;  *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                       *
;  *  GNU General Public License for more details.                                        *
;  *                                                                                      *
;  *  You should have received a copy of the GNU General Public License                   *
;  *  along with this program; if not, write to the Free Software                         *
;  *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA           *
;  *                                                                                      *
;  ***************************************************************************************
.LIST

.ifdef BACKEMF_ENABLED
;********************************************************************************
;* BACKEMF_ADJUST                                                                *
;* Top level routine                                                             *
;*                                                                               *
;* The throttle_set is compared against the back emf generated by the motor      *
;* and adjusted to reduce the error                                              *
;*                                                                               *
;* Inputs:     throttle_set         Target speed                                 *
;* Returns:    throttle_set         Adjusted target speed                        *
;* Changed:    error_hi             Adjusted throttle, upper 8 bits (local)      * 
;*             error_lo             Adjusted throttle, lower 8 bits (local)      *
;*             error_hi_prev        Previous throttle, for filter (global)       *
;*             error_lo_prev        Previous throttle, for filter (global)       *
;*             emf_hi               Measured emf, upper 8 bits (local)           *
;*             emf_lo               Measured emf, lower 8 bits (local)           *
;*             B_Temp,B_Temp1                                                    *
;* Calls:      ADC_SETUP_EMF                                                     *  
;*             div16u                                                            *
;*             DIVIDE_16_SIMPLE                                                  *
;* Goto:       none                                                              *
;********************************************************************************

   HILOCAL1    error_lo             ; assign local variables
   HILOCAL2    error_hi

   ;*****************************************************************   
   ;*Convert throttle setting into 2 byte 2's compl.                 *
   ;*                                                                *
   ;* This is a 7 bit number plus 1 more bits after the radix        *
   ;* It is in (error_hi) -radix- (error_lo)                         *
   ;*****************************************************************   
   mov   error_hi,throttle_set         ; Put throttle into 16 bit form  
   clr   error_lo

   lsr   error_hi                      ; Convert to 2's compliment
   ror   error_lo

   ;********************************************************************************
   ;* READ_BACKEMF                                                                  *
   ;* Returns a 2 byte 2's compliment measurement of the motor backemf.             *
   ;*                                                                               *
   ;* 1. Add together 8 samples of the (8 bit) pwm value in the two byte            *
   ;*    emf_hi--emf_lo register.                                                   *
   ;* 2. Multiply by 16.                                                            *
   ;* 3. Result:  Minimum value = 0x000 (decimal 0)                                 *
   ;*             Maximum value = 0x7F8 (decimal 2040)                              *
   ;*                                                                               *
   ;* Time required:                                                                *
   ;* 1. 1st Sample:             125uS                                              *
   ;* 2. next 7 Samples:         455uS                                              *
   ;* 3. balance of Subroutine:  10's of uS                                         *
   ;*    TOTAL                   580uS min                                          *
   ;*                                                                               *
   ;* Each cycle of the 25kHz PWM takes 40uS, therefore, this routine takes         *
   ;* at least 14.5 cycles of the 25kHz pwm.                                        *
   ;*                                                                               *
   ;* Inputs:  None                                                                 *
   ;* Returns: emf_hi--emf_lo:      2 Byte 2's compl (but always positive)          *
   ;*                               measure of motor backemf.                       *
   ;* Changed: B_Temp,B_Temp1                                                       *
   ;* Calls:   ADC_SETUP_EMF                                                        *
   ;********************************************************************************
LOWLOCAL1   emf_hi                     ; Names of local registers
LOWLOCAL2   emf_lo

;READ_BACKEMF:
   rcall ADC_SETUP_EMF                 ; Setup ADC to measure back_emf.
   clr   emf_lo                        ; Clear the value of emf.
   clr   emf_hi

   ldi   B_Temp,8                      ; Add 8 samples
WAIT_FOR_EMF_MEASURE:                  ; Wait for a measurement of the EMF
   sbis  ADCSR,ADIF
   rjmp  WAIT_FOR_EMF_MEASURE
   
   in    B_Temp1,ADCH                  ; Read the measurement

   sbi   ADCSR,4                       ; Clear the interrupt

   add   emf_lo,B_Temp1                ; Add to low byte (no carry)

   clr   B_Temp1
   adc   emf_hi,B_Temp1                ; Add carry to high byte.

   dec   B_Temp
   brne  WAIT_FOR_EMF_MEASURE          ; Measure for complete set
                                       ; Sum of 8 samples.

   ldi   B_Temp,4                      ; Convert 11 bit number into a 15 bit
COMPUTE_EMF_AVERAGE:                   ; number (only 11 significant figures
   lsl   emf_lo                        ; though)
   rol   emf_hi
   dec   B_Temp
   brne  COMPUTE_EMF_AVERAGE

   ;*****************************************************************   
   ;* Compute the error.  That is, throttle = throttle - emf         *
   ;*                                                                *  
   ;* The result is a two byte number (signed two's compl) in        *
   ;* error_hi -radix- error_lo                                *
   ;*****************************************************************   
   sub   error_lo,emf_lo               ; subtract low bytes (after radix)
   sbc   error_hi,emf_hi               ; subtract high bytes (before radix)

.ifdef   BACKEMF_SCALE_ENABLED
   ;*****************************************************************   
   ;* Error multiplier (complex)                                     *
   ;*                                                                *
   ;* Error gain is equal to:                                        *
   ;*                                                                *
   ;* Error                       err_scale    err_mult              *
   ;* ------------------------ * 2          * 2                      *
   ;*  err_scale                                                     *
   ;* 2         + throttle_set                                       *
   ;*                                                                *
   ;* The maximum gain when throttle_set = 0 is 2^err_mult           *
   ;* is cut in half when throttle_set = 2^err_scale                 *
   ;*                                                                *
   ;* Result is signed two's compliment in                           *
   ;* error_hi--error_lo -radix-                                     *
   ;*****************************************************************   
   cbr   Flags_1,F_negative_err        ; Assume error is positive
      
   sbrs  error_hi,7                    ; Test algebraic sign
   rjmp  POSITIVE_ERR

   sbr   Flags_1,F_negative_err        ; If error is negative, set flag.
   com   error_lo                      ; Convert to positive
   com   error_hi
   subi  error_lo,0xFF
   sbci  error_hi,0xFF

POSITIVE_ERR:

   B_TEMPLOCAL    _bemf_lo_byte
   B_TEMPLOCAL1   _bemf_hi_byte

   mov   _bemf_lo_byte,throttle_set    ; Divisor = throttle_set+2^err_scale
   clr   _bemf_hi_byte 
   ldi   B_Temp2,EXP2(err_scale)
   add   _bemf_lo_byte,B_Temp2
   adc   _bemf_hi_byte,_bemf_hi_byte

;  mov   dd16uL,error_lo               ; Dividend = error (same register)
;  mov   dd16uH,error_hi

   rcall div16u                        ; Divide error by (throttle+offset)
                                       ; (almost 4 pwm cycles)
                                       ; adds 3 to Cycle_count

;  mov   error_lo,dres16uL             ; Same register
;  mov   error_hi,dres16uH

   sbrs  Flags_1,BF_negative_err       ; Check sign flag
   rjmp  POSITIVE_ERR_1

   com   error_lo                      ; Convert back to negative
   com   error_hi                      ; if necessary
   subi  error_lo,0xFF
   sbci  error_hi,0xFF

POSITIVE_ERR_1:                        ; Scale for maximum
   ldi   _bemf_lo_byte, 7 - error_mult - err_scale
   rcall DIVIDE_16_SIMPLE

.else    ;case BACKEMF_SCALE_ENABLED is NOT enabled
   ;*****************************************************************   
   ;* Error multiplier (simple)                                      *
   ;*                                                                *
   ;* The error multiplier setting (error_mult) can range            *
   ;* from -8 to +7, and the actual error multiplier is              *
   ;* 2^(error_mult), which therefore ranges from 1/256 to 128.      *
   ;*                                                                *
   ;* Step 1.  Multiply by 2^8.                                      *
   ;*          Equivalent to moving the radix point to after         *
   ;*          error_lo.  THIS STEP REQUIRES NO CODE                 * 
   ;*                                                                *
   ;* Step 2.  Divide by 2^(error_mult - 8)                          *
   ;*                                                                *
   ;* Result is signed two's compliment in                           *
   ;* error_hi--error_lo -radix-                                     *
   ;*****************************************************************   

   ldi   _bemf_lo_byte, 7 - error_mult        
   rcall DIVIDE_16_SIMPLE

.endif   ;BACKEMF_SCALE_ENABLED


COMPUTE_NEW_PWM:
   ;*****************************************************************   
   ;* Add in the original throttle                                   *
   ;*****************************************************************   
   add   error_lo,throttle_set
   clr   B_Temp
   adc   error_hi,B_Temp

   ;*****************************************************************
   ;* Clamp to between 0 and +255                                    *
   ;*****************************************************************   
   brmi  SET_ZERO_PWM                  ; If result is NEGATIVE, set to zero.

   cpi   error_hi,0x00                 ; If hi byte is zero, result is ok.
   breq  LOWPASS
   ldi   error_lo,0xFF                 ; otherwise, clamp
   rjmp  LOWPASS

SET_ZERO_PWM:
   clr   error_lo

LOWPASS:

.ifdef   LOWPASS_ENABLED
   ;*****************************************************************
   ;* A transversal low pass filter                                  *
   ;* Lowpass on the emf-adjusted pwm                                *
   ;*                                                                *
   ;* gain input "emf_lowpass_gain", range = 0 to 8                  *
   ;*                                                                *
   ;* The actual filter time constant "tau" is equal to              *
   ;* tau = 2^emf_lowpass_gain * sample_interval                     *
   ;*                                                                *
   ;* The sample interval is nominally 10mS, so the time             *
   ;* constant values are:                                           *
   ;* 0    1    2    3    4     5     6     7     8                  *
   ;* 10mS,20mS,40mS,80mS,160mS,320mS,640mS,1.28S,2.56S              *
   ;*                                                                *
   ;* The current sample is added to an attenuated sum of previous   *
   ;* samples as follows:                                            *
   ;*                                                                *
   ;* Adjusted Value = 1/(2^gain) x                                  *
   ;*                   (  1x sample number (i)                      *
   ;*                   +  gain x sample number (i-1)                *
   ;*                   +  gain^2 x sample number (i-2)              *
   ;*                   +  gain^3 x sample number (i-3)              *
   ;*                   + ....                                       *
   ;*                   )                                            *
   ;* Where:                                                         *
   ;* Gain values: gain =  (2^emf_lowpass_gain -  1) / 2^n           *
   ;*                      0,1/2,3/4,7/8,15/16 ... 255/256           *
   ;*                                                                *
   ;* Algorithm:                                                     *
   ;*                                                                *
   ;*    -Input (current sample) in error_lo (error_hi=0)            *
   ;*       0x00FF max                                               *
   ;*                                                                *
   ;*    -Input (scaled sum of previous samples) in                  *
   ;*       error_hi_prev--error_lo_prev.                            *
   ;*       0x00FF * (2^emf_lowpass_gain - 1 ) max                   *
   ;*                                                                *
   ;* 1. The error_hi_prev--error_lo_prev is added to                *
   ;*    error_hi--error_lo                                          *
   ;*       0x00FF * (2^emf_lowpass_gain) max                        *
   ;*                                                                *
   ;* 2. This value is also stored in                                *
   ;*    error_hi_prev--error_lo_prev                                *
   ;*                                                                *
   ;* 3. The value (error_hi--error_lo) is divided by                *
   ;*    2^emf_lowpass_gain (resulting in lowpass value,             *
   ;*    max 0x00FF)                                                 *
   ;*                                                                *
   ;* 4. The value (error_hi--error_lo) is subtracted from           *
   ;*    error_hi_prev--error_lo_prev.  This is the new              *
   ;*    stored value.                                               *     
   ;*****************************************************************   
   clr   error_hi 

   ;****
   ;* 1. Add in cumulative previous error
   ;****
   add   error_lo,error_lo_prev        ; Add in scaled previous samples
   adc   error_hi,error_hi_prev        ;

   ;****
   ;* 2. Store
   ;****
   mov   error_lo_prev,error_lo        ; Store new value
   mov   error_hi_prev,error_hi        ; Store new value

   ;****
   ;* 3. Divide new value
   ;****
   ldi   _bemf_lo_byte,emf_lowpass_gain 
   rcall DIVIDE_16_SIMPLE

   ;****
   ;* 4. New value in error_prev
   ;****
ADJUST_STORED:
   sub   error_lo_prev,error_lo
   sbc   error_hi_prev,error_hi

.endif ;LOWPASS_FILTER

   mov   throttle_set,error_lo
   subi  Cycle_count,256-15            ; Normal arrival here occurs after 3.5 + 14.5 +
                                       ; pwm cycles.  Add 15 counts here, also 3 added in
                                       ; div16u

.endif BACKEMF_ENABLED
