#ifndef CHIP8INTERPRETER_H
#define CHIP8INTERPRETER_H

#include <string>
#include <memory>
#include <array>
#include <map>
#include <functional>
#include <cstdint>
#include <random>

#include <QThread>
#include <QTimer>

const static uint8_t CHIP8FONTSET[80] =
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

struct Instruction {
    uint16_t opcode;
    uint16_t NNN;
    uint8_t KK, X, Y, N;
};

class RNDRegister {
private:
    std::unique_ptr<std::random_device> seed;
    std::unique_ptr<std::default_random_engine> generator;
    std::unique_ptr<std::uniform_int_distribution<uint8_t>> distribution;

public:
    RNDRegister() {
        seed = std::make_unique<std::random_device>();
        generator = std::make_unique<std::default_random_engine>((*seed)());
        distribution = std::make_unique<std::uniform_int_distribution<uint8_t>>(0, 255);
    }

    uint8_t next() {
        return (*distribution)(*generator);
    }
};

class Chip8Interpreter : public QThread {
Q_OBJECT

public:
    const static int screenWidth{64};
    const static int screenHeight{32};
    const static int stackSize{256};

    // function map
    std::map<uint8_t, std::function<void(std::shared_ptr<Instruction>)>> functionMap;

    // data registers
    std::array<uint8_t, 16> V{};
    // delay timer register
    uint8_t DT{};
    // sound timer register
    uint8_t ST{};
    // address register
    uint16_t I{0x200};
    // program counter
    uint16_t PC{0x200};
    // stack pointer
    uint8_t SP{};
    // stack
    std::array<uint16_t, stackSize> STACK{};
    // memory, 4KB
    std::array<uint8_t, 0x1000> RAM{};
    // random
    RNDRegister RND{};
    // keyboard inputs
    std::array<bool, 16> INPUTS{};
    // screen buffer
    std::array<std::array<bool, screenWidth>, screenHeight> BUFFER{};

signals:

    // coordinate description
    // +----------> y
    // |
    // |
    // |
    // x
    void draw(std::array<std::array<bool, screenWidth>, screenHeight> buffer);

    void beep(int milliseconds);

public slots:

    void KeyDown(int key);

    void KeyUp(int key);

public:
    void run() override;

private:
    uint tickInterval{10};
    QTimer *timer;
    uint64_t time{0};
    bool drawFlag{true};

public:
    explicit Chip8Interpreter(QObject *parent = nullptr);

    bool Load(const std::string& file);

    // translate opcode into Instruction object.
    std::shared_ptr<Instruction> ParseInstruction(uint16_t opcode);

    void Tick();

    void Push(uint16_t opcode);

    uint16_t Pop();

    // implement instructions
    void CLS(std::shared_ptr<Instruction> ins);

    void RET(std::shared_ptr<Instruction> ins);

    void JP_Addr(std::shared_ptr<Instruction> ins);

    void CALL_Addr(std::shared_ptr<Instruction> ins);

    void SE_Vx_Byte(std::shared_ptr<Instruction> ins);

    void SNE_Vx_Byte(std::shared_ptr<Instruction> ins);

    void SE_Vx_Vy(std::shared_ptr<Instruction> ins);

    void LD_Vx_Byte(std::shared_ptr<Instruction> ins);

    void ADD_Vx_Byte(std::shared_ptr<Instruction> ins);

    void LD_Vx_Vy(std::shared_ptr<Instruction> ins);

    void OR_Vx_Vy(std::shared_ptr<Instruction> ins);

    void AND_Vx_Vy(std::shared_ptr<Instruction> ins);

    void XOR_Vx_Vy(std::shared_ptr<Instruction> ins);

    void ADD_Vx_Vy(std::shared_ptr<Instruction> ins);

    void SUB_Vx_Vy(std::shared_ptr<Instruction> ins);

    void SHR_Vx_iVy(std::shared_ptr<Instruction> ins);

    void SUBN_Vx_Vy(std::shared_ptr<Instruction> ins);

    void SHL_Vx_iVy(std::shared_ptr<Instruction> ins);

    void SNE_Vx_Vy(std::shared_ptr<Instruction> ins);

    void LD_I_Addr(std::shared_ptr<Instruction> ins);

    void JP_V0_Addr(std::shared_ptr<Instruction> ins);

    void RND_Vx_KK(std::shared_ptr<Instruction> ins);

    void DRW_Vx_Vy_N(std::shared_ptr<Instruction> ins);

    void SKP_Vx(std::shared_ptr<Instruction> ins);

    void SKNP_Vx(std::shared_ptr<Instruction> ins);

    void LD_Vx_DT(std::shared_ptr<Instruction> ins);

    void LD_Vx_K(std::shared_ptr<Instruction> ins);

    void LD_DT_Vx(std::shared_ptr<Instruction> ins);

    void LD_ST_Vx(std::shared_ptr<Instruction> ins);

    void ADD_I_Vx(std::shared_ptr<Instruction> ins);

    void LD_F_Vx(std::shared_ptr<Instruction> ins);

    void LD_B_Vx(std::shared_ptr<Instruction> ins);

    void LD_I_Vx(std::shared_ptr<Instruction> ins);

    void LD_Vx_I(std::shared_ptr<Instruction> ins);
};

#endif // CHIP8INTERPRETER_H
