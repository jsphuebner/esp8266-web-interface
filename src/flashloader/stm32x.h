/* 
 * From OpenOCD, contrib/loaders/flash/stm32.s
 * i = STM32_FLASH_BASE;    0x08+r3  0x0800,0000
 * r0 = fl->buf_addr;       // source address
 * r1 = target;             // target address
 * r2 = count;              // count (16 bits half words)
 * r3 = 0;                  // result
 * r15 = fl->loader_addr;   // pc register
 */

//flashloader/stm32x.s
static const uint8_t flashloader_raw[] = {
    /* #define STM32_FLASH_CR_OFFSET  0x10 */
    /* #define STM32_FLASH_SR_OFFSET  0x0C */
    /* write: */
    0xdf, 0xf8, 0x20, 0x40,   /* ldr  r4, STM32_FLASH_BASE */
    /* write_half_word: */
    0x01, 0x23,         /* movs r3, #0x01 */
    0x23, 0x61,         /* str  r3, [r4, #STM32_FLASH_CR_OFFSET] */
    0x30, 0xf8, 0x02, 0x3b,   /* ldrh r3, [r0], #0x02 */
    0x21, 0xf8, 0x02, 0x3b,   /* strh r3, [r1], #0x02 */
    /* busy: */
    0xe3, 0x68,         /* ldr  r3, [r4, #STM32_FLASH_SR_OFFSET] */
    0x13, 0xf0, 0x01, 0x0f,   /* tst  r3, #0x01 */
    0xfb, 0xd0,         /* beq  busy */
    0x13, 0xf0, 0x14, 0x0f,   /* tst  r3, #0x14 */
    0x01, 0xd1,         /* bne  exit */
    0x01, 0x3a,         /* subs r2, r2, #0x01 */
    0xf0, 0xd1,         /* bne  write_half_word */
    /* exit: */
    0x00, 0xbe,         /* bkpt #0x00 */
    0x00, 0x20, 0x02, 0x40,   /* STM32_FLASH_BASE: .word 0x40022000 */
};
