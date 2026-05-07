#include <cstdint>
#include <fstream>
#include <chrono>
#include <random>

class Chip8
{
public:
    uint8_t registers[16]{};
    uint8_t memory[4096]{};
    uint16_t index{};
    uint16_t pc{};
    uint16_t stack[16]{};
    uint8_t sp{};
    uint8_t delayTimer{};
    uint8_t soundTimer{};
    uint8_t keypad[16]{};
    uint32_t video[64 * 32]{};
    uint16_t opcode;

    Chip8();
    void loadROM(const char *filename);
    void Cycle();

    std::default_random_engine randGen;
    std::uniform_int_distribution<uint8_t> randByte;

private:

    // 1. The Typedef MUST be first, and spelled correctly!
    typedef void (Chip8::*Chip8Func)();

    Chip8Func table[0xF + 1];
    Chip8Func table0[0xE + 1];
    Chip8Func table8[0xE + 1];
    Chip8Func tableE[0xE + 1];
    Chip8Func tableF[0x65 + 1];

    void Table0();
    void Table8();
    void TableE();
    void TableF();
    void OP_NULL();

    // --- The instructions ---

    void OP_00E0();
    void OP_00EE();
    void OP_1nnn();
    void OP_2nnn();
    void OP_3xkk();
    void OP_4xkk();
    void OP_5xy0();
    void OP_6xkk();
    void OP_7xkk();
    void OP_8xy0();
    void OP_8xy1();
    void OP_8xy2();
    void OP_8xy3();
    void OP_8xy4();
    void OP_8xy5();
    void OP_8xy6();
    void OP_8xy7();
    void OP_8xyE();
    void OP_9xy0();
    void OP_Annn();
    void OP_Bnnn();
    void OP_Cxkk();
    void OP_Dxyn();
    void OP_Ex9E();
    void OP_ExA1();
    void OP_Fx07();
    void OP_Fx0A();
    void OP_Fx15();
    void OP_Fx18();
    void OP_Fx1E();
    void OP_Fx29();
    void OP_Fx33();
    void OP_Fx55();
    void OP_Fx65();
};

const unsigned int START_ADDRESS = 0x200;
const unsigned int FONTSET_SIZE = 80;
const unsigned int FONTSET_START_ADDRESS = 0x50;
const unsigned int VIDEO_WIDTH = 64;
const unsigned int VIDEO_HEIGHT = 32;

uint8_t fontSet[FONTSET_SIZE] =

    {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

Chip8::Chip8()
    : randGen(std::chrono::system_clock::now().time_since_epoch().count())
{

    randByte = std::uniform_int_distribution<uint8_t>(0, 255U);
    for (unsigned int i = 0; i < FONTSET_SIZE; i++)
    {
        memory[FONTSET_START_ADDRESS + i] = fontSet[i];
    }
}

void Chip8::loadROM(const char *filename)
{
    // Open the file as a stream of binary and move the file pointer to the end
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (file.is_open())
    {
        // Get size of file and allocate a buffer to hold the contents

        std::streampos size = file.tellg(); // tellg will return the total number of bytes in the file.
        char *buffer = new char[size];

        // Go back to the beginning of the file and fill the buffer
        file.seekg(0, std::ios::beg);
        file.read(buffer, size);
        file.close();

        for (long i = 0; i < size; i++)
        {
            // Load the ROM contents into the Chip8's memory, starting at 0x200
            memory[START_ADDRESS + i] = buffer[i];
        }

        delete[] buffer;
    }
}

// ===== Chip8 set of instructions =====

// 1. Clear the display instruction 00E0: CLS

void Chip8::OP_00E0()
{
    memset(video, 0, sizeof(video)); // We can simply set the entire video buffer to zeroes.
}

// 2. Return from a subroutine 00EE: RET

void Chip8::OP_00EE()
{
    --sp;
    pc = stack[sp]; // The top of the stack has the address of one instruction past the one that called the subroutine,
                    // so we can put that back into the PC. Note that this overwrites our preemptive pc += 2 earlier.
}

// 3. Jump to location nnn 1nnn: JP addr

void Chip8::OP_1nnn()
{
    // The interpreter sets the program counter to nnn.
    u_int16_t address = opcode & 0x0FFFu;
    // A jump doesn’t remember its origin, so no stack interaction required.
    pc = address;
}

// 4. Call subroutine at nnn. 2nnn - CALL addr

void Chip8::OP_2nnn()
{
    uint16_t address = opcode & 0x0FFFu;
    stack[sp] = pc; // Before the pc jumps it writes its current address at the top of the stack
    ++sp;           // The sp will be increased inorder to not overrides itself later
    pc = address;   // At last we can now put the address number in the pc
}

// 5. Skip next instruction if Vx = kk. 3xkk - SE Vx, byte

void Chip8::OP_3xkk()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; // We are isolating the Vx value and then shift it.
    u_int8_t byte = opcode & 0x00FFu;

    if (registers[Vx] == byte)
    {

        pc += 2;
    }
}

// 6. Skip next instruction if Vx != kk. 4xkk - SNE Vx, byte

void Chip8::OP_4xkk()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    u_int8_t byte = opcode & 0x00FFu;

    if (registers[Vx] != byte)
    {

        pc += 2;
    }
}

// 7. Skip next instruction if Vx = Vy. 5xy0 - SE Vx, Vy

void Chip8::OP_5xy0()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    if (registers[Vx] == registers[Vy])
    {
        pc += 2;
    }
}

// 8. Set Vx = kk. 6xkk - LD Vx, byte

void Chip8::OP_6xkk()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    registers[Vx] = byte;
}

// 9. Set Vx = Vx + kk. 7xkk - ADD Vx, byte

void Chip8::OP_7xkk()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    registers[Vx] += byte;
}

// 10. Set Vx = Vy. 8xy0 - LD Vx, Vy

void Chip8::OP_8xy0()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] = registers[Vy];
}

// 11. Set Vx = Vx OR Vy. 8xy1 - OR Vx, Vy

void Chip8::OP_8xy1()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] |= registers[Vy];
}

// 12. Set Vx = Vx AND Vy. 8xy2 - AND Vx, Vy

void Chip8::OP_8xy2()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] &= registers[Vy];
}

// 13. Set Vx = Vx XOR Vy. 8xy3 - XOR Vx, Vy

void Chip8::OP_8xy3()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] ^= registers[Vy];
}

// 14. Set Vx = Vx + Vy, set VF = carry. 8xy4 - ADD Vx, Vy

void Chip8::OP_8xy4()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x0F00u) >> 4u;

    uint16_t sum = registers[Vx] + registers[Vy];

    // This is an ADD with an overflow flag.
    // If the sum is greater than what can fit into a byte (255), register VF will be set to 1 as a flag.

    if (sum > 255U)
    {
        registers[0xF] = 1;
    }

    else
    {
        registers[0xF] = 0;
    }

    registers[Vx] = sum & 0xFFu;
}

// 15. Set Vx = Vx - Vy, set VF = NOT borrow. 8xy5 - SUB Vx, Vy

void Chip8::OP_8xy5()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    // If Vx > Vy, then VF is set to 1,
    // otherwise 0. Then Vy is subtracted from Vx, and the results stored in Vx.

    if (registers[Vx] > registers[Vy])
    {
        registers[0xF] = 1;
    }
    else
    {
        registers[0xF] = 0;
    }

    registers[Vx] -= registers[Vy];
}

// 16. Set Vx = Vx SHR 1. 8xy6 - SHR Vx

void Chip8::OP_8xy6()
{
    // If the least-significant bit of Vx is 1, then VF is set to 1,
    // otherwise 0. Then Vx is divided by 2.
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    // Save LSB in VF
    registers[0xF] = (registers[Vx] & 0x1u);

    registers[Vx] >>= 1;
}

// 17. Set Vx = Vy - Vx, set VF = NOT borrow. 8xy7 - SUBN Vx, Vy

void Chip8::OP_8xy7()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    // Set Vx = Vy - Vx, set VF = NOT borrow.

    if (registers[Vy] > registers[Vx])
    {
        registers[0xF] = 1;
    }
    else
    {
        registers[0xF] = 0;
    }

    registers[Vx] = registers[Vy] - registers[Vx];
}

// 18. Set Vx = Vx SHL 1.8xyE - SHL Vx {, Vy}

void Chip8::OP_8xyE()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    // Save MSB in VF
    registers[0xF] = (registers[Vx] & 0x80u) >> 7u;

    registers[Vx] <<= 1;
}

// 19. Skip next instruction if Vx != Vy. 9xy0 - SNE Vx, Vy

void Chip8::OP_9xy0()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    if (registers[Vx] != registers[Vy])
    {
        pc += 2;
    }
}

// 20. Set I = nnn. Annn - LD I, addr

void Chip8::OP_Annn()
{
    uint16_t address = opcode & 0x0FFFu;

    index = address;
}

// 21. Jump to location nnn + V0. Bnnn - JP V0, addr

void Chip8::OP_Bnnn()
{
    uint16_t address = opcode & 0x0FFFu;
    pc = registers[0] + address;
}

// 22. Set Vx = random byte AND kk. Cxkk - RND Vx, byte

void Chip8::OP_Cxkk()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    registers[Vx] = randByte(randGen) & byte;
}

// 23. Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision. Dxyn - DRW Vx, Vy, nibble

void Chip8::OP_Dxyn()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;
    uint8_t height = opcode & 0x000Fu;

    uint8_t xPos = registers[Vx] % VIDEO_WIDTH;
    uint8_t yPos = registers[Vy] % VIDEO_HEIGHT;

    registers[0xF] = 0;

    for (unsigned int row = 0; row < height; ++row)
    {
        uint8_t spriteByte = memory[index + row];

        for (unsigned int col = 0; col < 8; ++col)
        {
            uint8_t spritePixel = spriteByte & (0x80u >> col);
            uint32_t *screenPixel = &video[(yPos + row) * VIDEO_WIDTH + (xPos + col)];

            if (spritePixel)
            {
                // Screen pixel also on - collision
                if (*screenPixel == 0xFFFFFFFF)
                {
                    registers[0xF] = 1;
                }

                // Effectively XOR with the sprite pixel
                *screenPixel ^= 0xFFFFFFFF;
            }
        }
    }
}

// 24. Skip next instruction if key with the value of Vx is pressed. Ex9E - SKP Vx

void Chip8::OP_Ex9E()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    uint8_t key = registers[Vx];

    if (keypad[key])
    {
        pc += 2;
    }
}

// 25. Skip next instruction if key with the value of Vx is not pressed. ExA1 - SKNP Vx

void Chip8::OP_ExA1()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    uint8_t key = registers[Vx];

    if (!keypad[key])
    {
        pc += 2;
    }
}

// 26. Set Vx = delay timer value. Fx07 - LD Vx, DT

void Chip8::OP_Fx07()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    registers[Vx] = delayTimer;
}

// 27. Wait for a key press, store the value of the key in Vx. Fx0A - LD Vx, K

void Chip8::OP_Fx0A()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    if (keypad[0])
    {
        registers[Vx] = 0;
    }
    else if (keypad[1])
    {
        registers[Vx] = 1;
    }
    else if (keypad[2])
    {
        registers[Vx] = 2;
    }
    else if (keypad[3])
    {
        registers[Vx] = 3;
    }
    else if (keypad[4])
    {
        registers[Vx] = 4;
    }
    else if (keypad[5])
    {
        registers[Vx] = 5;
    }
    else if (keypad[6])
    {
        registers[Vx] = 6;
    }
    else if (keypad[7])
    {
        registers[Vx] = 7;
    }
    else if (keypad[8])
    {
        registers[Vx] = 8;
    }
    else if (keypad[9])
    {
        registers[Vx] = 9;
    }
    else if (keypad[10])
    {
        registers[Vx] = 10;
    }
    else if (keypad[11])
    {
        registers[Vx] = 11;
    }
    else if (keypad[12])
    {
        registers[Vx] = 12;
    }
    else if (keypad[13])
    {
        registers[Vx] = 13;
    }
    else if (keypad[14])
    {
        registers[Vx] = 14;
    }
    else if (keypad[15])
    {
        registers[Vx] = 15;
    }
    else
    {
        pc -= 2; // The easiest way to “wait” is to decrement the PC by 2 whenever a keypad value is not detected.
                 // This has the effect of running the same instruction repeatedly.
    }
}

// 28. Set delay timer = Vx. Fx15 - LD DT, Vx

void Chip8::OP_Fx15()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    delayTimer = registers[Vx];
}

// 29. Set sound timer = Vx. Fx18 - LD ST, Vx

void Chip8::OP_Fx18()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    soundTimer = registers[Vx];
}

// 30. Set I = I + Vx. Fx1E - ADD I, Vx

void Chip8::OP_Fx1E()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    index += registers[Vx];
}

// 31. Set I = location of sprite for digit Vx. Fx29 - LD F, Vx

void Chip8::OP_Fx29()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t digit = registers[Vx];

    index = FONTSET_START_ADDRESS + (5 * digit);
}

// 32. Store BCD representation of Vx in memory locations I, I+1, and I+2. Fx33 - LD B, Vx

void Chip8::OP_Fx33()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t value = registers[Vx];

    // Ones-place
    memory[index + 2] = value % 10;
    value /= 10;

    // Tens-place
    memory[index + 1] = value % 10;
    value /= 10;

    // Hundreds-place
    memory[index] = value % 10;
}

// 33. Store registers V0 through Vx in memory starting at location I. Fx55 - LD [I], Vx

void Chip8::OP_Fx55()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    for (uint8_t i = 0; i <= Vx; ++i)
    {
        memory[index + i] = registers[i];
    }
}

// 34. Read registers V0 through Vx from memory starting at location I. Fx65 - LD Vx, [I]

void Chip8::OP_Fx65()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    for (uint8_t i = 0; i <= Vx; ++i)
    {
        registers[i] = memory[index + i];
    }
}
