#include "chip8interpreter.h"

#include<iostream>
#include<fstream>
#include<QDateTime>
#include<QString>

#include "utils.h"

Chip8Interpreter::Chip8Interpreter(QObject *parent) : QThread{parent} {
    // copy fontset data
    for (int i = 0; i < 80; i++) {
        RAM[i] = CHIP8FONTSET[i];
    }
    // fill function map
    functionMap = {
            {0x00, [this](std::shared_ptr<Instruction> ins) {
                if (ins->KK == 0xE0) {
                    CLS(ins);
                } else if (ins->KK == 0xEE) {
                    RET(ins);
                }
            }},
            {0x01, [this](std::shared_ptr<Instruction> ins) {
                JP_Addr(ins);
            }},
            {0x02, [this](std::shared_ptr<Instruction> ins) {
                CALL_Addr(ins);
            }},
            {0x03, [this](std::shared_ptr<Instruction> ins) {
                SE_Vx_Byte(ins);
            }},
            {0x04, [this](std::shared_ptr<Instruction> ins) {
                SNE_Vx_Byte(ins);
            }},
            {0x05, [this](std::shared_ptr<Instruction> ins) {
                SE_Vx_Vy(ins);
            }},
            {0x06, [this](std::shared_ptr<Instruction> ins) {
                LD_Vx_Byte(ins);
            }},
            {0x07, [this](std::shared_ptr<Instruction> ins) {
                ADD_Vx_Byte(ins);
            }},
            {0x08, [this](std::shared_ptr<Instruction> ins) {
                switch (ins->N) {
                    case 0x0:
                        LD_Vx_Vy(ins);
                        break;
                    case 0x1:
                        OR_Vx_Vy(ins);
                        break;
                    case 0x2:
                        AND_Vx_Vy(ins);
                        break;
                    case 0x3:
                        XOR_Vx_Vy(ins);
                        break;
                    case 0x4:
                        ADD_Vx_Vy(ins);
                        break;
                    case 0x5:
                        SUB_Vx_Vy(ins);
                        break;
                    case 0x6:
                        SHR_Vx_iVy(ins);
                        break;
                    case 0x7:
                        SUBN_Vx_Vy(ins);
                        break;
                    case 0xE:
                        SHL_Vx_iVy(ins);
                        break;
                    default:
                        break;
                }
            }},
            {0x09, [this](std::shared_ptr<Instruction> ins) {
                SNE_Vx_Vy(ins);
            }},
            {0x0A, [this](std::shared_ptr<Instruction> ins) {
                LD_I_Addr(ins);
            }},
            {0x0B, [this](std::shared_ptr<Instruction> ins) {
                JP_V0_Addr(ins);
            }},
            {0x0C, [this](std::shared_ptr<Instruction> ins) {
                RND_Vx_KK(ins);
            }},
            {0x0D, [this](std::shared_ptr<Instruction> ins) {
                DRW_Vx_Vy_N(ins);
            }},
            {0x0E, [this](std::shared_ptr<Instruction> ins) {
                switch (ins->KK) {
                    case 0x9E:
                        SKP_Vx(ins);
                        break;
                    case 0xA1:
                        SKNP_Vx(ins);
                        break;
                    default:
                        break;
                }
            }},
            {0x0F, [this](std::shared_ptr<Instruction> ins) {
                switch (ins->KK) {
                    case 0x07:
                        LD_Vx_DT(ins);
                        break;
                    case 0x0A:
                        LD_Vx_K(ins);
                        break;
                    case 0x15:
                        LD_DT_Vx(ins);
                        break;
                    case 0x18:
                        LD_ST_Vx(ins);
                        break;
                    case 0x1E:
                        ADD_I_Vx(ins);
                        break;
                    case 0x29:
                        LD_F_Vx(ins);
                        break;
                    case 0x33:
                        LD_B_Vx(ins);
                        break;
                    case 0x55:
                        LD_I_Vx(ins);
                        break;
                    case 0x65:
                        LD_Vx_I(ins);
                        break;
                    default:
                        break;
                }
            }},
    };

    timer = new QTimer();
    connect(timer, &QTimer::timeout, this, &Chip8Interpreter::Tick);
    timer->setInterval(tickInterval);
    timer->start();
}

void Chip8Interpreter::run() {
    exec();
}

bool Chip8Interpreter::Load(const std::string &file) {
    std::ifstream stream(file, std::ios::binary | std::ios::in);
    if (!stream.is_open()) {
        return false;
    }
    char c;
    for (int i = 0x200; stream.get(c); i++) {
        RAM[i] = c;
    }
    stream.close();
    PC = 0x200;
    I = 0x200;
    return true;
}

std::shared_ptr<Instruction> Chip8Interpreter::ParseInstruction(uint16_t opcode) {
    std::shared_ptr<Instruction> ins = std::make_shared<Instruction>();
    ins->NNN = opcode & 0x0FFF;
    ins->KK = opcode & 0x00FF;
    ins->X = (opcode & 0x0F00) >> 8;
    ins->Y = (opcode & 0x00F0) >> 4;
    ins->N = opcode & 0x000F;

    return ins;
}

void Chip8Interpreter::Tick() {
    // read 2 bytes opcode (big endian).
    uint16_t opcode = RAM[PC] << 8 | RAM[PC + 1];

//    std::cout << QString("PC: %1, opcode: %2").arg(ToHex(PC), ToHex(opcode)).toStdString() << std::endl;

    auto ins = ParseInstruction(opcode);
    int op = opcode >> 12;
    if (functionMap.find(op) != functionMap.end()) {
        functionMap[op](ins);
    }

    uint64_t currentTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
    if (currentTime - time > 20) {
        if (DT > 0) {
            DT--;
        }
        if (ST > 0) {
            emit beep(ST * 20);
            ST = 0;
        }
        if (drawFlag) {
            emit draw(BUFFER);
            drawFlag = false;
        }
        time = currentTime;
    }
}

void Chip8Interpreter::Push(uint16_t opcode) {
    // std::cout << QString("Push: %1").arg(ToHex(opcode)).toStdString() << std::endl;
    STACK[SP++] = opcode;
}

uint16_t Chip8Interpreter::Pop() {
    uint16_t opcode = STACK[--SP];
    // std::cout << QString("Pop: %1").arg(ToHex(opcode)).toStdString() << std::endl;
    return opcode;
}

void Chip8Interpreter::KeyDown(int key) {
    INPUTS[key] = true;
}

void Chip8Interpreter::KeyUp(int key) {
    INPUTS[key] = false;
}

void Chip8Interpreter::CLS(std::shared_ptr<Instruction> ins) {
    for (int x = 0; x < screenHeight; x++) {
        for (int y = 0; y < screenWidth; y++) {
            BUFFER[x][y] = false;
        }
    }
    PC += 2;
}

void Chip8Interpreter::RET(std::shared_ptr<Instruction> ins) {
    PC = Pop();
    PC += 2;
}


void Chip8Interpreter::JP_Addr(std::shared_ptr<Instruction> ins) {
    PC = ins->NNN;
}

void Chip8Interpreter::CALL_Addr(std::shared_ptr<Instruction> ins) {
    std::cout << QString("Call Address: %1").arg(ToHex(ins->NNN)).toStdString() << std::endl;
    Push(PC);
    PC = ins->NNN;
}

void Chip8Interpreter::SE_Vx_Byte(std::shared_ptr<Instruction> ins) {
    if (V[ins->X] == ins->KK) {
        PC += 2;
    }
    PC += 2;
}

void Chip8Interpreter::SNE_Vx_Byte(std::shared_ptr<Instruction> ins) {
    if (V[ins->X] != ins->KK) {
        PC += 2;
    }
    PC += 2;
}

void Chip8Interpreter::SE_Vx_Vy(std::shared_ptr<Instruction> ins) {
    if (V[ins->X] == V[ins->Y]) {
        PC += 2;
    }
    PC += 2;
}

void Chip8Interpreter::LD_Vx_Byte(std::shared_ptr<Instruction> ins) {
    V[ins->X] = ins->KK;
    PC += 2;
}

void Chip8Interpreter::ADD_Vx_Byte(std::shared_ptr<Instruction> ins) {
    V[ins->X] += ins->KK;
    PC += 2;
}

void Chip8Interpreter::LD_Vx_Vy(std::shared_ptr<Instruction> ins) {

    V[ins->X] = V[ins->Y];
    PC += 2;
}

void Chip8Interpreter::OR_Vx_Vy(std::shared_ptr<Instruction> ins) {
    V[ins->X] |= V[ins->Y];
    V[0x0F] = 0;
    PC += 2;
}

void Chip8Interpreter::AND_Vx_Vy(std::shared_ptr<Instruction> ins) {
    V[ins->X] &= V[ins->Y];
    V[0x0F] = 0;
    PC += 2;
}

void Chip8Interpreter::XOR_Vx_Vy(std::shared_ptr<Instruction> ins) {
    V[ins->X] ^= V[ins->Y];
    V[0x0F] = 0;
    PC += 2;
}

void Chip8Interpreter::ADD_Vx_Vy(std::shared_ptr<Instruction> ins) {
    if (V[ins->X] > 0xFF - V[ins->Y]) {
        V[0x0F] = 1;
    } else {
        V[0x0F] = 0;
    }

    V[ins->X] += V[ins->Y];
    PC += 2;
}

void Chip8Interpreter::SUB_Vx_Vy(std::shared_ptr<Instruction> ins) {
    if (V[ins->X] < V[ins->Y]) {
        V[0x0F] = 0;
    } else {
        V[0x0F] = 1;
    }

    V[ins->X] -= V[ins->Y];
    PC += 2;
}

void Chip8Interpreter::SHR_Vx_iVy(std::shared_ptr<Instruction> ins) {
    V[0x0F] = V[ins->X] & 0x01;
    V[ins->X] >>= 1;
    PC += 2;
}

// 8xy7
// set Vx = Vy - Vx, set VF = NOT borrow.
// if Vy > Vx, then VF is set to 1, otherwise 0.
// then Vx is subtracted from Vy, and the results stored in Vx.
void Chip8Interpreter::SUBN_Vx_Vy(std::shared_ptr<Instruction> ins) {

    if (V[ins->X] > V[ins->Y]) {
        V[0x0F] = 0;
    } else {
        V[0x0F] = 1;
    }

    V[ins->X] = V[ins->Y] - V[ins->X];
    PC += 2;
}

// 8xyE
// set Vx = Vx SHL 1.
// if the most-significant bit of Vx is 1, then VF is set to 1, otherwise to 0.
// then Vx is multiplied by 2.
void Chip8Interpreter::SHL_Vx_iVy(std::shared_ptr<Instruction> ins) {
    V[0x0F] = V[ins->X] >> 7;
    V[ins->X] <<= 1;
    PC += 2;
}

// 9xy0
// skip next instruction if Vx != Vy.
// the values of Vx and Vy are compared, and if they are not equal, the program counter is increased by 2.
void Chip8Interpreter::SNE_Vx_Vy(std::shared_ptr<Instruction> ins) {
    if (V[ins->X] != V[ins->Y]) {
        PC += 2;
    }
    PC += 2;
}

// Annn
// set I = nnn.
// the value of register I is set to nnn.
void Chip8Interpreter::LD_I_Addr(std::shared_ptr<Instruction> ins) {
    I = ins->NNN;
    PC += 2;
}

// Bnnn
// jump to location nnn + V0.
// the program counter is set to nnn plus the value of V0.
void Chip8Interpreter::JP_V0_Addr(std::shared_ptr<Instruction> ins) {
    PC = (uint16_t) V[0] + ins->NNN;
}

// Cxkk
// set Vx = random byte AND kk.
// the interpreter generates a random number from 0 to 255, which is then ANDed with the value kk.
// the results are stored in Vx.
void Chip8Interpreter::RND_Vx_KK(std::shared_ptr<Instruction> ins) {
    V[ins->X] = RND.next() & ins->KK;
    PC += 2;
}

// DXYN
// Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a height of N pixels.
// Each row of 8 pixels is read as bit-coded starting from memory location I;
// I value doesn’t change after the execution of this instruction.
// VF is set to 1 if any screen pixels are flipped from set to unset when the sprite is drawn, and to 0 if that doesn’t happen
void Chip8Interpreter::DRW_Vx_Vy_N(std::shared_ptr<Instruction> ins) {
    // std::cout << "draw instruction called." << std::endl;
    // for(int i = 0; i < screenHeight; i++) {
    //     for (int j = 0; j < screenWidth; j++) {
    //         std::cout << BUFFER[i][j] << " ";
    //     }
    //     std::cout << std::endl;
    // }
    // std::cout << std::endl;


    int startX = V[ins->X];
    int startY = V[ins->Y];


    V[0x0F] = 0;

    // number of sprites for display
    for (int i = 0; i < ins->N; i++) {
        uint8_t spriteLine = RAM[I + i];
        // fixed 1 byte for each sprite
        for (int j = 0; j < 8; j++) {
            int x = (startY + i) % screenHeight;
            int y = (startX + j) % screenWidth;

            bool currentPix = BUFFER[x][y];
            bool spritePix = spriteLine & (0x80 >> j);

            if (currentPix != spritePix) {
                drawFlag = true;
            }

            if (currentPix && spritePix) {
                V[0xF] = 1;
            }

            BUFFER[x][y] = (bool) (currentPix ^ spritePix);
        }
    }

    PC += 2;

    // std::cout << "draw instruction called." << std::endl;
    // for(int i = 0; i < screenHeight; i++) {
    //     for (int j = 0; j < screenWidth; j++) {
    //         std::cout << BUFFER[i][j] << " ";
    //     }
    //     std::cout << std::endl;
    // }
    // std::cout << std::endl;
    // std::cout << "--------------" << std::endl;
}

void Chip8Interpreter::SKP_Vx(std::shared_ptr<Instruction> ins) {
    if (INPUTS[V[ins->X]]) {
        PC += 2;
    }
    PC += 2;
}

void Chip8Interpreter::SKNP_Vx(std::shared_ptr<Instruction> ins) {
    if (!INPUTS[V[ins->X]]) {
        PC += 2;
    }
    PC += 2;
}

void Chip8Interpreter::LD_Vx_DT(std::shared_ptr<Instruction> ins) {
    V[ins->X] = DT;
    PC += 2;
}

// Fx0A - LD Vx, K
// wait for a key press, store the value of the key in Vx.
// all execution stops until a key is pressed, then the value of that key is stored in Vx.
void Chip8Interpreter::LD_Vx_K(std::shared_ptr<Instruction> ins) {
    bool anyKeyPressed = false;

    for (int i = 0; i < INPUTS.size(); i++) {
        if (INPUTS[i]) {
            anyKeyPressed = true;
            V[ins->X] = i;
            break;
        }
    }

    if (anyKeyPressed) {
        PC += 2;
    }
}

void Chip8Interpreter::LD_DT_Vx(std::shared_ptr<Instruction> ins) {
    DT = V[ins->X];
    PC += 2;
}

void Chip8Interpreter::LD_ST_Vx(std::shared_ptr<Instruction> ins) {
    ST = V[ins->X];
    PC += 2;
}

void Chip8Interpreter::ADD_I_Vx(std::shared_ptr<Instruction> ins) {
    I += V[ins->X];
    PC += 2;
}

void Chip8Interpreter::LD_F_Vx(std::shared_ptr<Instruction> ins) {
    I = V[ins->X] * 0x5;
    PC += 2;
}

void Chip8Interpreter::LD_B_Vx(std::shared_ptr<Instruction> ins) {
    int value = V[ins->X];
    RAM[I] = value % 10;
    value /= 10;
    RAM[I + 1] = value % 10;
    value /= 10;
    RAM[I + 2] = value % 10;

    PC += 2;
}

void Chip8Interpreter::LD_I_Vx(std::shared_ptr<Instruction> ins) {
    for (int i = 0; i <= ins->X; i++) {
        RAM[I + i] = V[i];
    }
    I = I + ins->X + 1;
    PC += 2;
}

void Chip8Interpreter::LD_Vx_I(std::shared_ptr<Instruction> ins) {
    for (int i = 0; i <= ins->X; i++) {
        V[i] = RAM[I + i];
    }
    I = I + ins->X + 1;
    PC += 2;
}
