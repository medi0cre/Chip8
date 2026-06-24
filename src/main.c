#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "raylib.h"

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#include <Windows.h>
#else
#include <dirent.h>
#endif

static const int InstructionsPerSecond = 700;
static const int RefreshRate = 60;
static const int Scale = 16;
static const int Width = Scale * 64;
static const int Height = Scale * 32;

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

#ifdef _WIN32
void PrintRoms()
{
    WIN32_FIND_DATA FindData = { 0 };
    HANDLE FindHandle = FindFirstFile("../../roms/*", &FindData);
    Enforce(FindHandle != INVALID_HANDLE_VALUE, "No roms found!");

    do
    {
        if (strcmp(FindData.cFileName, ".") == 0 || strcmp(FindData.cFileName, "..") == 0) { continue; }
        printf("%s\n", FindData.cFileName);
    }
    while (FindNextFile(FindHandle, &FindData));

    FindClose(FindHandle);
}
#else
void PrintRoms()
{
    DIR* Directory = opendir("../../roms/");
    Enforce(Directory != NULL, "No roms found!");

    struct dirent* File = NULL;
    while ((File = readdir(Directory)) != NULL)
    {
        if (strcmp(File->d_name, ".") == 0 || strcmp(File->d_name, "..") == 0) { continue; }
        printf("%s\n", File->d_name);
    }

    closedir(Directory);
}
#endif

void GetUserInput(Chip8* Emulator)
{
    Emulator->Key[1] = (unsigned char)IsKeyDown(KEY_ONE);
    Emulator->Key[2] = (unsigned char)IsKeyDown(KEY_TWO);
    Emulator->Key[3] = (unsigned char)IsKeyDown(KEY_THREE);
    Emulator->Key[12] = (unsigned char)IsKeyDown(KEY_FOUR);

    Emulator->Key[4] = (unsigned char)IsKeyDown(KEY_Q);
    Emulator->Key[5] = (unsigned char)IsKeyDown(KEY_W);
    Emulator->Key[6] = (unsigned char)IsKeyDown(KEY_E);
    Emulator->Key[13] = (unsigned char)IsKeyDown(KEY_R);

    Emulator->Key[7] = (unsigned char)IsKeyDown(KEY_A);
    Emulator->Key[8] = (unsigned char)IsKeyDown(KEY_S);
    Emulator->Key[9] = (unsigned char)IsKeyDown(KEY_D);
    Emulator->Key[14] = (unsigned char)IsKeyDown(KEY_F);

    Emulator->Key[10] = (unsigned char)IsKeyDown(KEY_Z);
    Emulator->Key[0] = (unsigned char)IsKeyDown(KEY_X);
    Emulator->Key[11] = (unsigned char)IsKeyDown(KEY_C);
    Emulator->Key[15] = (unsigned char)IsKeyDown(KEY_V);
}

void EmulateCycles(Chip8* Emulator)
{
    for (int n = 0; n < InstructionsPerSecond / RefreshRate; n++)
    {
        // Fetch Opcode
        Enforce(Emulator->PC < 4095, "Program counter out of bounds!");
        Emulator->OpCode = Emulator->Memory[Emulator->PC] << 8 | Emulator->Memory[Emulator->PC + 1];
        Emulator->PC += 2;

        // Decode Opcode
        const unsigned short X = (Emulator->OpCode >> 8) & 0x000F;
        const unsigned short Y = (Emulator->OpCode >> 4) & 0x000F;
        const unsigned short N = Emulator->OpCode & 0x000F;
        const unsigned short NN = Emulator->OpCode & 0x00FF;
        const unsigned short NNN = Emulator->OpCode & 0x0FFF;

        switch (Emulator->OpCode & 0xF000)
        {
        case 0x0000:
        {
            switch (NN)
            {
            case 0x00E0:
            {
                memset(Emulator->GFX, 0, sizeof(Emulator->GFX));
                Emulator->DrawFlag = true;
                break;
            }
            case 0x00EE:
            {
                Enforce(Emulator->SP > 0, "Invalid value of stack pointer");
                Emulator->SP--;
                Emulator->PC = Emulator->Stack[Emulator->SP];
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
            Emulator->PC = NNN;
            break;
        }
        case 0x2000:
        {
            Enforce(Emulator->SP < 15, "Invalid value of stack pointer");
            Emulator->Stack[Emulator->SP] = Emulator->PC;
            Emulator->SP++;
            Emulator->PC = NNN;
            break;
        }
        case 0x3000:
        {
            if (Emulator->V[X] == NN) { Emulator->PC += 2; }
            break;
        }
        case 0x4000:
        {
            if (Emulator->V[X] != NN) { Emulator->PC += 2; }
            break;
        }
        case 0x5000:
        {
            Enforce((Emulator->OpCode & 0xF00F) == 0x5000, "Invalid Opcode inside 0x5XXX case");
            if (Emulator->V[X] == Emulator->V[Y]) { Emulator->PC += 2; }
            break;
        }
        case 0x6000:
        {
            Emulator->V[X] = NN;
            break;
        }
        case 0x7000:
        {
            Emulator->V[X] += NN;
            break;
        }
        case 0x8000:
        {
            switch (N)
            {
            case 0x0000:
            {
                Emulator->V[X] = Emulator->V[Y];
                break;
            }
            case 0x0001:
            {
                Emulator->V[X] |= Emulator->V[Y];
                break;
            }
            case 0x0002:
            {
                Emulator->V[X] &= Emulator->V[Y];
                break;
            }
            case 0x0003:
            {
                Emulator->V[X] ^= Emulator->V[Y];
                break;
            }
            case 0x0004:
            {
                if ((int)Emulator->V[X] + (int)Emulator->V[Y] > 255) { Emulator->V[15] = 1; }
                else { Emulator->V[15] = 0; }

                Emulator->V[X] += Emulator->V[Y];
                break;
            }
            case 0x0005:
            {
                if (Emulator->V[X] >= Emulator->V[Y]) { Emulator->V[15] = 1; }
                else { Emulator->V[15] = 0; }

                Emulator->V[X] -= Emulator->V[Y];
                break;
            }
            case 0x0006:
            {
                Emulator->V[15] = Emulator->V[X] & 0x01;
                Emulator->V[X] >>= 1;
                break;
            }
            case 0x0007:
            {
                if (Emulator->V[Y] >= Emulator->V[X]) { Emulator->V[15] = 1; }
                else { Emulator->V[15] = 0; }

                Emulator->V[X] = Emulator->V[Y] - Emulator->V[X];
                break;
            }
            case 0x000E:
            {
                Emulator->V[15] = (Emulator->V[X] >> 7) & 0x01;
                Emulator->V[X] <<= 1;
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
            Enforce((Emulator->OpCode & 0xF00F) == 0x9000, "Invalid opcode inside 0x9000 case");

            if (Emulator->V[X] != Emulator->V[Y]) { Emulator->PC += 2; }
            break;
        }
        case 0xA000:
        {
            Emulator->I = NNN;
            break;
        }
        case 0xB000:
        {
            Emulator->PC = Emulator->V[0] + NNN;
            break;
        }
        case 0xC000:
        {
            Emulator->V[X] = (rand() % 256) & NN;
            break;
        }
        case 0xD000:
        {
            const unsigned short VX = Emulator->V[X] % 64;
            const unsigned short VY = Emulator->V[Y] % 32;
            unsigned short Pixel = 0;
            Emulator->V[15] = 0;

            for (int Yline = 0; Yline < N; Yline++)
            {
                Enforce(Emulator->I + Yline < 4096, "Invalid memory write inside DXYN");
                Pixel = Emulator->Memory[Emulator->I + Yline];
                for (int Xline = 0; Xline < 8; Xline++)
                {
                    if ((Pixel & (0x80 >> Xline)) != 0)
                    {
                        int XIndex = (VX + Xline) % 64;
                        int YIndex = (VY + Yline) % 32;

                        if (Emulator->GFX[XIndex + (YIndex * 64)] == 1) { Emulator->V[15] = 1; }
                        Emulator->GFX[XIndex + (YIndex * 64)] ^= 1;
                    }
                }
            }

            Emulator->DrawFlag = true;
            break;
        }
        case 0xE000:
        {
            switch (NN)
            {
            case 0x009E:
            {
                if (Emulator->Key[Emulator->V[X] & 0x0F] == 1) { Emulator->PC += 2; }
                break;
            }
            case 0x00A1:
            {
                if (Emulator->Key[Emulator->V[X] & 0x0F] == 0) { Emulator->PC += 2; }
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
            switch (NN)
            {
            case 0x0007:
            {
                Emulator->V[X] = Emulator->DelayTimer;
                break;
            }
            case 0x000A:
            {
                bool KeyPress = false;

                for (int i = 0; i < 16; i++)
                {
                    if (Emulator->Key[i] != 0)
                    {
                        Emulator->V[X] = i;
                        KeyPress = true;
                        break;
                    }
                }

                if (!KeyPress) { Emulator->PC -= 2; }
                break;
            }
            case 0x0015:
            {
                Emulator->DelayTimer = Emulator->V[X];
                break;
            }
            case 0x0018:
            {
                Emulator->SoundTimer = Emulator->V[X];
                break;
            }
            case 0x001E:
            {
                Emulator->I += Emulator->V[X];
                break;
            }
            case 0x0029:
            {
                Emulator->I = (Emulator->V[X] & 0x0F) * 5;
                break;
            }
            case 0x0033:
            {
                Enforce(Emulator->I < 4094, "Invalid memory write inside BCD opcode");
                Emulator->Memory[Emulator->I] = Emulator->V[X] / 100;
                Emulator->Memory[Emulator->I + 1] = (Emulator->V[X] / 10) % 10;
                Emulator->Memory[Emulator->I + 2] = Emulator->V[X] % 10;
                break;
            }
            case 0x0055:
            {
                for (int i = 0; i <= X; i++)
                {
                    Enforce(Emulator->I + i < 4096, "Memory overflow");
                    Emulator->Memory[Emulator->I + i] = Emulator->V[i];
                }
                break;
            }
            case 0x0065:
            {
                for (int i = 0; i <= X; i++)
                {
                    Enforce(Emulator->I + i < 4096, "Memory overflow");
                    Emulator->V[i] = Emulator->Memory[Emulator->I + i];
                }
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
    }
}

void Render(Chip8* Emulator)
{
    // Update timers
    if (Emulator->DelayTimer > 0) { Emulator->DelayTimer--; }
    if (Emulator->SoundTimer > 0)
    {
        if (Emulator->SoundTimer == 1)
        {
            // TODO: Implement actual sounds
            printf("Beep!\n");
        }
        Emulator->SoundTimer--;
    }

    // Draw graphics
    if (Emulator->DrawFlag)
    {
        BeginDrawing();
        ClearBackground(BLACK);

        for (int i = 0; i < 64; i++)
        {
            for (int j = 0; j < 32; j++)
            {
                if (Emulator->GFX[64 * j + i] == 1) { DrawRectangle(Scale * i, Scale * j, Scale, Scale, CLITERAL(Color){ 0, 255, 0, 255 }); }
            }
        }

        DrawFPS(Scale / 2, Scale / 2);
        Emulator->DrawFlag = false;
        EndDrawing();
    }
}

int main()
{
    printf("==== Chip 8 Emulator ====\n\nRoms:\n");
    PrintRoms();

    char Rom[32];
    char Path[256];
    printf("\nEnter the name of the rom you want to play: ");
    fgets(Rom, sizeof(Rom), stdin);
    Rom[strcspn(Rom, "\n")] = '\0';
    snprintf(Path, sizeof(Path), "../../roms/%s", Rom);

    // Initialize registers and memory once
    Chip8 Emulator = { 0 };
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
    FILE* Game = fopen(Path, "rb");
    Enforce(Game != NULL, "Failed to load game");
    Enforce(fseek(Game, 0, SEEK_END) == 0, "Failed to go to the end of the file");
    long GameSize = ftell(Game);
    Enforce(GameSize > 0 && GameSize <= 3584, "Game is too big to fit into memory");
    rewind(Game);

    printf("Loaded %s: %ld bytes\n\n", Rom, GameSize);
    unsigned char* Buffer = (unsigned char*)malloc(GameSize);
    Enforce(Buffer != NULL, "Failed to malloc memory for game");
    Enforce(fread(Buffer, 1, GameSize, Game) >= GameSize, "Failed to properly read game into buffer");
    Enforce(fclose(Game) == 0, "Failed to close file properly");

    for (int i = 0; i < GameSize; i++) { Emulator.Memory[i + 512] = Buffer[i]; }

    // Setup graphics and input
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(Width, Height, "Chip8 Emulator");

    // Emulation loop
    while (!WindowShouldClose())
    {
        GetUserInput(&Emulator);
        EmulateCycles(&Emulator);
        Render(&Emulator);
    }

    free(Buffer);
    return 0;
}
