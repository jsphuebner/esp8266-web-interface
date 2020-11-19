/*
 * Register definitions for remote debugging on ARMv6-M chips.
 */

#pragma once

// Cortex M3 Memory Access Port
#define MEMAP_BANK_0  0x00000000       					// BANK 0 => CSW, TAR, Reserved, DRW
#define MEMAP_BANK_1  0x00000001						// BANK 1 => BD0, BD1, BD2, BD3

// Flash program and erase controller (FPEC), PM0075 manual STM32F10xxx Flash memory
#define REG_FPEC_FLASH_ACR			0x40022000			//Flash access control register
#define REG_FPEC_FLASH_KEYR 		0x40022004 			//FPEC key register
#define REG_FPEC_FLASH_OPTKEYR 		0x40022008 			//Option byte key register
#define REG_FPEC_FLASH_SR  			0x4002200C 			//Flash status register
#define REG_FPEC_FLASH_CR 			0x40022010  		//Flash control register
#define REG_FPEC_FLASH_AR 			0x40022014 			//Flash address register
#define REG_FPEC_FLASH_OBR  		0x4002201C 			//Option byte register
#define REG_FPEC_FLASH_WRPR 		0x40022020 			//Write protection register
#define REG_FPEC_KEY_RDPRT 			0x00A5
#define REG_FPEC_KEY_KEY1 			0x45670123
#define REG_FPEC_KEY_KEY2 			0xCDEF89AB

// System Control Space (SCS), ARMv7 ref manual, B3.2, page 708
#define REG_SCB_CPUID               0xE000ED00          // CPUID Base Register
#define REG_SCB_ICSR                0xE000ED04          // Interrupt Control and State
#define REG_SCB_ICSR_PENDSTSET      0x04000000
#define REG_SCB_VTOR                0xE000ED08          // Vector Table Offset
#define REG_SCB_AIRCR               0xE000ED0C          // Application Interrupt and Reset Control
#define REG_SCB_SCR                 0xE000ED10          // System Control Register
#define REG_SCB_CCR                 0xE000ED14          // Configuration and Control
#define REG_SCB_SHPR1               0xE000ED18          // System Handler Priority Register 1
#define REG_SCB_SHPR2               0xE000ED1C          // System Handler Priority Register 2
#define REG_SCB_SHPR3               0xE000ED20          // System Handler Priority Register 3
#define REG_SCB_SHCSR               0xE000ED24          // System Handler Control and State
#define REG_SCB_CFSR                0xE000ED28          // Configurable Fault Status Register
#define REG_SCB_HFSR                0xE000ED2C          // HardFault Status
#define REG_SCB_DFSR                0xE000ED30          // Debug Fault Status
#define REG_SCB_MMFAR               0xE000ED34          // MemManage Fault Address
#define REG_SCB_DHCSR               0xE000EDF0          // Debug Halting Control and Status Register
#define REG_SCB_DCRSR               0xE000EDF4          // Debug Core Register Selector Register
#define REG_SCB_DCRDR               0xE000EDF8          // Debug Core Register Data Register
#define REG_SCB_DEMCR               0xE000EDFC          // Debug Exception and Monitor Control Register

#define REG_SYST_CSR                0xE000E010          // SysTick Control and Status
#define REG_SYST_CSR_COUNTFLAG      0x00010000
#define REG_SYST_CSR_CLKSOURCE      0x00000004
#define REG_SYST_CSR_TICKINT        0x00000002
#define REG_SYST_CSR_ENABLE         0x00000001
#define REG_SYST_RVR                0xE000E014          // SysTick Reload Value Register
#define REG_SYST_CVR                0xE000E018          // SysTick Current Value Register
#define REG_SYST_CALIB              0xE000E01C          // SysTick Calibration Value

#define REG_ARM_DEMCR               0xE000EDFC          // Debug Exception and Monitor Control
#define REG_ARM_DEMCR_TRCENA        (1 << 24)           // Enable debugging & monitoring blocks
#define REG_ARM_DWT_CTRL            0xE0001000          // DWT control register
#define REG_ARM_DWT_CTRL_CYCCNTENA  (1 << 0)            // Enable cycle count
#define REG_ARM_DWT_CYCCNT          0xE0001004          // Cycle count register
