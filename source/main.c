#include <3ds.h>
#include <stdio.h>

int main(void)
{
    gfxInitDefault();
    PrintConsole top, bottom;
    consoleInit(GFX_TOP, &top);
    consoleInit(GFX_BOTTOM, &bottom);

    consoleSelect(&top);
    printf("\x1b[10;17H\x1b[36mHELLO, WORLD!\x1b[0m");
    printf("\x1b[14;17HHello, Nicholas!");
    printf("\x1b[18;10HYour 3DS homebrew test is running.");
    printf("\x1b[27;18HVersion 1.0.0");

    consoleSelect(&bottom);
    printf("\x1b[7;8HPress A to test input.");
    printf("\x1b[10;8HA presses: 0");
    printf("\x1b[23;8HPress START to exit.");

    unsigned int presses = 0;
    while (aptMainLoop()) {
        hidScanInput();
        const u32 pressed = hidKeysDown();
        if (pressed & KEY_START) break;
        if (pressed & KEY_A) {
            printf("\x1b[10;8HA presses: %u", ++presses);
        }
        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }

    gfxExit();
    return 0;
}
