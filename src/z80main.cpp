#include "def/hardware.h"
#include "startup.h"
#include <stdio.h>
#include <string.h>
#include "ZX-ESPectrum.h"

#include "Emulator/Keyboard/PS2Kbd.h"
#include "Emulator/Memory.h"
#include "Emulator/clock.h"
#include "Emulator/z80Input.h"
#include "Emulator/z80main.h"
#include "Emulator/Sound/AY-emulator.h"

#define RAM_AVAILABLE 0xC000

Z80_STATE _zxCpu;

extern byte borderTemp;
extern byte z80ports_in[128];
extern byte tick;

CONTEXT _zxContext;
static uint16_t _attributeCount;
int _total;
int _next_total = 0;
static uint8_t zx_data = 0;
static uint32_t frames = 0;
static uint32_t _ticks = 0;
int cycles_per_step = CalcTStates();

extern "C" {
uint8_t readbyte(uint16_t addr);
uint16_t readword(uint16_t addr);
void writebyte(uint16_t addr, uint8_t data);
void writeword(uint16_t addr, uint16_t data);
uint8_t input(uint8_t portLow, uint8_t portHigh);
void output(uint8_t portLow, uint8_t portHigh, uint8_t data);
}

uint8_t last_register =0;

void zx_setup() {
    _zxContext.readbyte = readbyte;
    _zxContext.readword = readword;
    _zxContext.writeword = writeword;
    _zxContext.writebyte = writebyte;
    _zxContext.input = input;
    _zxContext.output = output;

    zx_reset();
}

void zx_reset() {
    memset(z80ports_in, 0x1F, 128);
    borderTemp = 7;
    bank_latch = 0;
    video_latch = 0;
    rom_latch = 0;
    paging_lock = 0;
    sp3_mode = 0;
    sp3_rom = 0;
    rom_in_use = 0;
    cycles_per_step = CalcTStates();

    Z80Reset(&_zxCpu);
    ay_reset(true);
}

int32_t zx_loop() {
    int32_t result = -1;
    byte tmp_color = 0;

    _total = Z80Emulate(&_zxCpu, cycles_per_step, &_zxContext);
     //Z80Interrupt(&_zxCpu, ula_bus, &_zxContext);
   //Serial.println(_total);

    return result;
}

extern "C" uint8_t readbyte(uint16_t addr) {
    switch (addr) {
    case 0x0000 ... 0x3fff:
        switch (rom_in_use) {
        case 0:
            return rom0[addr];
        case 1:
            return rom1[addr];
        case 2:
            return rom2[addr];
        case 3:
            return rom3[addr];
        }
    case 0x4000 ... 0x7fff:
        return ram5[addr - 0x4000];
        break;
    case 0x8000 ... 0xbfff:
        return ram2[addr - 0x8000];
        break;
    case 0xc000 ... 0xffff:
        switch (bank_latch) {
        case 0:
            return ram0[addr - 0xc000];
            break;
        case 1:
            return ram1[addr - 0xc000];
            break;
        case 2:
            return ram2[addr - 0xc000];
            break;
        case 3:
            return ram3[addr - 0xc000];
            break;
        case 4:
            return ram4[addr - 0xc000];
            break;
        case 5:
            return ram5[addr - 0xc000];
            break;
        case 6:
            return ram6[addr - 0xc000];
            break;
        case 7:
            return ram7[addr - 0xc000];
            break;
        }
        // Serial.printf("Address: %x Returned address %x  Bank: %x\n",addr,addr-0xc000,bank_latch);
        break;
    }
}

extern "C" uint16_t readword(uint16_t addr) { return ((readbyte(addr + 1) << 8) | readbyte(addr)); }

extern "C" void writebyte(uint16_t addr, uint8_t data) {



    switch (addr) {
    case 0x0000 ... 0x3fff:
        return;
    case 0x4000 ... 0x7fff:
        ram5[addr - 0x4000] = data;
        break;
    case 0x8000 ... 0xbfff:
        ram2[addr - 0x8000] = data;
        break;
    case 0xc000 ... 0xffff:
        switch (bank_latch) {
        case 0:
            ram0[addr - 0xc000] = data;
            break;
        case 1:
            ram1[addr - 0xc000] = data;
            break;
        case 2:
            ram2[addr - 0xc000] = data;
            break;
        case 3:
            ram3[addr - 0xc000] = data;
            break;
        case 4:
            ram4[addr - 0xc000] = data;
            break;
        case 5:
            ram5[addr - 0xc000] = data;
            break;
        case 6:
            ram6[addr - 0xc000] = data;
            break;
        case 7:
            ram7[addr - 0xc000] = data;
            break;
        }
        break;
    }
    return;
}

extern "C" void writeword(uint16_t addr, uint16_t data) {
    writebyte(addr, (uint8_t)data);
    writebyte(addr + 1, (uint8_t)(data >> 8));
}

extern "C" uint8_t input(uint8_t portLow, uint8_t portHigh) {
    int16_t kbdarrno = 0;

    if (portLow == 0xFE) {
        // Keyboard

        switch (portHigh) {

        case 0xfe:
            kbdarrno = 0;
            break;
        case 0xfd:
            kbdarrno = 1;
            break;
        case 0xfb:
            kbdarrno = 2;
            break;
        case 0xf7:
            kbdarrno = 3;
            break;
        case 0xef:
            kbdarrno = 4;
            break;
        case 0xdf:
            kbdarrno = 5;
            break;
        case 0xbf:
            kbdarrno = 6;
            break;
        case 0x7f:
            kbdarrno = 7;
            break;

        case 0x00: {
            uint8_t result = z80ports_in[7];
            result &= z80ports_in[6];
            result &= z80ports_in[5];
            result &= z80ports_in[4];
            result &= z80ports_in[3];
            result &= z80ports_in[2];
            result &= z80ports_in[1];
            result &= z80ports_in[0];
            return result;
        }

      }
        bitWrite(z80ports_in[kbdarrno], 6, digitalRead(EAR_PIN));
        return (z80ports_in[kbdarrno]);
    }
    // Kempston
    if (portLow == 0x1F) {
        return z80ports_in[31];
    }
    // Sound (AY-3-8912)

    #ifdef AY_SOUND
    if (portLow == 0xFD) {
        switch (portHigh) {
        case 0xFF:
            // Serial.println("Read AY register");
            #ifdef AY_SOUND
             return ay_read_register(last_register);

            #else
              return (ula_bus);
            #endif
        }
    }
    #endif

    uint8_t data = zx_data;
    data |= (0xe0); /* Set bits 5-7 - as reset above */
    data &= ~0x40;
    //Serial.printf("Floating bus Port %x%x  Data %x\n", portHigh,portLow,data);
    //return data;
    return ula_bus;
}

extern "C" void output(uint8_t portLow, uint8_t portHigh, uint8_t data) {
    uint8_t tmp_data = data;
    switch (portLow) {
    case 0xFE: {


        // border color (no bright colors)
        bitWrite(borderTemp, 0, bitRead(data, 0));
        bitWrite(borderTemp, 1, bitRead(data, 1));
        bitWrite(borderTemp, 2, bitRead(data, 2));


        byte sound = (data & 0x10);

        /*
        if ((z80ports_in[0x20] & 0x10) != sound)
        {
          if (sound)
			       {
				        digitalWrite(SPEAKER_PIN,1);
			       }
			    else
			       {
				        digitalWrite(SPEAKER_PIN,0);

			       }
             //delayMicroseconds(100);
        }
        */
        digitalWrite(SPEAKER_PIN, bitRead(data, 4)); // speaker
        digitalWrite(MIC_PIN, bitRead(data, 3)); // tape

        z80ports_in[0x20] = data;
    } break;

    case 0xFD: {
        // Sound (AY-3-8912)
        switch (portHigh) {

#ifdef AY_SOUND
        case 0xFF:

            last_register = data;
            break;
        case 0xBF:
            //Serial.printf("Select AY register Data %x %x\n",last_register,data);
            ay_write_register(last_register,data);
            break;
#endif

        case 0x7F:
            if (!paging_lock) {
                paging_lock = bitRead(tmp_data, 5);
                rom_latch = bitRead(tmp_data, 4);
                video_latch = bitRead(tmp_data, 3);
                bank_latch = tmp_data & 0x7;
                // rom_in_use=0;
                bitWrite(rom_in_use, 1, sp3_rom);
                bitWrite(rom_in_use, 0, rom_latch);
                // Serial.printf("7FFD data: %x ROM latch: %x Video Latch: %x bank latch: %x page lock:
                // %x\n",tmp_data,rom_latch,video_latch,bank_latch,paging_lock);
            }
            break;

        case 0x1F:
            sp3_mode = bitRead(data, 0);
            sp3_rom = bitRead(data, 2);
            bitWrite(rom_in_use, 1, sp3_rom);
            bitWrite(rom_in_use, 0, rom_latch);

            // Serial.printf("1FFD data: %x mode: %x rom bits: %x ROM chip: %x\n",data,sp3_mode,sp3_rom, rom_in_use);
            break;
        }
    } break;
        // default:
        //    zx_data = data;
        break;
    }

}
