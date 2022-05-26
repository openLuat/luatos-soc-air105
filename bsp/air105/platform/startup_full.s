/*
 * Copyright (c) 2022 OpenLuat & AirM2M
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

  .syntax unified
  .cpu cortex-m4
  .fpu softvfp
  .thumb

.global  g_pfnVectors
.global HardFault_Handler
/* start address for the initialization values of the .data section. 
defined in linker script */
.word  _sidata
/* start address for the .data section. defined in linker script */  
.word  _sdata
/* end address for the .data section. defined in linker script */
.word  _edata
/* start address for the .bss section. defined in linker script */
.word  _sbss
/* end address for the .bss section. defined in linker script */
.word  _ebss
/* stack used for SystemInit_ExtMemCtl; always internal RAM used */

/**
 * @brief  This is the code that gets called when the processor first
 *          starts execution following a reset event. Only the absolutely
 *          necessary set is performed, after which the application
 *          supplied main() routine is called. 
 * @param  None
 * @retval : None
*/

    .section  .text.Reset_Handler
  .weak  Reset_Handler
  .type  Reset_Handler, %function
Reset_Handler:  
	movs r0, #0
	movs r1, #0
	movs r2, #0
	movs r3, #0
	movs r4, #0
	movs r5, #0
	movs r6, #0
	movs r7, #0

	LDR R0, =g_pfnVectors
	LDR R1, =0xE000ED08
	STR R0,[R1]
  ldr   sp, =_estack     /* set stack pointer */

/* Copy the data segment initializers from flash to SRAM */  
  movs  r1, #0
  b  LoopCopyDataInit

CopyDataInit:
  ldr  r3, =_sidata
  ldr  r3, [r3, r1]
  str  r3, [r0, r1]
  adds  r1, r1, #4
    
LoopCopyDataInit:
  ldr  r0, =_sdata
  ldr  r3, =_edata
  adds  r2, r0, r1
  cmp  r2, r3
  bcc  CopyDataInit
  ldr  r2, =_sbss
  b  LoopFillZerobss
/* Zero fill the bss segment. */  
FillZerobss:
  movs  r3, #0
  str  r3, [r2], #4
    
LoopFillZerobss:
  ldr  r3, = _ebss
  cmp  r2, r3
  bcc  FillZerobss

/* Call the clock system intitialization function.*/
  bl  SystemInit   
/* Call static constructors */
  bl __libc_init_array /*目前看来这个没什么用，如果代码量有超可以去掉*/
/* Call the application's entry point.*/
  bl  main
  bx  lr    
.size  Reset_Handler, .-Reset_Handler

/**
 * @brief  This is the code that gets called when the processor receives an 
 *         unexpected interrupt.  This simply enters an infinite loop, preserving
 *         the system state for examination by a debugger.
 * @param  None     
 * @retval None       
*/
    .section  .text.Default_Handler,"ax",%progbits
Default_Handler:
Infinite_Loop:
  b  Infinite_Loop
  .size  Default_Handler, .-Default_Handler
/******************************************************************************
*
* The minimal vector table for a Cortex M3. Note that the proper constructs
* must be placed on this to ensure that it ends up at physical address
* 0x0000.0000.
* 
*******************************************************************************/
   .section  .isr_vector,"a",%progbits
  .type  g_pfnVectors, %object
  .size  g_pfnVectors, .-g_pfnVectors
    
    
g_pfnVectors:
  .word  _estack
  .word  Reset_Handler
  .word  NMI_Handler
  .word  HardFault_Handler
  .word  MemManage_Handler
  .word  BusFault_Handler
  .word  UsageFault_Handler
  .word  0
  .word  0
  .word  0
  .word  0
  .word  SVC_Handler
  .word  DebugMon_Handler
  .word  0
  .word  PendSV_Handler
  .word  SysTick_Handler
  
  /* External Interrupts */
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
  .word     ISR_GlobalHandler
/*******************************************************************************
*
* Provide weak aliases for each Exception handler to the Default_Handler. 
* As they are weak aliases, any function with the same name will override 
* this definition.
* 
*******************************************************************************/
   .weak      NMI_Handler
   .thumb_set NMI_Handler,Default_Handler



	.section  .text.CmTrace_Handler
 	.type CmTrace_Handler, %function
CmTrace_Handler:
 	MOV     r0, lr                  /* get lr */
    MOV     r1, sp                  /* get stack pointer (current is MSP) */
    BL      cm_backtrace_fault
Fault_Loop:
    BL      Fault_Loop              /* while(1) */
    
   .weak      NMI_Handler
   .thumb_set NMI_Handler,CmTrace_Handler
  
   .weak      HardFault_Handler
   .thumb_set HardFault_Handler,CmTrace_Handler
  
   .weak      MemManage_Handler
   .thumb_set MemManage_Handler,CmTrace_Handler
  
   .weak      BusFault_Handler
   .thumb_set BusFault_Handler,CmTrace_Handler

   .weak      UsageFault_Handler
   .thumb_set UsageFault_Handler,CmTrace_Handler

   .weak      SVC_Handler
   .thumb_set SVC_Handler,Default_Handler

   .weak      DebugMon_Handler
   .thumb_set DebugMon_Handler,Default_Handler

   .weak      PendSV_Handler
   .thumb_set PendSV_Handler,Default_Handler

   .weak      SysTick_Handler
   .thumb_set SysTick_Handler,Default_Handler              
  
   .weak      ISR_GlobalHandler                   
   .thumb_set ISR_GlobalHandler,Default_Handler      
                  

