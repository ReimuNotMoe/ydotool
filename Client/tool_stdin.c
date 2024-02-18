#include <string.h>
#include <stdint.h>
#include <termios.h>
#include "ydotool.h"

#define FLAG_UPPERCASE		0x80000000
#define FLAG_CTRL               0x40000000

static const int32_t ascii2keycode_map[128] = {
	// 00 - 0f
	-1,KEY_A|FLAG_CTRL,KEY_B|FLAG_CTRL,KEY_C|FLAG_CTRL,KEY_D|FLAG_CTRL,KEY_E|FLAG_CTRL,KEY_F|FLAG_CTRL,KEY_G|FLAG_CTRL,
	KEY_BACKSPACE,KEY_TAB,KEY_ENTER,KEY_K|FLAG_CTRL,KEY_L|FLAG_CTRL,KEY_M|FLAG_CTRL,KEY_N|FLAG_CTRL,KEY_O|FLAG_CTRL,

	// 10 - 1f
	KEY_P|FLAG_CTRL,KEY_Q|FLAG_CTRL,KEY_R|FLAG_CTRL,KEY_S|FLAG_CTRL,KEY_T|FLAG_CTRL,KEY_U|FLAG_CTRL,KEY_V|FLAG_CTRL,KEY_W|FLAG_CTRL,
	KEY_X|FLAG_CTRL,KEY_Y|FLAG_CTRL,KEY_Z|FLAG_CTRL,KEY_ESC,-1,-1,-1,-1,

	// 20 - 2f
	KEY_SPACE,KEY_1|FLAG_UPPERCASE,KEY_APOSTROPHE|FLAG_UPPERCASE,KEY_3|FLAG_UPPERCASE,KEY_4|FLAG_UPPERCASE,KEY_5|FLAG_UPPERCASE,KEY_7|FLAG_UPPERCASE,KEY_APOSTROPHE,
	KEY_9|FLAG_UPPERCASE,KEY_0|FLAG_UPPERCASE,KEY_8|FLAG_UPPERCASE,KEY_EQUAL|FLAG_UPPERCASE,KEY_COMMA,KEY_MINUS,KEY_DOT,KEY_SLASH,

	// 30 - 3f
	KEY_0,KEY_1,KEY_2,KEY_3,KEY_4,KEY_5,KEY_6,KEY_7,
	KEY_8,KEY_9,KEY_SEMICOLON|FLAG_UPPERCASE,KEY_SEMICOLON,KEY_COMMA|FLAG_UPPERCASE,KEY_EQUAL,KEY_DOT|FLAG_UPPERCASE,KEY_SLASH|FLAG_UPPERCASE,

	// 40 - 4f
	KEY_2|FLAG_UPPERCASE,KEY_A|FLAG_UPPERCASE,KEY_B|FLAG_UPPERCASE,KEY_C|FLAG_UPPERCASE,KEY_D|FLAG_UPPERCASE,KEY_E|FLAG_UPPERCASE,KEY_F|FLAG_UPPERCASE,KEY_G|FLAG_UPPERCASE,
	KEY_H|FLAG_UPPERCASE,KEY_I|FLAG_UPPERCASE,KEY_J|FLAG_UPPERCASE,KEY_K|FLAG_UPPERCASE,KEY_L|FLAG_UPPERCASE,KEY_M|FLAG_UPPERCASE,KEY_N|FLAG_UPPERCASE,KEY_O|FLAG_UPPERCASE,

	// 50 - 5f
	KEY_P|FLAG_UPPERCASE,KEY_Q|FLAG_UPPERCASE,KEY_R|FLAG_UPPERCASE,KEY_S|FLAG_UPPERCASE,KEY_T|FLAG_UPPERCASE,KEY_U|FLAG_UPPERCASE,KEY_V|FLAG_UPPERCASE,KEY_W|FLAG_UPPERCASE,
	KEY_X|FLAG_UPPERCASE,KEY_Y|FLAG_UPPERCASE,KEY_Z|FLAG_UPPERCASE,KEY_LEFTBRACE,KEY_BACKSLASH,KEY_RIGHTBRACE,KEY_6|FLAG_UPPERCASE,KEY_MINUS|FLAG_UPPERCASE,

	// 60 - 6f
	KEY_GRAVE,KEY_A,KEY_B,KEY_C,KEY_D,KEY_E,KEY_F,KEY_G,
	KEY_H,KEY_I,KEY_J,KEY_K,KEY_L,KEY_M,KEY_N,KEY_O,

	// 70 - 7f
	KEY_P,KEY_Q,KEY_R,KEY_S,KEY_T,KEY_U,KEY_V,KEY_W,
	KEY_X,KEY_Y,KEY_Z,KEY_LEFTBRACE|FLAG_UPPERCASE,KEY_BACKSLASH|FLAG_UPPERCASE,KEY_RIGHTBRACE|FLAG_UPPERCASE,KEY_GRAVE|FLAG_UPPERCASE,KEY_BACKSPACE
};

static const int32_t ascii2ctrlcode_map[12] = {
	KEY_UP, KEY_DOWN, KEY_RIGHT, KEY_LEFT, -1, -1, -1, -1, -1, -1, KEY_PAGEUP, KEY_PAGEDOWN
};

static int opt_key_delay_ms = 20;
static int opt_key_hold_ms = 20;
static int opt_next_delay_ms = 0;

static struct termios old_tio;

static void restore_terminal() {
    // Restore the old terminal attributes
    tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
}

static void handle_signal(int sig) {
    if (sig == SIGINT) {
        restore_terminal();
        exit(0);
    }
}

static void configure_terminal() {
    struct termios new_tio;
    
    // Get the current terminal attributes
    tcgetattr(STDIN_FILENO, &old_tio);
    
    // Set the new attributes for the terminal (non-canonical, no echo)
    new_tio = old_tio;
    new_tio.c_lflag &= (~ICANON & ~ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);

    atexit(restore_terminal); // Ensure terminal settings are restored on exit
    signal(SIGINT, handle_signal); // Handle CTRL-C
}

int tool_stdin(int argc, char **argv) {
    configure_terminal(); // Set terminal to raw mode

    printf("Type anything (CTRL-C to exit):\n");

    while (1) {
	char buffer[4] = {0};
	read(STDIN_FILENO, buffer, 3);

	printf("Key code: %d %d %d\n", buffer[0], buffer[1], buffer[2]);

	char c = buffer[0];

        // Convert char to keycode and flags based on the ascii2keycode_map
        int kdef = ascii2keycode_map[(int)c];

	if ((int)buffer[0] == 27 && (int)buffer[1] == 91 && (int)buffer[2] >= 65 && (int)buffer[2] <= 76) {
	    kdef = ascii2ctrlcode_map[(int)buffer[2] - 65];
	}

        if (kdef == -1) continue; // Skip unsupported characters
        printf("  Maps to: %d\n", kdef);

        uint16_t kc = kdef & 0xffff; // Extract keycode
        bool isUppercase = (kdef & FLAG_UPPERCASE) != 0;
	bool isCtrl = (kdef & FLAG_CTRL) != 0;

        // Emit key events
        if (isUppercase) {
	    printf("  Sending shift\n");
            uinput_emit(EV_KEY, KEY_LEFTSHIFT, 1, 1); // Press shift for uppercase
        }
	if (isCtrl) {
	    printf("  Sending ctrl\n");
	    uinput_emit(EV_KEY, KEY_LEFTCTRL, 1, 1); // Press ctrl
        }
        uinput_emit(EV_KEY, kc, 1, 1); // Key down
        usleep(opt_key_hold_ms * 1000); // Hold key
        uinput_emit(EV_KEY, kc, 0, 1); // Key up
	if (isCtrl) {
	    uinput_emit(EV_KEY, KEY_LEFTCTRL, 0, 1); // Release ctrl
	}
        if (isUppercase) {
            uinput_emit(EV_KEY, KEY_LEFTSHIFT, 0, 1); // Release shift for uppercase
        }

        usleep(opt_key_delay_ms * 1000); // Delay between keys
    }

    return 0;
}
