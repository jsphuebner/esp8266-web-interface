/*
 * Simple ARM debug interface for Arduino, using the SWD (Serial Wire Debug) port.
 *
 * Copyright (c) 2013 Micah Elizabeth Scott
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

#include <Arduino.h>
#include <StreamString.h>
#include <stdarg.h>
#include "arm_debug.h"
#include "arm_reg.h"
#include "flashloader/stm32x.h"
//#include "flashloader/stm32f0.h"

/*
Additional Research:
    https://research.kudelskisecurity.com/2019/05/16/swd-arms-alternative-to-jtag
    https://research.kudelskisecurity.com/2019/07/31/swd-part-2-the-mem-ap
    https://www.silabs.com/documents/public/application-notes/an0062.pdf
    https://www.silabs.com/documents/public/example-code/an0062-efm32-programming-guide.zip
    https://github.com/m-mcgowan/embedded-swd/blob/master/firmware/swd.h
    http://markding.github.io/swd_programing_sram
    https://github.com/MarkDing/swd_programing_sram
    https://www.cnblogs.com/shangdawei/p/4753040.html
    https://www.cnblogs.com/shangdawei/p/3164075.html
*/

ARMDebug::ARMDebug(unsigned clockPin, unsigned dataPin, LogLevel logLevel)
    : clockPin(clockPin), dataPin(dataPin), logLevel(logLevel)
{}

bool ARMDebug::begin()
{
    pinMode(clockPin, OUTPUT);
    pinMode(dataPin, INPUT_PULLUP);

    // Invalidate cache
    cache.select = 0xFFFFFFFF;

    // Put the bus in a known state, and trigger a JTAG-to-SWD transition.
    wireWriteTurnaround();
    wireWrite(0xFFFFFFFF, 32);
    wireWrite(0xFFFFFFFF, 32);
    wireWrite(0xE79E, 16);
    wireWrite(0xFFFFFFFF, 32);
    wireWrite(0xFFFFFFFF, 32);
    wireWrite(0, 32);
    wireWrite(0, 32);

	//The host must read IDCODE register after line request sequence.
    uint32_t idcode;
    if (!getIDCODE(idcode))
        return false;
    log(LOG_NORMAL, "Found ARM processor debug port (IDCODE: %08x)", idcode);

    if (!debugPortPowerup())
        return false;

    if (!initMemPort())
        return false;

    return true;
}

bool ARMDebug::getIDCODE(uint32_t &idcode)
{
    // Retrieve IDCODE
    if (!dpRead(IDCODE, false, idcode)) {
        log(LOG_ERROR, "No ARM processor detected. Check power and cables?");
        return false;
    }

    return true;
}

bool ARMDebug::debugPortPowerup()
{
    //Power up debug domain
    //dpWrite(4, 0x50000000);

    // Initialize CTRL/STAT, request system and debugger power-up
    if (!dpWrite(CTRLSTAT, false, CSYSPWRUPREQ | CDBGPWRUPREQ))
        return false;

    // Wait for power-up acknowledgment
    uint32_t ctrlstat;
    if (!dpReadPoll(CTRLSTAT, ctrlstat, CDBGPWRUPACK | CSYSPWRUPACK, -1)) {
        log(LOG_ERROR, "ARMDebug: Debug port failed to power on (CTRLSTAT: %08x)", ctrlstat);
        return false;
    }

    return true;
}

bool ARMDebug::debugPortReset()
{
    // Note: This is an optional feature.
    // It's implemented on the Cortex-M3 but not the M0+, for example.

    const uint32_t powerup = CSYSPWRUPREQ | CDBGPWRUPREQ;
    const uint32_t reset_request = powerup | CDBGRSTREQ;

    // Reset the debug access port
    if (!dpWrite(CTRLSTAT, false, reset_request))
        return false;

    // Wait for reset acknowledgment
    uint32_t ctrlstat;
    if (!dpReadPoll(CTRLSTAT, ctrlstat, CDBGRSTACK, -1)) {
        log(LOG_ERROR, "ARMDebug: Debug port failed to reset (CTRLSTAT: %08x)", ctrlstat);
        return false;
    }

    // Clear reset request bit (leave power requests on)
    if (!dpWrite(CTRLSTAT, false, powerup))
        return false;

    return true;
}

bool ARMDebug::initMemPort()
{
    // Make sure the default debug access port is an AHB (memory bus) port, like we expect
    uint32_t idr;
    if (!apRead(MEM_IDR, idr))
        return false;
    if ((idr & 0xF) != 1) {
        log(LOG_ERROR, "ARMDebug: Default access port is not an AHB-AP as expected! (IDR: %08x)", idr);
        return false;
    }

    // Invalidate CSW cache
    cache.csw = -1;

    return true;
}

bool ARMDebug::debugHalt()
{
    /*
     * Enable debug, request a halt, and read back status.
     *
     * This part is somewhat timing critical, since we're racing against the watchdog
     * timer. Avoid memWait() by calling the lower-level interface directly.
     *
     * Since this is expected to fail a bunch before succeeding, mute errors temporarily.
     */

    unsigned haltRetries = 10000;
    LogLevel savedLogLevel;
    uint32_t dhcsr;

    // Point at the debug halt control/status register. We disable MEM-AP autoincrement,
    // and leave TAR pointed at DHCSR for the entire loop.
    if (memWriteCSW(CSW_32BIT) && apWrite(MEM_TAR, REG_SCB_DHCSR)) {

        setLogLevel(LOG_NONE, savedLogLevel);

        while (haltRetries) {
            haltRetries--;

            //DHCSR bits C_STOP and C_DEBUGEN (2 + 1)
            if (!apWrite(MEM_DRW, 0xA05F0003)) //STEP AND HALT
                continue;
            if (!apRead(MEM_DRW, dhcsr))
                continue;

            if (dhcsr & (1 << 17)) { //checking whether it actually is halted (0x30000)
                // Halted!
                break;
            }
        }

        setLogLevel(savedLogLevel);
    }

    if (!haltRetries) {
        log(LOG_ERROR, "ARMDebug: Failed to put CPU in debug halt state. (DHCSR: %08x)", dhcsr);
        return false;
    }

    return true;
}

bool ARMDebug::debugHaltOnReset(uint8_t enable)
{
    /* 
    //Enable TRC (Trace and Debug blocks) (including DWT)
    DEMCR &= ~0x01000000; //disable
    DEMCR |= 0x01000000; //enable
    */

	// Vector catch is the mechanism for generating a debug event 
    // and entering Debug state when a particular exception occurs

    uint32_t VC_CORERESET = 0x0;
    if(enable == 1)
        VC_CORERESET = 0x1; //Halt after Reset set VC_CORERESET bit to 1

    if (apWrite(MEM_TAR, REG_SCB_DEMCR))
        if (apWrite(MEM_DRW, VC_CORERESET))
            return true;

    return false;
}

bool ARMDebug::debugReset()
{
    if (apWrite(MEM_TAR, REG_SCB_AIRCR))
        if (apWrite(MEM_DRW, 0xFA050004))
            return true;

    return false;
}

bool ARMDebug::debugStep()
{
    if (apWrite(MEM_TAR, REG_SCB_DHCSR))
        if (apWrite(MEM_DRW, 0xA05F0005))
            return true;
    return false;
}

bool ARMDebug::debugRun()
{
	const uint32_t DHCSR_DEBUG_KEY = 0xA05F0000;

    if (apWrite(MEM_TAR, REG_SCB_DHCSR))
        if (apWrite(MEM_DRW, DHCSR_DEBUG_KEY))
            return true;
    return false;
}

bool ARMDebug::flashloaderSRAM()
{
    uint32_t i, size, addr = MEMAP_SRAM_START;

    size = sizeof(flashloader_raw);
    for (i = 0; i < size; i++)
        memStoreByte(addr + i, flashloader_raw[i]);
}

bool ARMDebug::flashloaderRUN(uint32_t addr, unsigned count)
{
    uint32_t buf_addr = MEMAP_SRAM_START + sizeof(flashloader_raw); //flashloader end address

    // Update vector table entry in 0xe000ed08 to SRAM start position 0x20000000
    memStore(REG_SCB_VTOR, MEMAP_SRAM_START); //Debugger to check the core VTOR register

    // https://www.st.com/resource/en/programming_manual/cd00228163-stm32f10xxx-20xxx-21xxx-l1xxxx-cortex-m3-programming-manual-stmicroelectronics.pdf

    // Update R15(PC) with reset vector address. It locates at second word position in firmware.
    /*
    The Program Counter (PC) is register R15. It contains the current program address.
    On reset, the processor loads the PC with the value of the reset vector, which is at address 0x00000004.
    */
    //regWrite(15, flashloader_raw[1] & 0xFFFFFFFE); // R15
    //Serial.printf("Flasloader Program Counter %08x\n", flashloader_raw[1] & 0xFFFFFFFE);

    // Update R13(SP) with stack address defined in first word in firmware.
    /*
    The Stack Pointer (SP) is register R13. In Thread mode, bit[1] of the CONTROL register indicates the stack pointer to use:
    0 = Main Stack Pointer (MSP). This is the reset value.
    1 = Process Stack Pointer (PSP).
    On reset, the processor loads the MSP with the value from address 0x00000000.
    */
    //regWrite(13, flashloader_raw[0]);              // R13
    //Serial.printf("Flasloader Stack Pointer %08x\n", flashloader_raw[0]);

    // Flashloader Variables
    // ----------------------
    // Communication implemented by using fixed-address variables that both parties check (flashloader <-> CPU R0, R1, R2, R3)
    log(LOG_NORMAL,"Flasloader Buffer Start %08x\n", buf_addr);
    log(LOG_NORMAL, "Flasloader Page Count %d\n", count / 2);
    
    //OpenOCD stm32x.h
    regWrite(0, buf_addr);    // Source
    regWrite(1, addr);        // Target
    regWrite(2, count / 2);   // Count(16 bits half words)
    //regWrite(3, 0);         // Result
    /*
    //ST-Link stm32f0.h
    regWrite(0, buf_addr);    // Source
    regWrite(1, addr);        // Target
    regWrite(2, count / 2);   // Count(16 bits half words)
    regWrite(3, 0);           // Flash Base 0
    //if (addr >= MEMAP_FLASH_BANK2_START)
    //    regWrite(3, REG_FPEC_FLASH_BANK2_OFS); // Flash Base 0 with Offset
    */
    regWrite(15, MEMAP_SRAM_START);

    //Debug
    uint32_t r0, r1, r2, r3, r15;
    regRead(0, r0);
    regRead(1, r1);
    regRead(2, r2);
    regRead(3, r3);
    regRead(15, r15);
    log(LOG_NORMAL, "R0:%08x R1:%08x R2:%08x R3:%08x R15:%08x \n", r0, r1, r2, r3, r15);

    unlockFlash();

    //Run code
    memStore(REG_SCB_DHCSR, 0xA05F0000);

    flashWait();
}

bool ARMDebug::writeBufferSRAM(uint32_t addr, const uint8_t *data, unsigned count)
{
    // Write the buffer right after the loader
    uint32_t i, buf_addr = MEMAP_SRAM_START + sizeof(flashloader_raw) + addr; //flashloader end address

    for (i = 0; i < count; i++)
    {
        memStoreByte(buf_addr + i, data[i]);
        memWait();
    }
}

bool ARMDebug::lockFlash()
{
	uint32_t flashlock;
	apRead(REG_FPEC_FLASH_CR, flashlock);

	if (!(flashlock & FLASH_CR_LOCK))
    {
    	if (apWrite(REG_FPEC_FLASH_CR, FLASH_CR_LOCK))
    		return true;
    }
    return false;
}

bool ARMDebug::unlockFlash()
{
    //https://ioprog.com/2018/05/23/using-flash-memory-on-the-stm32f103

    //FLASH_CR default value is 0x80000000. Bit 7 is reset by hardware after detecting unlock sequence. So expected value is 0x00
    uint32_t flashlock;

	//apRead(REG_FPEC_FLASH_CR, flashlock);
    //log(LOG_NORMAL, "Flash Unlock Check %08x\n",flashlock);

	//if (flashlock & FLASH_CR_LOCK) {
    	/*
	    An unlocking sequence should be written to the FLASH_KEYR register to open up the FPEC block.
	    This sequence consists of two write cycles, where two key values (KEY1 and KEY2) are written to the FLASH_KEYR address
	    */
        memStore(REG_FPEC_FLASH_KEYR, REG_FPEC_KEY_KEY1);
        memStore(REG_FPEC_FLASH_KEYR, REG_FPEC_KEY_KEY2);
        flashWait();
	    //Authorize the programming of the option bytes by writing the same set of KEYS (KEY1 and KEY2) to the FLASH_OPTKEYR register
	    //memStore(REG_FPEC_FLASH_OPTKEYR, REG_FPEC_KEY_KEY1);
	    //memStore(REG_FPEC_FLASH_OPTKEYR, REG_FPEC_KEY_KEY2);
        //flashWait();
    //}
    //Any wrong sequence locks up the FPEC block and FLASH_CR register until the next reset.

    return true;
}

int ARMDebug::flashWait()
{
	//Programming error flags. Cleared by writing this value into FLASH_CR
	const uint32_t FLASH_SR_ERROR_FLAGS = 0xF0;
	const uint32_t FLASH_SR_BUSY = 1<<16;
    const uint32_t FLASH_SR_WRPRTERR = 4;

    uint32_t timeout = 10000;
    uint32_t start = millis();
    uint32_t status;

    for (;;)
	{
		apRead(REG_FPEC_FLASH_SR, status);
        //if (status & FLASH_SR_WRPRTERR)
        //    return -2;
        if (!(status & FLASH_SR_BUSY))
            break;
		if (millis()-start > timeout)
		{
            log(LOG_ERROR, "Flash Wait Timeout %08x\n",status);
			return -1;
		}
	}
	if (status & FLASH_SR_ERROR_FLAGS)
	{
		// clear the errors
		memStore(REG_FPEC_FLASH_SR, status & ~FLASH_SR_ERROR_FLAGS);
	}

    return 0;
}

bool ARMDebug::flashEraseAll()
{
    unlockFlash();

    uint32_t flashcr;
    if (apRead(REG_FPEC_FLASH_CR, flashcr))
    {
        log(LOG_NORMAL, "FLASH_CR Before %08x\n",flashcr);
        flashcr &= ~FLASH_CR_PG;    // ensure PG bit low
        flashcr &= ~FLASH_CR_PER;   // ensure PER is low
        log(LOG_NORMAL, "FLASH_CR After %08x\n",flashcr);

        memStore(REG_FPEC_FLASH_CR, flashcr |=  FLASH_CR_MER); // set MER bit
        memStore(REG_FPEC_FLASH_CR, flashcr |=  FLASH_CR_STRT); // set STRT bit

        flashWait();

        return true;
    }

    return false;
}

bool ARMDebug::flashErase(uint32_t addr)
{
    unlockFlash();

    uint32_t flashcr;
    if (apRead(REG_FPEC_FLASH_CR, flashcr))
    {
        log(LOG_NORMAL, "FLASH_CR Before %08x\n",flashcr);
        flashcr &= ~FLASH_CR_PG;    // ensure PG bit low
        flashcr &= ~FLASH_CR_MER;   // ensure MER is low
        log(LOG_NORMAL, "FLASH_CR After %08x\n",flashcr);

        memStore(REG_FPEC_FLASH_CR, flashcr |= FLASH_CR_PER);   // set PER bit
        memStore(REG_FPEC_FLASH_AR, addr);
        memStore(REG_FPEC_FLASH_CR, flashcr |= FLASH_CR_STRT);  // set STRT bit

        flashWait();
        return true;
    }

    return false;
}

bool ARMDebug::flashWrite(uint32_t addr, uint32_t data)
{
	flashWait();

    unlockFlash();

    uint32_t flashcr;
    if (apRead(REG_FPEC_FLASH_CR, flashcr))
    {
        apWrite(REG_FPEC_FLASH_CR, flashcr &= ~1);  // ensure PER is low
        apWrite(REG_FPEC_FLASH_CR, flashcr &= ~2);  // ensure MER is low
        apWrite(REG_FPEC_FLASH_CR, flashcr |= 0);   // set PG bit

        //Perform the data write (half-word) at the desired address
        memStoreHalf(addr, data >> 16);
        memStoreHalf(addr + 2, data & 0x0000FFFF);

        flashWait();
    }else{
    	return false;
    }

	return true;
}

bool ARMDebug::regTransactionHandshake()
{
    const uint32_t S_REGRDY = 1<<16;
    uint32_t dhcsr;
    if (!memLoad(REG_SCB_DHCSR, dhcsr)) {
        // Lower-level communications error
        return false;
    }
    return (dhcsr & S_REGRDY) != 0;
}

bool ARMDebug::regWrite(unsigned num, uint32_t data)
{
    const uint32_t write = 0x10000;
    num &= 0xFFFF;
    return memStore(REG_SCB_DCRDR, data)
        && memStore(REG_SCB_DCRSR, num | write)
        && regTransactionHandshake();
}

bool ARMDebug::regRead(unsigned num, uint32_t& data)
{
    num &= 0xFFFF;
    return memStore(REG_SCB_DCRSR, num)
        && regTransactionHandshake()
        && memLoad(REG_SCB_DCRDR, data);
}

bool ARMDebug::memWriteCSW(uint32_t data)
{
    // Write to the MEM-AP CSW. Duplicate writes are ignored, and the cache is updated.

    if (data == cache.csw)
        return true;

    if (!apWrite(MEM_CSW, data | CSW_DEFAULTS))
        return false;

    cache.csw = data;
    return true;
}

bool ARMDebug::memWait()
{
    // Wait for a transaction to complete & the port to be enabled.

    uint32_t csw;
    if (!apReadPoll(MEM_CSW, csw, CSW_TRIN_PROG | CSW_DEVICE_EN, CSW_DEVICE_EN)) {
        log(LOG_ERROR, "ARMDebug: Error while waiting for memory port");
        return false;
    }

    return true;
}

bool ARMDebug::memStore(uint32_t addr, uint32_t data)
{
    return memStore(addr, &data, 1);
}

bool ARMDebug::memLoad(uint32_t addr, uint32_t &data)
{
    return memLoad(addr, &data, 1);
}

bool ARMDebug::memStoreAndVerify(uint32_t addr, uint32_t data)
{
    return memStoreAndVerify(addr, &data, 1);
}

bool ARMDebug::memStoreAndVerify(uint32_t addr, const uint32_t *data, unsigned count)
{
    if (!memStore(addr, data, count))
        return false;

    if (!memWait())
        return false;
    if (!apWrite(MEM_TAR, addr))
        return false;

    while (count) {
        uint32_t readback;

        if (!memWait())
            return false;
        if (!apRead(MEM_DRW, readback))
            return false;

        log(readback == *data ? LOG_TRACE_MEM : LOG_ERROR,
            "MEM Verif [%08x] %08x (expected %08x)", addr, readback, *data);

        if (readback != *data)
            return false;

        data++;
        addr++;
        count--;
    }

    return true;
}

bool ARMDebug::memStore(uint32_t addr, const uint32_t *data, unsigned count)
{
    if (!memWait())
        return false;
    if (!memWriteCSW(CSW_32BIT | CSW_ADDRINC_SINGLE))
        return false;
    if (!apWrite(MEM_TAR, addr))
        return false;

    while (count) {
        log(LOG_TRACE_MEM, "MEM Store [%08x] %08x", addr, *data);

        if (!memWait())
            return false;
        if (!apWrite(MEM_DRW, *data))
            return false;

        data++;
        addr++;
        count--;
    }

    return true;
}

bool ARMDebug::memLoad(uint32_t addr, uint32_t *data, unsigned count)
{
    if (!memWait())
        return false;
    if (!memWriteCSW(CSW_32BIT | CSW_ADDRINC_SINGLE))
        return false;
    if (!apWrite(MEM_TAR, addr))
        return false;

    while (count) {
        if (!memWait())
            return false;
        if (!apRead(MEM_DRW, *data))
            return false;

        log(LOG_TRACE_MEM, "MEM Load  [%08x] %08x", addr, *data);

        data++;
        addr++;
        count--;
    }

    return true;
}

bool ARMDebug::memStoreByte(uint32_t addr, uint8_t data)
{
    if (!memWait())
        return false;
    if (!memWriteCSW(CSW_8BIT))
        return false;
    if (!apWrite(MEM_TAR, addr))
        return false;

    log(LOG_TRACE_MEM, "MEM Store [%08x] %02x", addr, data);

    // Replicate across lanes
    uint32_t word = data | (data << 8) | (data << 16) | (data << 24);

    return apWrite(MEM_DRW, word);
}

bool ARMDebug::memLoadByte(uint32_t addr, uint8_t &data)
{
    uint32_t word;
    if (!memWait())
        return false;
    if (!memWriteCSW(CSW_8BIT))
        return false;
    if (!apWrite(MEM_TAR, addr))
        return false;
    if (!apRead(MEM_DRW, word))
        return false;

    // Select the proper lane
    data = word >> ((addr & 3) << 3);

    log(LOG_TRACE_MEM, "MEM Load  [%08x] %02x", addr, data);
    return true;
}

bool ARMDebug::memStoreHalf(uint32_t addr, uint16_t data)
{
    if (!memWait())
        return false;
    if (!memWriteCSW(CSW_16BIT))
        return false;
    if (!apWrite(MEM_TAR, addr))
        return false;

    log(LOG_TRACE_MEM, "MEM Store [%08x] %04x", addr, data);

    // Replicate across lanes
    uint32_t word = data | (data << 16);

    return apWrite(MEM_DRW, word);
}

bool ARMDebug::memLoadHalf(uint32_t addr, uint16_t &data)
{
    uint32_t word;
    if (!memWait())
        return false;
    if (!memWriteCSW(CSW_16BIT))
        return false;
    if (!apWrite(MEM_TAR, addr))
        return false;
    if (!apRead(MEM_DRW, word))
        return false;

    // Select the proper lane
    data = word >> ((addr & 2) << 3);

    log(LOG_TRACE_MEM, "MEM Load  [%08x] %04x", addr, data);
    return true;
}

bool ARMDebug::apWrite(unsigned addr, uint32_t data)
{
    log(LOG_TRACE_AP, "AP  Write [%x] %08x", addr, data);
    return dpSelect(addr) && dpWrite(addr, true, data);
}

bool ARMDebug::apRead(unsigned addr, uint32_t &data)
{
    /*
     * This is really hard to find in the docs, but AP reads are delayed by one transaction.
     * So, the very first AP read will return undefined data, the next AP read returns the
     * data for the previous address, and so on.
     *
     * See:
     *   http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0314h/ch02s04s03.html
     *
     * Solutions include:
     *   - Tracking DP reads and their responses asynchronously. Hard but efficient. Not for us.
     *   - Dummy read, to flush the read through.
     *   - Better yet, we can do this explicitly with the RDBUFF register in ADI5v1.
     */

    unsigned dummyAddr = (addr & kSelectMask) | MEM_IDR;  // No side effects, same AP and bank
    uint32_t dummyData;

    bool result = dpSelect(addr) &&                  // Select AP and register bank
                  dpRead(addr, true, dummyData) &&   // Initiate read, returns dummy data
                  dpRead(RDBUFF, false, data);       // Explicitly request read results

    if (result) {
        log(LOG_TRACE_AP, "AP  Read  [%x] %08x", addr, data);
    }

    return result;
}

bool ARMDebug::dpReadPoll(unsigned addr, uint32_t &data, uint32_t mask, uint32_t expected, unsigned retries)
{
    expected &= mask;
    do {
        if (!dpRead(addr, false, data))
            return false;
        if ((data & mask) == expected)
            return true;
        yield();
    } while (retries--);

    log(LOG_ERROR, "ARMDebug: Timed out while polling DP ([%08x] & %08x == %08x). Current value: %08x",
        addr, mask, expected, data);
    return false;
}

bool ARMDebug::apReadPoll(unsigned addr, uint32_t &data, uint32_t mask, uint32_t expected, unsigned retries)
{
    expected &= mask;
    do {
        if (!apRead(addr, data))
            return false;
        if ((data & mask) == expected)
            return true;
        yield();
    } while (retries--);

    log(LOG_ERROR, "ARMDebug: Timed out while polling AP ([%08x] & %08x == %08x). Current value: %08x",
        addr, mask, expected, data);
    return false;
}

bool ARMDebug::memPoll(unsigned addr, uint32_t &data, uint32_t mask, uint32_t expected, unsigned retries)
{
    expected &= mask;
    do {
        if (!memLoad(addr, data))
            return false;
        if ((data & mask) == expected)
            return true;
        yield();
    } while (retries--);

    log(LOG_ERROR, "ARMDebug: Timed out while polling MEM ([%08x] & %08x == %08x). Current value: %08x",
        addr, mask, expected, data);
    return false;
}

bool ARMDebug::memPollByte(unsigned addr, uint8_t &data, uint8_t mask, uint8_t expected, unsigned retries)
{
    expected &= mask;
    do {
        if (!memLoadByte(addr, data))
            return false;
        if ((data & mask) == expected)
            return true;
        yield();
    } while (retries--);

    log(LOG_ERROR, "ARMDebug: Timed out while polling MEM ([%08x] & %02x == %02x). Current value: %02x",
        addr, mask, expected, data);
    return false;
}

bool ARMDebug::dpSelect(unsigned addr)
{
    /*
     * Select a new access port and/or a bank. Uses only the high byte and
     * second nybble of 'addr'. This is cached, so redundant dpSelect()s have no effect.
     */

    uint32_t select = addr & kSelectMask;

    if (select != cache.select) {
        if (!dpWrite(SELECT, false, select))
            return false;
        cache.select = select;
    }
    return true;
}

bool ARMDebug::dpWrite(unsigned addr, bool APnDP, uint32_t data)
{
    unsigned retries = 10;
    unsigned ack;
    log(LOG_TRACE_DP, "DP  Write [%x:%x] %08x", addr, APnDP, data);

    do {
        wireWrite(packHeader(addr, APnDP, false), 8);
        wireReadTurnaround();
        ack = wireRead(3);
        wireWriteTurnaround();

        switch (ack) {
            case 1:     // Success
                wireWrite(data, 32);
                wireWrite(evenParity(data), 1);
                wireWriteIdle();
                return true;

            case 2:     // WAIT
                wireWriteIdle();
                log(LOG_TRACE_DP, "DP  WAIT response, %d retries left", retries);
                retries--;
                break;

            case 4:     // FAULT
                wireWriteIdle();
                log(LOG_ERROR, "FAULT response during write (addr=%x APnDP=%d data=%08x)",
                    addr, APnDP, data);
                if (!handleFault())
                    log(LOG_ERROR, "Error during fault recovery!");
                return false;

            default:
                wireWriteIdle();
                log(LOG_ERROR, "PROTOCOL ERROR response during write (ack=%x addr=%x APnDP=%d data=%08x)",
                    ack, addr, APnDP, data);
                return false;
        }
    } while (retries--);

    log(LOG_ERROR, "WAIT timeout during write (addr=%x APnDP=%d data=%08x)",
        addr, APnDP, data);
    return false;
}

bool ARMDebug::dpRead(unsigned addr, bool APnDP, uint32_t &data)
{
    unsigned retries = 10;
    unsigned ack;

    do {
        wireWrite(packHeader(addr, APnDP, true), 8);
        wireReadTurnaround();
        ack = wireRead(3);

        switch (ack) {
            case 1:     // Success
                data = wireRead(32);
                if (wireRead(1) != evenParity(data)) {
                    wireWriteTurnaround();
                    wireWriteIdle();
                    log(LOG_ERROR, "PARITY ERROR during read (addr=%x APnDP=%d data=%08x)",
                        addr, APnDP, data);
                    return false;
                }
                wireWriteTurnaround();
                wireWriteIdle();
                log(LOG_TRACE_DP, "DP  Read  [%x:%x] %08x", addr, APnDP, data);
                return true;

            case 2:     // WAIT
                wireWriteTurnaround();
                wireWriteIdle();
                log(LOG_TRACE_DP, "DP  WAIT response, %d retries left", retries);
                retries--;
                break;

            case 4:     // FAULT
                wireWriteTurnaround();
                wireWriteIdle();
                log(LOG_ERROR, "FAULT response during read (addr=%x APnDP=%d)", addr, APnDP);
                if (!handleFault())
                    log(LOG_ERROR, "Error during fault recovery!");
                return false;

            default:
                wireWriteTurnaround();
                wireWriteIdle();
                log(LOG_ERROR, "PROTOCOL ERROR response during read (ack=%x addr=%x APnDP=%d)", ack, addr, APnDP);
                return false;
        }
    } while (retries--);

    log(LOG_ERROR, "WAIT timeout during read (addr=%x APnDP=%d)", addr, APnDP);
    return false;
}

bool ARMDebug::handleFault()
{
    // Read CTRLSTAT to see what the fault was, and set appropriate ABORT bits

    uint32_t ctrlstat;
    uint32_t abortbits = 0;
    bool dumpRegs = false;

    if (!dpRead(CTRLSTAT, false, ctrlstat))
        return false;

    if (ctrlstat & (1 << 7)) {
        log(LOG_ERROR, "FAULT: Detected WDATAERR");
        abortbits |= 1 << 3;
    }
    if (ctrlstat & (1 << 4)) {
        log(LOG_ERROR, "FAULT: Detected STICKCMP");
        abortbits |= 1 << 1;
    }
    if (ctrlstat & (1 << 1)) {
        log(LOG_ERROR, "FAULT: Detected STICKORUN");
        abortbits |= 1 << 4;
    }
    if (ctrlstat & (1 << 5)) {
        log(LOG_ERROR, "FAULT: Detected STICKYERR");
        abortbits |= 1 << 2;
        dumpRegs = true;
    }

    if (abortbits && !dpWrite(ABORT, false, abortbits))
        return false;

    if (dumpRegs && !dumpMemPortRegisters()) {
        log(LOG_ERROR, "Failed to dump memory port registers for diagnostics.");
        return false;
    }

    return true;
}

bool ARMDebug::dumpMemPortRegisters()
{
    uint32_t reg;

    if (!apRead(MEM_IDR, reg)) return false;
    log(LOG_ERROR, "  IDR = %08x", reg);

    if (!apRead(MEM_CSW, reg)) return false;
    log(LOG_ERROR, "  CSW = %08x", reg);

    if (!apRead(MEM_TAR, reg)) return false;
    log(LOG_ERROR, "  TAR = %08x", reg);

    return true;
}

uint8_t ARMDebug::packHeader(unsigned addr, bool APnDP, bool RnW)
{
    bool a2 = (addr >> 2) & 1;
    bool a3 = (addr >> 3) & 1;
    bool parity = APnDP ^ RnW ^ a2 ^ a3;
    return (1 << 0)              |  // Start
           (APnDP << 1)          |
           (RnW << 2)            |
           ((addr & 0xC) << 1)   |
           (parity << 5)         |
           (1 << 7)              ;  // Park
}

bool ARMDebug::evenParity(uint32_t word)
{
    word = (word & 0xFFFF) ^ (word >> 16);
    word = (word & 0xFF) ^ (word >> 8);
    word = (word & 0xF) ^ (word >> 4);
    word = (word & 0x3) ^ (word >> 2);
    word = (word & 0x1) ^ (word >> 1);
    return word;
}

void ARMDebug::wireWrite(uint32_t data, unsigned nBits)
{
    log(LOG_TRACE_SWD, "SWD Write %08x (%d)", data, nBits);

    while (nBits--) {
        digitalWrite(dataPin, data & 1);
        digitalWrite(clockPin, LOW);
        data >>= 1;
        digitalWrite(clockPin, HIGH);
    }
}

void ARMDebug::wireWriteIdle()
{
    // Minimum 8 clock cycles.
    wireWrite(0, 32);
}

uint32_t ARMDebug::wireRead(unsigned nBits)
{
    uint32_t result = 0;
    uint32_t mask = 1;
    unsigned count = nBits;

    while (count--) {
        if (digitalRead(dataPin)) {
            result |= mask;
        }
        digitalWrite(clockPin, LOW);
        mask <<= 1;
        digitalWrite(clockPin, HIGH);
    }

    log(LOG_TRACE_SWD, "SWD Read  %08x (%d)", result, nBits);
    return result;
}

void ARMDebug::wireWriteTurnaround()
{
    log(LOG_TRACE_SWD, "SWD Write trn");

    digitalWrite(dataPin, HIGH);
    pinMode(dataPin, INPUT_PULLUP);
    digitalWrite(clockPin, LOW);
    digitalWrite(clockPin, HIGH);
    pinMode(dataPin, OUTPUT);
}

void ARMDebug::wireReadTurnaround()
{
    log(LOG_TRACE_SWD, "SWD Read  trn");

    digitalWrite(dataPin, HIGH);
    pinMode(dataPin, INPUT_PULLUP);
    digitalWrite(clockPin, LOW);
    digitalWrite(clockPin, HIGH);
}

void ARMDebug::log(int level, const char *fmt, ...)
{
    if (level <= logLevel && Serial) {
        va_list ap;
        char buffer[256];

        va_start(ap, fmt);
        int ret = vsnprintf(buffer, sizeof buffer, fmt, ap);
        va_end(ap);

        Serial.println(buffer);
    }
}

void ARMDebug::binDump(uint32_t addr, uint8_t* &buffer)
{
    // Hex dump target memory to buffer

    va_list ap;
    LogLevel oldLogLevel;
    setLogLevel(LOG_NONE, oldLogLevel); //force log level none

    uint8_t byte;
    memLoadByte(addr, byte);

    *buffer = byte;
    
    setLogLevel(oldLogLevel); //restore previous log level
}

void ARMDebug::hexDump(uint32_t addr, unsigned count, StreamString &data)
{
    // Hex dump target memory to StreamString

    va_list ap;
    char buffer[32];
    LogLevel oldLogLevel;
    setLogLevel(LOG_NONE, oldLogLevel); //force log level none

    while (count) {
        snprintf(buffer, sizeof buffer, "%08x:", addr);
        data.print(buffer);

        for (unsigned x = 0; count && x < 4; x++) {
            uint32_t word;
            if (memLoad(addr, word)) {

                /*
                word = ((word << 8) & 0xFF00FF00) | ((word >> 8) & 0xFF00FF); 
                word = (word << 16) | ((word >> 16) & 0xFFFF);
                snprintf(buffer, sizeof buffer, " %08x", word);
                */

                uint8_t bytes[3];
                bytes[0] = (word >> 0)  & 0xFF;
                bytes[1] = (word >> 8)  & 0xFF;
                bytes[2] = (word >> 16) & 0xFF;
                bytes[3] = (word >> 24) & 0xFF;
                //snprintf(buffer, sizeof buffer, " | %02x %02x %02x %02x", bytes[3], bytes[2], bytes[1], bytes[0]); // big-endian
                snprintf(buffer, sizeof buffer, " | %02x %02x %02x %02x", bytes[0], bytes[1], bytes[2], bytes[3]); // little-endian (reversed)

                data.print(buffer);
            } else {
                data.print(" (error )");
            }

            count--;
            addr += 4; //int32 word = 4 individual bytes
        }

        data.println();
    }
    setLogLevel(oldLogLevel); //restore previous log level
}

void ARMDebug::setLogLevel(LogLevel newLevel)
{
    logLevel = newLevel;
}

void ARMDebug::setLogLevel(LogLevel newLevel, LogLevel &prevLevel)
{
    prevLevel = logLevel;
    logLevel = newLevel;
}
