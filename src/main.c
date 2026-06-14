#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "raylib.h"

unsigned char Chip8FontSet[80] =
{
    0xF0, 0x90, 0x90, 0x90, 0xF0,
    0x20, 0x60, 0x20, 0x20, 0x70,
    0xF0, 0x10, 0xF0, 0x80, 0xF0,
    0xF0, 0x10, 0xF0, 0x10, 0xF0,
    0x90, 0x90, 0xF0, 0x10, 0x10,
    0xF0, 0x80, 0xF0, 0x10, 0xF0,
    0xF0, 0x80, 0xF0, 0x90, 0xF0,
    0xF0, 0x10, 0x20, 0x40, 0x40,
    0xF0, 0x90, 0xF0, 0x90, 0xF0,
    0xF0, 0x90, 0xF0, 0x10, 0xF0,
    0xF0, 0x90, 0xF0, 0x90, 0x90,
    0xE0, 0x90, 0xE0, 0x90, 0xE0,
    0xF0, 0x80, 0x80, 0x80, 0xF0,
    0xE0, 0x90, 0x90, 0x90, 0xE0,
    0xF0, 0x80, 0xF0, 0x80, 0xF0,
    0xF0, 0x80, 0xF0, 0x80, 0x80
};

typedef struct Chip8
{
    bool DrawFlag;

    unsigned short OpCode;
    unsigned short I;
    unsigned short PC;
    unsigned short SP;
    unsigned short Stack[16];

    unsigned char DelayTimer;
    unsigned char SoundTimer;
    unsigned char Memory[4096];
    unsigned char V[16];
    unsigned char GFX[64 * 32];
    unsigned char Key[16];
} Chip8;

void Enforce(bool Condition, const char* Message)
{
    if (Condition) { return; }

    printf("%s\n", Message);
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		printf("Please include the name of the game you want to play\n");
		printf("For example: ./Chip8.exe Pong.ch8");
		exit(EXIT_FAILURE);
	}

    Chip8 Emulator = { 0 };

    // Setup graphics and input
    const int Scale = 16;
    const int Width = Scale * 64;
    const int Height = Scale * 32;
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(Width, Height, "Chip8 Emulator");

    // Initialize registers and memory once
    Emulator.PC = 0x200;
    Emulator.I = 0;
    Emulator.OpCode = 0;
    Emulator.SP = 0;
    Emulator.DrawFlag = false;
    srand(time(NULL));

    memset(Emulator.Stack, 0, sizeof(Emulator.Stack));
    memset(Emulator.Memory, 0, sizeof(Emulator.Memory));
    memset(Emulator.V, 0, sizeof(Emulator.V));
    memset(Emulator.GFX, 0, sizeof(Emulator.GFX));
    memset(Emulator.Key, 0, sizeof(Emulator.Key));

    for (int i = 0; i < 80; i++) { Emulator.Memory[i] = Chip8FontSet[i]; }

    Emulator.DelayTimer = 0;
    Emulator.SoundTimer = 0;

    // Load the game
    FILE* Game = fopen(argv[1], "rb");
    Enforce(Game != NULL, "Failed to load game");
    Enforce(fseek(Game, 0, SEEK_END) == 0, "Failed to go to the end of the file");
    long GameSize = ftell(Game);
    Enforce(GameSize <= 3584, "Game is too big to fit into memory");
    rewind(Game);

    printf("Loaded Pong.ch8: %ld bytes\n", GameSize);
    unsigned char* Buffer = (unsigned char*)malloc(GameSize);
    Enforce(Buffer != NULL, "Failed to malloc memory for game");
    Enforce(fread(Buffer, 1, GameSize, Game) >= GameSize, "Failed to properly read game into buffer");
    Enforce(fclose(Game) == 0, "Failed to close file properly");

    for (int i = 0; i < GameSize; i++) { Emulator.Memory[i + 512] = Buffer[i]; }

    // Emulation loop
    while (!WindowShouldClose())
    {
        // Fetch Opcode
        Emulator.OpCode = Emulator.Memory[Emulator.PC] << 8 | Emulator.Memory[Emulator.PC + 1];
        Emulator.PC += 2;

        // Decode Opcode
        switch (Emulator.OpCode & 0xF000)
        {
        case 0x0000:
        {
            switch (Emulator.OpCode & 0x00FF)
            {
            case 0x00E0:
            {
                memset(Emulator.GFX, 0, sizeof(Emulator.GFX));
                Emulator.DrawFlag = true;
                break;
            }
            case 0x00EE:
            {
                Emulator.SP--;
                Enforce(Emulator.SP >= 0 && Emulator.SP < 16, "Invalid value of stack pointer");
                Emulator.PC = Emulator.Stack[Emulator.SP];
                break;
            }
            default:
                Enforce(false, "Unknown OpCode encountered");
                break;
            }
            break;
        }
        case 0x1000:
        {
            Emulator.PC = Emulator.OpCode & 0x0FFF;
            break;
        }
        case 0x2000:
        {
            Emulator.Stack[Emulator.SP] = Emulator.PC;
            Emulator.SP++;
            Enforce(Emulator.SP >= 0 && Emulator.SP < 16, "Invalid value of stack pointer");
            Emulator.PC = Emulator.OpCode & 0x0FFF;
            break;
        }
        case 0x3000:
        {
            unsigned short NN = Emulator.OpCode & 0x00FF;
            unsigned short X = (Emulator.OpCode >> 8) & 0x000F;

            if (Emulator.V[X] == NN) { Emulator.PC += 2; }
            break;
        }
        case 0x4000:
        {
            unsigned short NN = Emulator.OpCode & 0x00FF;
            unsigned short X = (Emulator.OpCode >> 8) & 0x000F;

            if (Emulator.V[X] != NN) { Emulator.PC += 2; }
            break;
        }
        case 0x5000:
        {
            Enforce((Emulator.OpCode & 0xF00F) == 0x5000, "Invalid Opcode inside 0x5XXX case");

            unsigned short X = (Emulator.OpCode >> 8) & 0x000F;
            unsigned short Y = (Emulator.OpCode >> 4) & 0x000F;

            if (Emulator.V[X] == Emulator.V[Y]) { Emulator.PC += 2; }
            break;
        }
        case 0x6000:
        {
            unsigned short NN = Emulator.OpCode & 0x00FF;
            unsigned short X = (Emulator.OpCode >> 8) & 0x000F;

            Emulator.V[X] = NN;
            break;
        }
        case 0x7000:
        {
            unsigned short NN = Emulator.OpCode & 0x00FF;
            unsigned short X = (Emulator.OpCode >> 8) & 0x000F;

            Emulator.V[X] += NN;
            break;
        }
        case 0x8000:
        {
            switch (Emulator.OpCode & 0x000F)
            {
            case 0x0000:
            {
                unsigned short X = (Emulator.OpCode >> 8) & 0x000F;
                unsigned short Y = (Emulator.OpCode >> 4) & 0x000F;

                Emulator.V[X] = Emulator.V[Y];
                break;
            }
            case 0x0001:
            {
                unsigned short X = (Emulator.OpCode >> 8) & 0x000F;
                unsigned short Y = (Emulator.OpCode >> 4) & 0x000F;

                Emulator.V[X] |= Emulator.V[Y];
                break;
            }
            case 0x0002:
            {
                unsigned short X = (Emulator.OpCode >> 8) & 0x000F;
                unsigned short Y = (Emulator.OpCode >> 4) & 0x000F;

                Emulator.V[X] &= Emulator.V[Y];
                break;
            }
            case 0x0003:
            {
                unsigned short X = (Emulator.OpCode >> 8) & 0x000F;
                unsigned short Y = (Emulator.OpCode >> 4) & 0x000F;

                Emulator.V[X] ^= Emulator.V[Y];
                break;
            }
            case 0x0004:
            {
                unsigned short X = (Emulator.OpCode >> 8) & 0x000F;
                unsigned short Y = (Emulator.OpCode >> 4) & 0x000F;

                if ((int)Emulator.V[X] + (int)Emulator.V[Y] >= 256) { Emulator.V[15] = 1; }
                else { Emulator.V[15] = 0; }

                Emulator.V[X] += Emulator.V[Y];
                break;
            }
            case 0x0005:
            {
                unsigned short X = (Emulator.OpCode >> 8) & 0x000F;
                unsigned short Y = (Emulator.OpCode >> 4) & 0x000F;

                if (Emulator.V[X] >= Emulator.V[Y]) { Emulator.V[15] = 1; }
                else { Emulator.V[15] = 0; }

                Emulator.V[X] -= Emulator.V[Y];
                break;
            }
            case 0x0006:
            {
                unsigned short X = (Emulator.OpCode >> 8) & 0x000F;

                Emulator.V[15] = Emulator.V[X] & 0x01;
                Emulator.V[X] >>= 1;
                break;
            }
            case 0x0007:
            {
                unsigned short X = (Emulator.OpCode >> 8) & 0x000F;
                unsigned short Y = (Emulator.OpCode >> 4) & 0x000F;

                if (Emulator.V[Y] >= Emulator.V[X]) { Emulator.V[15] = 1; }
                else { Emulator.V[15] = 0; }

                Emulator.V[X] = Emulator.V[Y] - Emulator.V[X];
                break;
            }
            case 0x000E:
            {
                unsigned short X = (Emulator.OpCode >> 8) & 0x000F;

                Emulator.V[15] = (Emulator.V[X] >> 7) & 0x01;
                Emulator.V[X] <<= 1;
                break;
            }
            default:
                Enforce(false, "Unknown OpCode encountered");
                break;
            }
            break;
        }
        case 0x9000:
        {
            Enforce((Emulator.OpCode & 0xF00F) == 0x9000, "Invalid opcode inside 0x9000 case");
            unsigned short X = (Emulator.OpCode >> 8) & 0x000F;
            unsigned short Y = (Emulator.OpCode >> 4) & 0x000F;

            if (Emulator.V[X] != Emulator.V[Y]) { Emulator.PC += 2; }
            break;
        }
        case 0xA000:
        {
            Emulator.I = Emulator.OpCode & 0x0FFF;
            break;
        }
        case 0xB000:
        {
            unsigned short NNN = Emulator.OpCode & 0x0FFF;
            Emulator.PC = Emulator.V[0] + NNN;
            break;
        }
        case 0xC000:
        {
            unsigned short NN = Emulator.OpCode & 0x00FF;
            unsigned short X = (Emulator.OpCode >> 8) & 0x000F;

            Emulator.V[X] = (rand() % 256) & NN;
            break;
        }
        case 0xD000:
        {
            unsigned short X = Emulator.V[(Emulator.OpCode & 0x0F00) >> 8];
            unsigned short Y = Emulator.V[(Emulator.OpCode & 0x00F0) >> 4];
            unsigned short MaxHeight = Emulator.OpCode & 0x000F;
            unsigned short Pixel = 0;
            Emulator.V[15] = 0;

            for (int Yline = 0; Yline < MaxHeight; Yline++)
            {
                Pixel = Emulator.Memory[Emulator.I + Yline];
                for (int Xline = 0; Xline < 8; Xline++)
                {
                    if ((Pixel & (0x80 >> Xline)) != 0)
                    {
                        if (Emulator.GFX[(X + Xline + ((Y + Yline) * 64))] == 1) { Emulator.V[15] = 1; }
                        Emulator.GFX[X + Xline + ((Y + Yline) * 64)] ^= 1;
                    }
                }
            }

            Emulator.DrawFlag = true;
            break;
        }
        case 0xE000:
        {
            switch (Emulator.OpCode & 0x00FF)
            {
            case 0x009E:
            {
                unsigned short X = (Emulator.OpCode >> 8) & 0x000F;

                if (Emulator.Key[Emulator.V[X] & 0x0F] == 1) { Emulator.PC += 2; }
                break;
            }
            case 0xA1:
            {
                unsigned short X = (Emulator.OpCode >> 8) & 0x000F;

                if (Emulator.Key[Emulator.V[X] & 0x0F] == 0) { Emulator.PC += 2; }
                break;
            }
            default:
                Enforce(false, "Unknown OpCode encountered inside 0xE000 case");
                break;
            }
            break;
        }
        case 0xF000:
        {
            switch (Emulator.OpCode & 0x00FF)
            {
            case 0x0007:
            {
                unsigned short X = (Emulator.OpCode >> 8) & 0x000F;
                Emulator.V[X] = Emulator.DelayTimer;
                break;
            }
            case 0x000A:
            {
                unsigned short X = (Emulator.OpCode >> 8) & 0x000F;
                bool KeyPress = false;

                for (int i = 0; i < 16; i++)
                {
                    if (Emulator.Key[i] != 0)
                    {
                        Emulator.V[X] = i;
                        KeyPress = true;
                        break;
                    }
                }

                if (!KeyPress) { Emulator.PC -= 2; }
                break;
            }
            case 0x0015:
            {
                unsigned short X = (Emulator.OpCode >> 8) & 0x000F;
                Emulator.DelayTimer = Emulator.V[X];
                break;
            }
            case 0x0018:
            {
                unsigned short X = (Emulator.OpCode >> 8) & 0x000F;
                Emulator.SoundTimer = Emulator.V[X];
                break;
            }
            case 0x001E:
            {
                unsigned short X = (Emulator.OpCode >> 8) & 0x000F;
                Emulator.I += Emulator.V[X];
                break;
            }
            case 0x0029:
            {
                unsigned short X = (Emulator.OpCode >> 8) & 0x000F;
                Emulator.I = (Emulator.V[X] & 0x0F) * 5;
                break;
            }
            case 0x0033:
            {
                unsigned short X = (Emulator.OpCode >> 8) & 0x000F;
                Emulator.Memory[Emulator.I] = Emulator.V[X] / 100;
                Emulator.Memory[Emulator.I + 1] = (Emulator.V[X] / 10) % 10;
                Emulator.Memory[Emulator.I + 2] = Emulator.V[X] % 10;
                break;
            }
            case 0x0055:
            {
                unsigned short X = (Emulator.OpCode >> 8) & 0x000F;

                for (int i = 0; i <= X; i++) { Emulator.Memory[Emulator.I + i] = Emulator.V[i]; }
                break;
            }
            case 0x0065:
            {
                unsigned short X = (Emulator.OpCode >> 8) & 0x000F;

                for (int i = 0; i <= X; i++) { Emulator.V[i] = Emulator.Memory[Emulator.I + i]; }
                break;
            }
            default:
                Enforce(false, "Unknown OpCode encountered inside 0xF000 case");
                break;
            }
            break;
        }
        default:
            Enforce(false, "Unknown OpCode encountered");
            break;
        }

        // Update timers
        if (Emulator.DelayTimer > 0) { Emulator.DelayTimer--; }
        if (Emulator.SoundTimer > 0)
        {
            if (Emulator.SoundTimer == 1) { printf("Beep!\n"); }
            Emulator.SoundTimer--;
        }

        // Draw graphics
        if (Emulator.DrawFlag)
        {
            BeginDrawing();
            ClearBackground(BLACK);

            for (int i = 0; i < 64; i++)
            {
                for (int j = 0; j < 32; j++)
                {
                    if (Emulator.GFX[64 * j + i] == 1) { DrawRectangle(Scale * i, Scale * j, Scale, Scale, WHITE); }
                }
            }

            DrawFPS(Scale / 2, Scale / 2);
            EndDrawing();
            Emulator.DrawFlag = false;
        }

        // Store Key state (Press and release)
        Emulator.Key[1] = (unsigned char)IsKeyDown(KEY_KP_1);
        Emulator.Key[2] = (unsigned char)IsKeyDown(KEY_KP_2);
        Emulator.Key[3] = (unsigned char)IsKeyDown(KEY_KP_3);
        Emulator.Key[12] = (unsigned char)IsKeyDown(KEY_KP_4);

        Emulator.Key[4] = (unsigned char)IsKeyDown(KEY_Q);
        Emulator.Key[5] = (unsigned char)IsKeyDown(KEY_W);
        Emulator.Key[6] = (unsigned char)IsKeyDown(KEY_E);
        Emulator.Key[13] = (unsigned char)IsKeyDown(KEY_R);

        Emulator.Key[7] = (unsigned char)IsKeyDown(KEY_A);
        Emulator.Key[8] = (unsigned char)IsKeyDown(KEY_S);
        Emulator.Key[9] = (unsigned char)IsKeyDown(KEY_D);
        Emulator.Key[14] = (unsigned char)IsKeyDown(KEY_F);

        Emulator.Key[10] = (unsigned char)IsKeyDown(KEY_Z);
        Emulator.Key[0] = (unsigned char)IsKeyDown(KEY_X);
        Emulator.Key[11] = (unsigned char)IsKeyDown(KEY_C);
        Emulator.Key[15] = (unsigned char)IsKeyDown(KEY_V);
    }

    free(Buffer);
    return 0;
}
