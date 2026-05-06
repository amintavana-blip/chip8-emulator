#include <cstdint>
#include <fstream>
#include <chrono>
#include <random>

class Chip8
{
public:
    uint8_t registers[16];
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

    std::default_random_engine randGen;
    std::uniform_int_distribution<uint8_t> randByte;

    void loadROM(const char *filename);
    void OP_00E0();
    void OP_00EE();
    void OP_1nnn();
    void OP_2nnn();
    void OP_3xkk();

};

const unsigned int START_ADDRESS = 0x200;
const unsigned int FONTSET_SIZE = 80;
const unsigned int FONTSET_START_ADDRESS = 0x50;

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