/*
 * This file is part of the esp8266 web interface
 *
 * Copyright (C) 2018 Johannes Huebner <dev@johanneshuebner.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

var docstrings = {
    data: {
        /* spot values */
        version: "Firmware version.",
        hwver: "Hardware version",
        opmode: "Operating mode. 0=Off, 1=Run, 2=Manual_run, 3=Boost, 4=Buck, 5=Sine, 6=2 Phase sine",
        lasterr: "Last error message",
        status: "",
        udc: "Voltage on the DC side of the inverter. a.k.a, battery voltage.",
        idc: "Current passing through the DC side of the inverter (calculated).", 
        il1: "Current passing through the first current sensor on the AC side.",
        il2: "Current passing through the second current sensor on the AC side.",
        id: "",
        iq: "",
        ud: "",
        uq: "",
        heatcur: "", 
        fstat: "Stator frequency",
        speed: "The speed (rpm) of the motor.",
        cruisespeed: "", 
        turns: "Number of turns the motor has completed since startup.",
        amp: "Sine amplitude, 37813=max",
        angle: "Motor rotor angle, 0-360°. When using the SINE software, the slip is added to the rotor position.",
        pot: "Pot value, 4095=max",
        pot2: "Regen Pot value, 4095=max",
        potnom: "Scaled pot value, 0 accel",
        dir: "Rotation direction. -1=REV, 0=Neutral, 1=FWD",
        tmphs: "Inverter heatsink temperature",
        tmpm: "Motor temperature",
        uaux: "Auxiliary voltage (i.e. 12V system). Measured on pin 11 (mprot)",
        pwmio: "Raw state of PWM outputs at power up",
        canio: "Digital IO bits received via CAN",
        din_cruise: "Cruise Control. This pin activates the cruise control with the current speed. Pressing again updates the speed set point.", 
        din_start: "State of digital input \"start\". This pin starts inverter operation",
        din_brake: "State of digital input \"brake\". This pin sets maximum regen torque (brknompedal). Cruise control is disabled.",
        din_mprot: "State of digital input \"motor protection switch\". Shuts down the inverter when = 0",
        din_forward: "Direction forward.",
        din_reverse: "Direction backward.",
        din_emcystop: "State of digital input \"emergency stop\". Shuts down the inverter when = 0",
        din_ocur: "Over current detected.",
        din_desat: "",
        din_bms: "BMS over voltage/under voltage.",
        cpuload: "CPU load for everything except communication",
        /* parameters */
        curkp: "Current controller proportional gain",
        curki: "Current controller integral gain",
        curkifrqgain: "Current controllers integral gain frequency coefficient",
        fwkp: "Cross comparison field weakening controller gain",
        dmargin: "Margin for residual torque producing current (so field weakening current doesn't use up the entire amplitude)",
        syncadv: "Shifts \"syncofs\" downwards/upwards with frequency",
        boost: "0 Hz Boost in digit. 1000 digit ~ 2.5%",
        fweak: "Frequency where V/Hz reaches its peak",
        fconst: "Frequency where slip frequency is derated to form a constant power region. Only has an effect when < fweak",
        udcnom: "Nominal voltage for fweak and boost. fweak and boost are scaled to the actual dc voltage. 0=don't scale",
        fslipmin: "Slip frequency at minimum throttle",
        fslipmax: "Slip frequency at maximum throttle",
        fslipconstmax: "Slip frequency at maximum throttle and fconst",
        fmin: "Below this frequency no voltage is generated",
        polepairs: "Pole pairs of motor (e.g. 4-pole motor: 2 pole pairs)",
        respolepairs: "Pole pairs of resolver (normally same as polepairs of motor, but sometimes 1)",
        encflt: "Filter constant between pulse encoder and speed calculation. Makes up for slightly uneven pulse distribution",
        encmode: "0=single channel encoder, 1=quadrature encoder, 2=quadrature /w index pulse, 3=SPI (deprecated), 4=Resolver, 5=sin/cos chip",
        fmax: "At this frequency rev limiting kicks in",
        numimp: "Pulse encoder pulses per turn",
        dirchrpm: "Motor speed at which direction change is allowed",
        dirmode: "0=button (momentary pulse selects forward/reverse), 1=switch (forward or reverse signal must be constantly high)",
        syncofs: "Phase shift of sine wave after receiving index pulse",
        snsm: "Motor temperature sensor. 12=KTY83, 13=KTY84, 14=Leaf, 15=KTY81",
        pwmfrq: "PWM frequency. 0=17.6kHz, 1=8.8kHz, 2=4.4kHz, 3=2.2kHz. Needs PWM restart",
        pwmpol: "PWM polarity. 0=active high, 1=active low. DO NOT PLAY WITH THIS! Needs PWM restart",
        deadtime: "Deadtime between highside and lowside pulse. 28=800ns, 56=1.5µs. Not always linear, consult STM32 manual. Needs PWM restart",
        ocurlim: "Hardware over current limit. RMS-current times sqrt(2) + some slack. Set negative if il1gain and il2gain are negative.",
        minpulse: "Narrowest or widest pulse, all other mapped to full off or full on, respectively",
        il1gain: "Digits per A of current sensor L1",
        il2gain: "Digits per A of current sensor L2",
        udcgain: "Digits per V of DC link",
        udcofs: "DC link 0V offset",
        udclim: "High voltage at which the PWM is shut down",
        snshs: "Heatsink temperature sensor. 0=JCurve, 1=Semikron, 2=MBB600, 3=KTY81, 4=PT1000, 5=NTCK45+2k2, 6=Leaf",
        pinswap: "Swap pins (only \"FOC\" software). Multiple bits can be set. 1=Swap Current Inputs, 2=Swap Resolver sin/cos, 4=Swap PWM output 1/3\n001 = 1 Swap Currents only\n010 = 2 Swap Resolver only\n011 = 3 Swap Resolver and Currents\n100 = 4 Swap PWM only\n101 = 5 Swap PWM and Currents\n110 = 6 Swap PWM and Resolve\n111 = 7 Swap PWM and Resolver and Currents",
        bmslimhigh: "Positive throttle limit on BMS under voltage",
        bmslimlow: "Regen limit on BMS over voltage",
        udcmin: "Minimum battery voltage",
        udcmax: "Maximum battery voltage",
        iacmax: "Maximum peak AC current",
        idcmax: "Maximum DC input current",
        idcmin: "Maximum DC output current (regen)",
        throtmax: "Throttle limit",
        throtmin: "Throttle regen limit",
        ifltrise: "Controls how quickly slip and amplitude recover. The greater the value, the slower",
        ifltfall: "Controls how quickly slip and amplitude are reduced on over current. The greater the value, the slower",
        chargemode: "0=Off, 3=Boost, 4=Buck",
        chargecur: "Charge current setpoint. Boost mode: charger INPUT current. Buck mode: charger output current",
        chargekp: "Charge controller gain. Lower if you have oscillation, raise if current set point is not met",
        chargeflt: "Charge current filtering. Raise if you have oscillations",
        chargemax: "Charge mode duty cycle limit. Especially in boost mode this makes sure you don't overvolt you IGBTs if there is no battery connected.",
        potmin: "Value of \"pot\" when pot isn't pressed at all",
        potmax: "Value of \"pot\" when pot is pushed all the way in",
        pot2min: "Value of \"pot2\" when regen pot is in 0 position",
        pot2max: "Value of \"pot2\" when regen pot is in full on position",
        potmode: "0=Pot 1 is throttle and pot 2 is regen strength preset. 1=Pot 2 is proportional to pot 1 (redundancy) 2=Throttle controlled via CAN",
        throtramp: "Max positive throttle slew rate",
        throtramprpm: "No throttle ramping above this speed",
        ampmin: "Minimum relative sine amplitude (only \"sine\" software)",
        slipstart: "% positive throttle travel at which slip is increased (only \"sine\" software)",
        throtcur: "Motor current per % of throttle travel (only \"FOC\" software)",
        brknompedal: "Foot on brake pedal regen torque",
        brkpedalramp: "Ramp speed when entering regen. E.g. when you set brkmax to 20% and brknompedal to -60% and brkpedalramp to 1, it will take 400ms to arrive at brake force of -60%",
        brknom: "Range of throttle pedal travel allocated to regen",
        brkmax: "Foot-off throttle regen torque",
        brkrampstr: "Below this frequency the regen torque is reduced linearly with the frequency",
        brkout: "Activate brake light output at this amount of braking force",
        idlespeed: "Motor idle speed. Set to -100 to disable idle function. When idle speed controller is enabled, brake pedal must be pressed on start.",
        idlethrotlim: "Throttle limit of idle speed controller",
        idlemode: "Motor idle speed mode. 0=always run idle speed controller, 1=only run it when brake pedal is released, 2=like 1 but only when cruise switch is on",
        speedkp: "Speed controller gain (Cruise and idle speed). Decrease if speed oscillates. Increase for faster load regulation",
        speedflt: "Filter before cruise controller",
        cruisemode: "0=button (set when button pressed, reset with brake pedal), 1=switch (set when switched on, reset when switched off or brake pedal)",
        udcsw: "Voltage at which the DC contactor is allowed to close",
        udcswbuck: "Voltage at which the DC contactor is allowed to close in buck charge mode",
        tripmode: "What to do with relays at a shutdown event. 0=All off, 1=Keep DC switch closed, 2=close precharge relay",
        pwmfunc: "Quantity that controls the PWM output. 0=tmpm, 1=tmphs, 2=speed",
        pwmgain: "Gain of PWM output",
        pwmofs: "Offset of PWM output, 4096=full on",
        canspeed: "Baud rate of CAN interface 0=250k, 1=500k, 2=800k, 3=1M",
        canperiod: "0=send configured CAN messages every 100ms, 1=every 10ms",
        fslipspnt: "Slip setpoint in mode 2. Written by software in mode 1",
        ampnom: "Nominal amplitude in mode 2. Written by software in mode 1",
    },

    get: function(item)
    {
        if ( item in docstrings.data )
        {
            return docstrings.data[item];
        }
        return "";
    }

};
