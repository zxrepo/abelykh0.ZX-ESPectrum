/*

 AY-Emulator header file

 */

extern uint8_t ay_register_table[14];
extern uint8_t ay_command[3];

void ay_reset(boolean clear_regs);
void ay_write_register(uint8_t register, uint8_t value);
uint8_t ay_read_register(uint8_t register);
void ay_restore();
