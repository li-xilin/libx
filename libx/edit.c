/*
 * linenoise.c -- guerrilla line editing library against the idea that a
 * line editing lib needs to be 20,000 lines of C code.
 *
 * Copyright (c) 2010, Salvatore Sanfilippo <antirez at gmail dot com>
 * Copyright (c) 2010, Pieter Noordhuis <pcnoordhuis at gmail dot com>
 * Copyright (c) 2011, Steve Bennett <steveb at workware dot net dot au>
 * Copyright (c) 2023,2025, Li Xilin <lixilin@gmx.com>
 *
 * ------------------------------------------------------------------------
 *
 * References:
 * - http://invisible-island.net/xterm/ctlseqs/ctlseqs.html
 * - http://www.3waylabs.com/nw/WWW/products/wizcon/vt220.html
 *
 * Bloat:
 * - Completion?
 *
 * Unix/termios
 * ------------
 * List of escape sequences used by this program, we do everything just
 * a few sequences. In order to be so cheap we may have some
 * flickering effect with some slow terminal, but the lesser sequences
 * the more compatible.
 *
 * EL (Erase Line)
 *    Sequence: ESC [ 0 K
 *    Effect: clear from cursor to end of line
 *
 * CUF (CUrsor Forward)
 *    Sequence: ESC [ n C
 *    Effect: moves cursor forward n chars
 *
 * CR (Carriage Return)
 *    Sequence: \r
 *    Effect: moves cursor to column 1
 *
 * The following are used to clear the screen: ESC [ H ESC [ 2 J
 * This is actually composed of two sequences:
 *
 * cursorhome
 *    Sequence: ESC [ H
 *    Effect: moves the cursor to upper left corner
 *
 * ED2 (Clear entire screen)
 *    Sequence: ESC [ 2 J
 *    Effect: clear the whole screen
 *
 * == For highlighting control characters, we also use the following two ==
 * SO (enter StandOut)
 *    Sequence: ESC [ 7 m
 *    Effect: Uses some standout mode such as reverse video
 *
 * SE (Standout End)
 *    Sequence: ESC [ 0 m
 *    Effect: Exit standout mode
 *
 * == Only used if TIOCGWINSZ fails ==
 * DSR/CPR (Report cursor position)
 *    Sequence: ESC [ 6 n
 *    Effect: reports current cursor position as ESC [ NNN ; MMM R
 *
 * == Only used in multiline mode ==
 * CUU (Cursor Up)
 *    Sequence: ESC [ n A
 *    Effect: moves cursor up n chars.
 *
 * CUD (Cursor Down)
 *    Sequence: ESC [ n B
 *    Effect: moves cursor down n chars.
 *
 * win32/console
* -------------
* If __MINGW32__ is defined, the win32 console API is used.
* This could probably be made to work for the msvc compiler too.
* This support based in part on work by Jon Griffiths.
*/

#include "x/edit.h"
#include "x/file.h"
#include "x/unicode.h"
#include "x/string.h"
#include "x/uchar.h"
#include "x/strbuf.h"
#include "x/printf.h"

#ifdef _WIN32 /* Windows platform, either MinGW or Visual Studio (MSVC) */
#include <windows.h>
#include <fcntl.h>
#define USE_WINCONSOLE
#ifdef __MINGW32__
#undef HAVE_UNISTD_H
#define HAVE_UNISTD_H
#endif
#else
#include <termios.h>
#include <sys/ioctl.h>
#include <poll.h>
#define USE_TERMIOS
#undef HAVE_UNISTD_H
#define HAVE_UNISTD_H
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>

#if defined(_WIN32) && !defined(__MINGW32__)
/* Microsoft headers don't like old POSIX names */
#define snprintf _snprintf
#endif

#define LINENOISE_DEFAULT_HISTORY_MAX_LEN 100

/* ctrl('A') -> 0x01 */
#define ctrl(C) ((C) - '@')
/* meta('a') ->  0xe1 */
#define meta(C) ((C) | 0x80)

/* Use -ve numbers here to co-exist with normal unicode chars */
enum {
	SPECIAL_NONE,
	/* don't use -1 here since that indicates error */
	SPECIAL_UP = -20,
	SPECIAL_DOWN = -21,
	SPECIAL_LEFT = -22,
	SPECIAL_RIGHT = -23,
	SPECIAL_DELETE = -24,
	SPECIAL_HOME = -25,
	SPECIAL_END = -26,
	SPECIAL_INSERT = -27,
	SPECIAL_PAGE_UP = -28,
	SPECIAL_PAGE_DOWN = -29,

	/* Some handy names for other special keycodes */
	CHAR_ESCAPE = 27,
	CHAR_DELETE = 127,
};

static int history_max_len = LINENOISE_DEFAULT_HISTORY_MAX_LEN;
static int history_len = 0;
static int history_index = 0;
static x_uchar **history = NULL;

/* Structure to contain the status of the current (being edited) line */
struct current {
	x_strbuf buf; /* Current buffer. Always null terminated */
	int pos;    /* Cursor position, measured in chars */
	int cols;   /* Size of the window, in chars */
	int nrows;  /* How many rows are being used in multiline mode (>= 1) */
	int rpos;   /* The current row containing the cursor - multiline mode only */
	int colsright; /* refresh_line() cached cols for insert_char() optimisation */
	int colsleft;  /* refresh_line() cached cols for remove_char() optimisation */
	const x_uchar *prompt;
	x_strbuf capture; /* capture buffer, or NULL for none. Always null terminated */
	x_strbuf output;  /* used only during refresh_line() - output accumulator */
	bool use_capture, use_output;
#if defined(USE_TERMIOS)
	int fd;     /* Terminal fd */
#elif defined(USE_WINCONSOLE)
	HANDLE outh; /* Console output handle */
	HANDLE inh; /* Console input handle */
	int rows;   /* Screen rows */
	int x;      /* Current column during output */
	int y;      /* Current row */
#define UBUF_MAX_CHARS 132
	WORD ubuf[UBUF_MAX_CHARS + 1];  /* Accumulates utf16 output - one extra for final surrogate pairs */
	int ubuflen;      /* length used in ubuf */
	int ubufcols;     /* how many columns are represented by the chars in ubuf? */
#endif
};

static int fd_read(struct current *current);
static int get_window_size(struct current *current);
static void cursor_down(struct current *current, int n);
static void cursor_up(struct current *current, int n);
static void erase_eol(struct current *current);
static void refresh_line(struct current *current);
static void refresh_line_alt(struct current *current, const x_uchar *prompt, const x_uchar *buf, int cursor_pos);
static void set_cursor_pos(struct current *current, int x);
static void set_output_highlight(struct current *current, const int *props, int nprops);
static void set_current(struct current *current, const x_uchar *str);

static int fd_isatty(struct current *current)
{
#ifdef USE_TERMIOS
	return isatty(current->fd);
#else
	(void)current;
	return 0;
#endif
}

void x_edit_history_free(void) {
	if (history) {
		int j;

		for (j = 0; j < history_len; j++)
			free(history[j]);
		free(history);
		history = NULL;
		history_len = 0;
	}
}

typedef enum {
	EP_START,   /* looking for ESC */
	EP_ESC,     /* looking for [ */
	EP_DIGITS,  /* parsing digits */
	EP_PROPS,   /* parsing digits or semicolons */
	EP_END,     /* ok */
	EP_ERROR,   /* error */
} ep_state_t;

struct esc_parser {
	ep_state_t state;
	int props[5];   /* properties are stored here */
	int maxprops;   /* size of the props[] array */
	int numprops;   /* number of properties found */
	int termchar;   /* terminator char, or 0 for any alpha */
	int current;    /* current (partial) property value */
};

/**
 * Initialise the escape sequence parser at *parser.
 *
 * If termchar is 0 any alpha char terminates ok. Otherwise only the given
 * char terminates successfully.
 * Run the parser state machine with calls to parse_escape_sequence() for each char.
 */
static void init_parse_escape_seq(struct esc_parser *parser, int termchar)
{
	parser->state = EP_START;
	parser->maxprops = x_arrlen(parser->props);
	parser->numprops = 0;
	parser->current = 0;
	parser->termchar = termchar;
}

/**
 * Pass character 'ch' into the state machine to parse:
 *   'ESC' '[' <digits> (';' <digits>)* <termchar>
 *
 * The first character must be ESC.
 * Returns the current state. The state machine is done when it returns either EP_END
 * or EP_ERROR.
 *
 * On EP_END, the "property/attribute" values can be read from parser->props[]
 * of length parser->numprops.
 */
static int parse_escape_sequence(struct esc_parser *parser, int ch)
{
	switch (parser->state) {
		case EP_START:
			parser->state = (ch == '\x1b') ? EP_ESC : EP_ERROR;
			break;
		case EP_ESC:
			parser->state = (ch == '[') ? EP_DIGITS : EP_ERROR;
			break;
		case EP_PROPS:
			if (ch == ';') {
				parser->state = EP_DIGITS;
donedigits:
				if (parser->numprops + 1 < parser->maxprops) {
					parser->props[parser->numprops++] = parser->current;
					parser->current = 0;
				}
				break;
			}
			/* fall through */
		case EP_DIGITS:
			if (ch >= '0' && ch <= '9') {
				parser->current = parser->current * 10 + (ch - '0');
				parser->state = EP_PROPS;
				break;
			}
			/* must be terminator */
			if (parser->termchar != ch) {
				if (parser->termchar != 0 || !((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'))) {
					parser->state = EP_ERROR;
					break;
				}
			}
			parser->state = EP_END;
			goto donedigits;
		case EP_END:
			parser->state = EP_ERROR;
			break;
		case EP_ERROR:
			break;
	}
	return parser->state;
}

#define DEBUG_REFRESHLINE

#ifdef DEBUG_REFRESHLINE
#define DRL(...) x_fprintf(dfh, __VA_ARGS__)
static FILE *dfh;

static void DRL_CHAR(int ch)
{
	if (ch < ' ') {
		DRL(x_u("^%c"), ch + x_u('@'));
	}
	/* TODO
	else if (ch > 127) {
		DRL(x_u("\\u%04x"), ch);
	}
	*/
	else {
		DRL(x_u("%c"), ch);
	}
}
static void DRL_STR(const x_uchar *str)
{
	size_t len = x_ustrlen(str);
	while (*str) {
		uint32_t ch;
		int n = x_ustr_to_ucode(str, len, &ch);
		str += n;
		len -= n;
		DRL_CHAR(ch);
	}
}
#else
#define DRL(...)
#define DRL_CHAR(ch)
#define DRL_STR(str)
#endif

#if defined(USE_WINCONSOLE)
static DWORD orig_consolemode = 0;

static int flush_output(struct current *current);
static void output_new_line(struct current *current);

static void refresh_start(struct current *current)
{
	(void)current;
}

static void refresh_end(struct current *current)
{
	(void)current;
}

static void refresh_start_chars(struct current *current)
{
	assert(current->use_output == false);
	/* We accumulate all output here */
	x_strbuf_init(&current->output);
	current->use_output = true;
	current->ubuflen = 0;
}

static void refresh_new_line(struct current *current)
{
	DRL(x_u("<nl>"));
	output_new_line(current);
}

static void refresh_end_chars(struct current *current)
{
	assert(current->use_output);
	flush_output(current);
	x_strbuf_free(&current->output);
	current->use_output = false;
}

static int enable_raw_mode(struct current *current) {
	DWORD n;
	INPUT_RECORD irec;

	current->outh = GetStdHandle(STD_OUTPUT_HANDLE);
	current->inh = GetStdHandle(STD_INPUT_HANDLE);

	if (!PeekConsoleInput(current->inh, &irec, 1, &n)) {
		return -1;
	}
	if (get_window_size(current) != 0) {
		return -1;
	}
	if (GetConsoleMode(current->inh, &orig_consolemode)) {
		SetConsoleMode(current->inh, ENABLE_PROCESSED_INPUT);
	}
	return 0;
}

static void disable_raw_mode(struct current *current)
{
	SetConsoleMode(current->inh, orig_consolemode);
}

void x_edit_clear_screen(void)
{
	/* XXX: This is ugly. Should just have the caller pass a handle */
	struct current current;

	current.outh = GetStdHandle(STD_OUTPUT_HANDLE);

	if (get_window_size(&current) == 0) {
		COORD topleft = { 0, 0 };
		DWORD n;

		FillConsoleOutputCharacter(current.outh, ' ',
				current.cols * current.rows, topleft, &n);
		FillConsoleOutputAttribute(current.outh,
				FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN,
				current.cols * current.rows, topleft, &n);
		SetConsoleCursorPosition(current.outh, topleft);
	}
}

static void cursor_to_left(struct current *current)
{
	COORD pos;
	DWORD n;

	pos.X = 0;
	pos.Y = (SHORT)current->y;

	FillConsoleOutputAttribute(current->outh,
			FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN, current->cols, pos, &n);
	current->x = 0;
}

static void flush_ubuf(struct current *current)
{
	COORD pos;
	DWORD nwritten;
	pos.Y = (SHORT)current->y;
	pos.X = (SHORT)current->x;
	SetConsoleCursorPosition(current->outh, pos);
	WriteConsoleW(current->outh, current->ubuf, current->ubuflen, &nwritten, 0);
	current->x += current->ubufcols;
	current->ubuflen = 0;
	current->ubufcols = 0;
}

static void add_ubuf(struct current *current, int ch)
{
	/* This code originally by: Author: Mark E. Davis, 1994. */
	static const int halfShift  = 10; /* used for shifting by 10 bits */

	static const DWORD halfBase = 0x0010000UL;
	static const DWORD halfMask = 0x3FFUL;

#define UNI_SUR_HIGH_START  0xD800
#define UNI_SUR_HIGH_END    0xDBFF
#define UNI_SUR_LOW_START   0xDC00
#define UNI_SUR_LOW_END     0xDFFF

#define UNI_MAX_BMP 0x0000FFFF

	if (ch > UNI_MAX_BMP) {
		/* convert from unicode to utf16 surrogate pairs
		 * There is always space for one extra word in ubuf
		 */
		ch -= halfBase;
		current->ubuf[current->ubuflen++] = (WORD)((ch >> halfShift) + UNI_SUR_HIGH_START);
		current->ubuf[current->ubuflen++] = (WORD)((ch & halfMask) + UNI_SUR_LOW_START);
	}
	else {
		current->ubuf[current->ubuflen++] = ch;
	}
	current->ubufcols += x_ucode_width(ch);
	if (current->ubuflen >= UBUF_MAX_CHARS) {
		flush_ubuf(current);
	}
}

static int flush_output(struct current *current)
{
	const uint16_t *pt = x_strbuf_str(&current->output);
	int len = x_strbuf_len(&current->output);

	/* convert utf8 in current->output into utf16 in current->ubuf
	*/
	while (len) {
		uint32_t ch;
		int n = x_utf16_to_ucode(pt, len, &ch);

		pt += n;
		len -= n;

		add_ubuf(current, ch);
	}
	flush_ubuf(current);

	x_strbuf_clear(&current->output);

	return 0;
}

static int output_chars(struct current *current, const uint16_t *buf, int len)
{
	if (len < 0) {
		len = wcslen(buf);
	}
	assert(current->use_output);

	x_strbuf_append_len(&current->output, buf, len);

	return 0;
}

static void output_new_line(struct current *current)
{
	/* On the last row output a newline to force a scroll */
	if (current->y + 1 == current->rows) {
		output_chars(current, L"\n", 1);
	}
	flush_output(current);
	current->x = 0;
	current->y++;
}

static void set_output_highlight(struct current *current, const int *props, int nprops)
{
	int colour = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN;
	int bold = 0;
	int reverse = 0;
	int i;

	for (i = 0; i < nprops; i++) {
		switch (props[i]) {
			case 0:
				colour = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN;
				bold = 0;
				reverse = 0;
				break;
			case 1:
				bold = FOREGROUND_INTENSITY;
				break;
			case 7:
				reverse = 1;
				break;
			case 30:
				colour = 0;
				break;
			case 31:
				colour = FOREGROUND_RED;
				break;
			case 32:
				colour = FOREGROUND_GREEN;
				break;
			case 33:
				colour = FOREGROUND_RED | FOREGROUND_GREEN;
				break;
			case 34:
				colour = FOREGROUND_BLUE;
				break;
			case 35:
				colour = FOREGROUND_RED | FOREGROUND_BLUE;
				break;
			case 36:
				colour = FOREGROUND_BLUE | FOREGROUND_GREEN;
				break;
			case 37:
				colour = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN;
				break;
		}
	}

	flush_output(current);

	if (reverse) {
		SetConsoleTextAttribute(current->outh, BACKGROUND_INTENSITY);
	}
	else {
		SetConsoleTextAttribute(current->outh, colour | bold);
	}
}

static void erase_eol(struct current *current)
{
	COORD pos;
	DWORD n;

	pos.X = (SHORT) current->x;
	pos.Y = (SHORT) current->y;

	FillConsoleOutputCharacter(current->outh, ' ', current->cols - current->x, pos, &n);
}

static void set_cursor_xy(struct current *current)
{
	COORD pos;

	pos.X = (SHORT) current->x;
	pos.Y = (SHORT) current->y;

	SetConsoleCursorPosition(current->outh, pos);
}


static void set_cursor_pos(struct current *current, int x)
{
	current->x = x;
	set_cursor_xy(current);
}

static void cursor_up(struct current *current, int n)
{
	current->y -= n;
	set_cursor_xy(current);
}

static void cursor_down(struct current *current, int n)
{
	current->y += n;
	set_cursor_xy(current);
}

static int fd_read(struct current *current)
{
	while (1) {
		INPUT_RECORD irec;
		DWORD n;
		if (WaitForSingleObject(current->inh, INFINITE) != WAIT_OBJECT_0) {
			break;
		}
		if (!ReadConsoleInputW(current->inh, &irec, 1, &n)) {
			break;
		}
		if (irec.EventType == KEY_EVENT) {
			KEY_EVENT_RECORD *k = &irec.Event.KeyEvent;
			if (k->bKeyDown || k->wVirtualKeyCode == VK_MENU) {
				if (k->dwControlKeyState & ENHANCED_KEY) {
					switch (k->wVirtualKeyCode) {
						case VK_LEFT:
							return SPECIAL_LEFT;
						case VK_RIGHT:
							return SPECIAL_RIGHT;
						case VK_UP:
							return SPECIAL_UP;
						case VK_DOWN:
							return SPECIAL_DOWN;
						case VK_INSERT:
							return SPECIAL_INSERT;
						case VK_DELETE:
							return SPECIAL_DELETE;
						case VK_HOME:
							return SPECIAL_HOME;
						case VK_END:
							return SPECIAL_END;
						case VK_PRIOR:
							return SPECIAL_PAGE_UP;
						case VK_NEXT:
							return SPECIAL_PAGE_DOWN;
					}
				}
				/* Note that control characters are already translated in AsciiChar */
				else if (k->wVirtualKeyCode == VK_CONTROL)
					continue;
				else {
					// WCHAR buf[2] = { k->uChar.UnicodeChar };
					// MessageBoxW(0, buf, 0, 0);

					return k->uChar.UnicodeChar;
				}
			}
		}
	}
	return -1;
}

static int get_window_size(struct current *current)
{
	CONSOLE_SCREEN_BUFFER_INFO info;
	if (!GetConsoleScreenBufferInfo(current->outh, &info)) {
		return -1;
	}
	current->cols = info.dwSize.X;
	current->rows = info.dwSize.Y;
	if (current->cols <= 0 || current->rows <= 0) {
		current->cols = 80;
		return -1;
	}
	current->y = info.dwCursorPosition.Y;
	current->x = info.dwCursorPosition.X;
	return 0;
}
#endif

#if defined(USE_TERMIOS)
static void edit_at_exit(void);
static struct termios orig_termios; /* in order to restore at exit */
static int rawmode = 0; /* for atexit() function to check if restore is needed*/
static int atexit_registered = 0; /* register atexit just 1 time */

static const char *unsupported_term[] = {"dumb","cons25","emacs",NULL};

static int is_unsupported_term(void) {
	char *term = getenv("TERM");
	if (term) {
		int j;
		for (j = 0; unsupported_term[j]; j++) {
			if (strcmp(term, unsupported_term[j]) == 0) {
				return 1;
			}
		}
	}
	return 0;
}

static int enable_raw_mode(struct current *current)
{
	struct termios raw;
	current->fd = STDIN_FILENO;
	current->cols = 0;
	if (!isatty(current->fd) || is_unsupported_term() ||
			tcgetattr(current->fd, &orig_termios) == -1) {
fatal:
		errno = ENOTTY;
		return -1;
	}
	if (!atexit_registered) {
		atexit(edit_at_exit);
		atexit_registered = 1;
	}
	raw = orig_termios;  /* modify the original mode */
	/* input modes: no break, no CR to NL, no parity check, no strip char,
	 * no start/stop output control. */
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	/* output modes - actually, no need to disable post processing */
	/*raw.c_oflag &= ~(OPOST);*/
	/* control modes - set 8 bit chars */
	raw.c_cflag |= (CS8);
	/* local modes - choing off, canonical off, no extended functions,
	 * no signal chars (^Z,^C) */
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	/* control chars - set return condition: min number of bytes and timer.
	 * We want read to return every single byte, without timeout. */
	raw.c_cc[VMIN] = 1; raw.c_cc[VTIME] = 0; /* 1 byte, no timer */

	/* put terminal in raw mode after flushing */
	if (tcsetattr(current->fd,TCSADRAIN,&raw) < 0) {
		goto fatal;
	}
	rawmode = 1;
	return 0;
}

static void disable_raw_mode(struct current *current) {
	/* Don't even check the return value as it's too late. */
	if (rawmode && tcsetattr(current->fd,TCSADRAIN,&orig_termios) != -1)
		rawmode = 0;
}

/* At exit we'll try to fix the terminal to the initial conditions. */
static void edit_at_exit(void) {
	if (rawmode) {
		tcsetattr(STDIN_FILENO, TCSADRAIN, &orig_termios);
	}
	x_edit_history_free();
}

/* gcc/glibc insists that we care about the return code of write!
 * Clarification: This means that a void-cast like "(void) (EXPR)"
 * does not work.
 */
#define IGNORE_RC(EXPR) if (EXPR) {}

/**
 * Output bytes directly, or accumulate output (if current->output is set)
 */
static void output_chars(struct current *current, const x_uchar *buf, int len)
{
	if (len < 0) {
		len = x_ustrlen(buf);
	}
	if (current->use_output) {
		x_strbuf_append_len(&current->output, buf, len);
	}
	else {
		IGNORE_RC(write(current->fd, buf, len));
	}
}

/* Like output_chars, but using printf-style formatting
*/
static void output_formated(struct current *current, const x_uchar *format, ...)
{
	va_list args;
	x_uchar buf[64];
	int n;
	va_start(args, format);
	n = vsnprintf(buf, x_arrlen(buf), format, args);
	/* This will never happen because we are sure to use output_formated() only for short sequences */
	assert(n < x_arrlen(buf));
	va_end(args);
	output_chars(current, buf, n);
}

static void cursor_to_left(struct current *current)
{
	output_chars(current, "\r", -1);
}

static void set_output_highlight(struct current *current, const int *props, int nprops)
{
	output_chars(current, "\x1b[", -1);
	while (nprops--) {
		output_formated(current, "%d%c", *props, (nprops == 0) ? 'm' : ';');
		props++;
	}
}

static void erase_eol(struct current *current)
{
	output_chars(current, "\x1b[0K", -1);
}

static void set_cursor_pos(struct current *current, int x)
{
	if (x == 0) {
		cursor_to_left(current);
	}
	else {
		output_formated(current, "\r\x1b[%dC", x);
	}
}

static void cursor_up(struct current *current, int n)
{
	if (n) {
		output_formated(current, "\x1b[%dA", n);
	}
}

static void cursor_down(struct current *current, int n)
{
	if (n) {
		output_formated(current, "\x1b[%dB", n);
	}
}

void x_edit_clear_screen(void)
{
	IGNORE_RC(write(STDOUT_FILENO, "\x1b[H\x1b[2J", 7));
}

/**
 * Reads a char from 'fd', waiting at most 'timeout' milliseconds.
 *
 * A timeout of -1 means to wait forever.
 *
 * Returns -1 if no char is received within the time or an error occurs.
 */
static int fd_read_char(int fd, int timeout)
{
	struct pollfd p;
	unsigned char c;

	p.fd = fd;
	p.events = POLLIN;

	if (poll(&p, 1, timeout) == 0) {
		/* timeout */
		return -1;
	}
	if (read(fd, &c, 1) != 1) {
		return -1;
	}
	return c;
}

/**
 * Reads a complete utf-8 character
 * and returns the unicode value, or -1 on error.
 */
static int fd_read(struct current *current)
{
	x_uchar buf[MAX_UTF8_LEN];
	int n, i;
	uint32_t c;

	if (read(current->fd, &buf[0], 1) != 1) {
		return -1;
	}
	n = x_utf8_charlen(buf[0]);
	if (n < 1) {
		return -1;
	}
	for (i = 1; i < n; i++) {
		if (read(current->fd, &buf[i], 1) != 1) {
			return -1;
		}
	}
	/* decode and return the character */
	x_ustr_to_ucode(buf, n, &c);
	return c;
}

/**
 * Stores the current cursor column in '*cols'.
 * Returns 1 if OK, or 0 if failed to determine cursor pos.
 */
static int query_cursor(struct current *current, int* cols)
{
	struct esc_parser parser;
	int ch;

	/* Should not be buffering this output, it needs to go immediately */
	assert(current->use_output == false);

	/* control sequence - report cursor location */
	output_chars(current, "\x1b[6n", -1);

	/* Parse the response: ESC [ rows ; cols R */
	init_parse_escape_seq(&parser, 'R');
	while ((ch = fd_read_char(current->fd, 100)) > 0) {
		switch (parse_escape_sequence(&parser, ch)) {
			default:
				continue;
			case EP_END:
				if (parser.numprops == 2 && parser.props[1] < 1000) {
					*cols = parser.props[1];
					return 1;
				}
				break;
			case EP_ERROR:
				break;
		}
		/* failed */
		break;
	}
	return 0;
}

/**
 * Updates current->cols with the current window size (width)
 */
static int get_window_size(struct current *current)
{
	struct winsize ws;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col != 0) {
		current->cols = ws.ws_col;
		return 0;
	}

	/* Failed to query the window size. Perhaps we are on a serial terminal.
	 * Try to query the width by sending the cursor as far to the right
	 * and reading back the cursor position.
	 * Note that this is only done once per call to linenoise rather than
	 * every time the line is refreshed for efficiency reasons.
	 *
	 * In more detail, we:
	 * (a) request current cursor position,
	 * (b) move cursor far right,
	 * (c) request cursor position again,
	 * (d) at last move back to the old position.
	 * This gives us the width without messing with the externally
	 * visible cursor position.
	 */

	if (current->cols == 0) {
		int here;

		/* If anything fails => default 80 */
		current->cols = 80;

		/* (a) */
		if (query_cursor (current, &here)) {
			/* (b) */
			set_cursor_pos(current, 999);

			/* (c). Note: If (a) succeeded, then (c) should as well.
			 * For paranoia we still check and have a fallback action
			 * for (d) in case of failure..
			 */
			if (query_cursor(current, &current->cols)) {
				/* (d) Reset the cursor back to the original location. */
				if (current->cols > here) {
					set_cursor_pos(current, here);
				}
			}
		}
	}

	return 0;
}

/**
 * If CHAR_ESCAPE was received, reads subsequent
 * chars to determine if this is a known special key.
 *
 * Returns SPECIAL_NONE if unrecognised, or -1 if EOF.
 *
 * If no additional char is received within a short time,
 * CHAR_ESCAPE is returned.
 */
static int check_special(int fd)
{
	int c = fd_read_char(fd, 50);
	int c2;

	if (c < 0) {
		return CHAR_ESCAPE;
	}
	else if (c >= 'a' && c <= 'z') {
		/* esc-a => meta-a */
		return meta(c);
	}

	c2 = fd_read_char(fd, 50);
	if (c2 < 0) {
		return c2;
	}
	if (c == '[' || c == 'O') {
		/* Potential arrow key */
		switch (c2) {
			case 'A':
				return SPECIAL_UP;
			case 'B':
				return SPECIAL_DOWN;
			case 'C':
				return SPECIAL_RIGHT;
			case 'D':
				return SPECIAL_LEFT;
			case 'F':
				return SPECIAL_END;
			case 'H':
				return SPECIAL_HOME;
		}
	}
	if (c == '[' && c2 >= '1' && c2 <= '8') {
		/* extended escape */
		c = fd_read_char(fd, 50);
		if (c == '~') {
			switch (c2) {
				case '2':
					return SPECIAL_INSERT;
				case '3':
					return SPECIAL_DELETE;
				case '5':
					return SPECIAL_PAGE_UP;
				case '6':
					return SPECIAL_PAGE_DOWN;
				case '7':
					return SPECIAL_HOME;
				case '8':
					return SPECIAL_END;
			}
		}
		while (c != -1 && c != '~') {
			/* .e.g \e[12~ or '\e[11;2~   discard the complete sequence */
			c = fd_read_char(fd, 50);
		}
	}

	return SPECIAL_NONE;
}
#endif

static void clear_output_hightlight(struct current *current)
{
	int nohighlight = 0;
	set_output_highlight(current, &nohighlight, 1);
}

static void output_control_char(struct current *current, x_uchar ch)
{
	int reverse = 7;
	set_output_highlight(current, &reverse, 1);
	output_chars(current, x_u("^"), 1);
	output_chars(current, &ch, 1);
	clear_output_hightlight(current);
}

/**
 * Returns the unicode character at the given offset,
 * or -1 if none.
 */
static int get_char(struct current *current, int pos)
{
	if (pos < 0 || pos >= x_strbuf_chars(&current->buf))
		return -1;
	uint32_t c;
	int i = x_ustr_index(x_strbuf_str(&current->buf), pos);
	if (x_ustr_to_ucode(x_strbuf_str(&current->buf) + i, x_strbuf_chars(&current->buf) - i, &c) == 0)
		return -1;
	return c;
}

static int char_display_width(int ch)
{
	if (ch < ' ') {
		/* control chars take two positions */
		return 2;
	}
	else {
		return x_ucode_width(ch);
	}
}

#ifndef NO_COMPLETION
static x_edit_completion_cb *completionCallback = NULL;
static void *completionUserdata = NULL;
static int showhints = 1;
static x_edit_hints_cb *hints_cb = NULL;
static x_edit_free_hints_cb *free_hints_cb = NULL;
static void *hints_arg = NULL;

static void beep(void)
{
#ifdef USE_TERMIOS
	fprintf(stderr, "\x7");
	fflush(stderr);
#endif
}

static void freeCompletions(x_edit_completions *lc) {
	size_t i;
	for (i = 0; i < lc->len; i++)
		free(lc->cvec[i]);
	free(lc->cvec);
}

static int completeLine(struct current *current) {
	x_edit_completions lc = { 0, NULL };
	int c = 0;

	completionCallback(x_strbuf_str(&current->buf),&lc,completionUserdata);
	if (lc.len == 0) {
		beep();
	} else {
		size_t stop = 0, i = 0;

		while(!stop) {
			/* Show completion or original buffer */
			if (i < lc.len) {
				int chars = x_ustr_charcnt(lc.cvec[i], -1);
				refresh_line_alt(current, current->prompt, lc.cvec[i], chars);
			} else {
				refresh_line(current);
			}

			c = fd_read(current);
			if (c == -1) {
				break;
			}

			switch(c) {
				case '\t': /* tab */
					i = (i+1) % (lc.len+1);
					if (i == lc.len) beep();
					break;
				case CHAR_ESCAPE: /* escape */
					/* Re-show original buffer */
					if (i < lc.len) {
						refresh_line(current);
					}
					stop = 1;
					break;
				default:
					/* Update buffer and return */
					if (i < lc.len) {
						set_current(current,lc.cvec[i]);
					}
					stop = 1;
					break;
			}
		}
	}

	freeCompletions(&lc);
	return c; /* Return last read character */
}

/* Register a callback function to be called for tab-completion.
   Returns the prior callback so that the caller may (if needed)
   restore it when done. */
x_edit_completion_cb * x_edit_set_completion_cb(x_edit_completion_cb *fn, void *arg)
{
	x_edit_completion_cb * old = completionCallback;
	completionCallback = fn;
	completionUserdata = arg;
	return old;
}

void x_edit_add_completion(x_edit_completions *lc, const x_uchar *str) {
	lc->cvec = (x_uchar **)realloc(lc->cvec, sizeof(x_uchar *) * (lc->len+1));
	lc->cvec[lc->len++] = x_ustrdup(str);
}

void x_edit_set_hints_cb(x_edit_hints_cb *callback, void *arg)
{
	hints_cb = callback;
	hints_arg = arg;
}

void x_edit_set_free_hints_cb(x_edit_free_hints_cb *callback)
{
	free_hints_cb = callback;
}

#endif


static const x_uchar *reduce_single_buf(const x_uchar *buf, int availcols, int *cursor_pos)
{
	/* We have availcols columns available.
	 * If necessary, strip chars off the front of buf until *cursor_pos
	 * fits within availcols
	 */
	int needcols = 0;
	int pos = 0;
	int new_cursor_pos = *cursor_pos;
	const x_uchar *pt = buf;

	DRL(x_u("reduce_single_buf: availcols=%d, cursor_pos=%d\n"), availcols, *cursor_pos);

	size_t buf_len = x_ustrlen(buf);
	while (*pt) {
		uint32_t ch;
		int n = x_ustr_to_ucode(pt, buf_len, &ch);
		pt += n;
		buf_len -= n;

		needcols += char_display_width(ch);

		/* If we need too many cols, strip
		 * chars off the front of buf to make it fit.
		 * We keep 3 extra cols to the right of the cursor.
		 * 2 for possible wide chars, 1 for the last column that
		 * can't be used.
		 */
		while (needcols >= availcols - 3) {
			n = x_ustr_to_ucode(buf, buf_len -= n, &ch);
			buf += n;
			needcols -= char_display_width(ch);
			DRL_CHAR(ch);

			/* and adjust the apparent cursor position */
			new_cursor_pos--;

			if (buf == pt) {
				/* can't remove more than this */
				break;
			}
		}

		if (pos++ == *cursor_pos) {
			break;
		}

	}
	DRL(x_u("<snip>"));
	DRL_STR(buf);
	DRL(x_u("\nafter reduce, needcols=%d, new_cursor_pos=%d\n"), needcols, new_cursor_pos);

	/* Done, now new_cursor_pos contains the adjusted cursor position
	 * and buf points to he adjusted start
	 */
	*cursor_pos = new_cursor_pos;
	return buf;
}

static int mlmode = 0;

void x_edit_set_multi_line(int enableml)
{
	mlmode = enableml;
}

/* Helper of refreshSingleLine() and refreshMultiLine() to show hints
 * to the right of the prompt.
 * Returns 1 if a hint was shown, or 0 if not
 * If 'display' is 0, does no output. Just returns the appropriate return code.
 */
static int refresh_show_hints(struct current *current, const x_uchar *buf, int availcols, int display)
{
	int rc = 0;
	if (showhints && hints_cb && availcols > 0) {
		int bold = 0;
		int color = -1;
		x_uchar *hint = hints_cb(buf, &color, &bold, hints_arg);
		if (hint) {
			rc = 1;
			if (display) {
				const x_uchar *pt;
				if (bold == 1 && color == -1) color = 37;
				if (bold || color > 0) {
					int props[3] = { bold, color, 49 }; /* bold, color, fgnormal */
					set_output_highlight(current, props, 3);
				}
				DRL(x_u("<hint bold=%d,color=%d>"), bold, color);
				pt = hint;
				size_t len = x_ustrlen(hint);
				while (*pt) {
					uint32_t ch;
					int n = x_ustr_to_ucode(pt, len, &ch);
					int width = char_display_width(ch);
					if (width >= availcols) {
						DRL(x_u("<hinteol>"));
						break;
					}
					DRL_CHAR(ch);

					availcols -= width;
					output_chars(current, pt, n);
					pt += n;
					len -= n;
				}
				if (bold || color > 0) {
					clear_output_hightlight(current);
				}
				/* Call the function to free the hint returned. */
				if (free_hints_cb) free_hints_cb(hint, hints_arg);
			}
		}
	}
	return rc;
}

#ifdef USE_TERMIOS
static void refresh_start(struct current *current)
{
	/* We accumulate all output here */
	assert(current->use_output == false);
	x_strbuf_init(&current->output);
}

static void refresh_end(struct current *current)
{
	/* Output everything at once */
	IGNORE_RC(write(current->fd, x_strbuf_str(&current->output), x_strbuf_len(&current->output)));
	x_strbuf_free(&current->output);
	current->use_output = false;
}

static void refresh_start_chars(struct current *current)
{
	(void)current;
}

static void refresh_new_line(struct current *current)
{
	DRL(x_u("<nl>"));
	output_chars(current, "\n", 1);
}

static void refresh_end_chars(struct current *current)
{
	(void)current;
}
#endif

static void refresh_line_alt(struct current *current, const x_uchar *prompt, const x_uchar *buf, int cursor_pos)
{
	int i;
	const x_uchar *pt;
	int displaycol, displayrow, visible, currentpos,
			notecursor, cursorcol = 0, cursorrow = 0, hint;
	struct esc_parser parser;
	size_t len;
#ifdef DEBUG_REFRESHLINE
	dfh = fopen("linenoise.debuglog", "a");
#endif

	/* Should intercept SIGWINCH. For now, just get the size every time */
	get_window_size(current);
	refresh_start(current);
	DRL(x_u("wincols=%d, cursor_pos=%d, nrows=%d, rpos=%d\n"), current->cols, cursor_pos, current->nrows, current->rpos);
	/* Here is the plan:
	 * (a) move the the bottom row, going down the appropriate number of lines
	 * (b) move to beginning of line and erase the current line
	 * (c) go up one line and do the same, until we have erased up to the first row
	 * (d) output the prompt, counting cols and rows, taking into account escape sequences
	 * (e) output the buffer, counting cols and rows
	 *   (e') when we hit the current pos, save the cursor position
	 * (f) move the cursor to the saved cursor position
	 * (g) save the current cursor row and number of rows
	 */

	/* (a) - The cursor is currently at row rpos */
	cursor_down(current, current->nrows - current->rpos - 1);
	DRL(x_u("<cud=%d>"), current->nrows - current->rpos - 1);
	/* (b), (c) - Erase lines upwards until we get to the first row */
	for (i = 0; i < current->nrows; i++) {
		if (i) {
			DRL(x_u("<cup>"));
			cursor_up(current, 1);
		}
		DRL(x_u("<clearline>"));
		cursor_to_left(current);
		erase_eol(current);
	}
	DRL(x_u("\n"));
	/* (d) First output the prompt. control sequences don't take up display space */
	pt = prompt;
	len = x_ustrlen(pt);
	displaycol = 0; /* current display column */
	displayrow = 0; /* current display row */
	visible = 1;
	refresh_start_chars(current);
	while (*pt) {
		int width;
		uint32_t ch;
		int n = x_ustr_to_ucode(pt, len, &ch);
		if (visible && ch == CHAR_ESCAPE) {
			/* The start of an escape sequence, so not visible */
			visible = 0;
			init_parse_escape_seq(&parser, 'm');
			DRL(x_u("<esc-seq-start>"));
		}
		if (ch == '\n' || ch == '\r') {
			/* treat both CR and NL the same and force wrap */
			refresh_new_line(current);
			displaycol = 0;
			displayrow++;
		}
		else {
			width = visible * x_ucode_width(ch);

			displaycol += width;
			if (displaycol >= current->cols) {
				/* need to wrap to the next line because of newline or if it doesn't fit
				 * XXX this is a problem in single line mode
				 */
				refresh_new_line(current);
				displaycol = width;
				displayrow++;
			}
			DRL_CHAR(ch);
#ifdef USE_WINCONSOLE
			if (visible) {
				output_chars(current, pt, n);
			}
#else
			output_chars(current, pt, n);
#endif
		}
		pt += n;
		len -= n;
		if (!visible) {
			switch (parse_escape_sequence(&parser, ch)) {
				case EP_END:
					visible = 1;
					set_output_highlight(current, parser.props, parser.numprops);
					DRL(x_u("<esc-seq-end,numprops=%d>"), parser.numprops);
					break;
				case EP_ERROR:
					DRL(x_u("<esc-seq-err>"));
					visible = 1;
					break;
			}
		}

	}
	/* Now we are at the first line with all lines erased */
	DRL(x_u("\nafter prompt: displaycol=%d, displayrow=%d\n"), displaycol, displayrow);
	/* (e) output the buffer, counting cols and rows */
	if (mlmode == 0) {
		/* In this mode we may need to trim chars from the start of the buffer until the
		 * cursor fits in the window.
		 */
		pt = reduce_single_buf(buf, current->cols - displaycol, &cursor_pos);
	}
	else {
		pt = buf;
	}
	currentpos = 0;
	notecursor = -1;
	len = x_ustrlen(pt);
	while (*pt) {
		uint32_t ch;
		int n = x_ustr_to_ucode(pt, len, &ch);
		int width = char_display_width(ch);
		if (currentpos == cursor_pos) {
			/* (e') wherever we output this character is where we want the cursor */
			notecursor = 1;
		}
		if (displaycol + width >= current->cols) {
			if (mlmode == 0) {
				/* In single line mode stop once we print as much as we can on one line */
				DRL(x_u("<slmode>"));
				break;
			}
			/* need to wrap to the next line since it doesn't fit */
			refresh_new_line(current);
			displaycol = 0;
			displayrow++;
		}

		if (notecursor == 1) {
			/* (e') Save this position as the current cursor position */
			cursorcol = displaycol;
			cursorrow = displayrow;
			notecursor = 0;
			DRL(x_u("<cursor>"));
		}
		displaycol += width;
		if (ch < ' ') {
			output_control_char(current, ch + '@');
		}
		else {
			output_chars(current, pt, n);
		}
		DRL_CHAR(ch);
		if (width != 1) {
			DRL(x_u("<w=%d>"), width);
		}

		pt += n;
		len -= n;
		currentpos++;
	}
	/* If we didn't see the cursor, it is at the current location */
	if (notecursor) {
		DRL(x_u("<cursor>"));
		cursorcol = displaycol;
		cursorrow = displayrow;
	}
	DRL(x_u("\nafter buf: displaycol=%d, displayrow=%d, cursorcol=%d, cursorrow=%d\n"), displaycol, displayrow, cursorcol, cursorrow);
	/* (f) show hints */
	hint = refresh_show_hints(current, buf, current->cols - displaycol, 1);
	/* Remember how many many cols are available for insert optimisation */
	if (prompt == current->prompt && hint == 0) {
		current->colsright = current->cols - displaycol;
		current->colsleft = displaycol;
	}
	else {
		/* Can't optimise */
		current->colsright = 0;
		current->colsleft = 0;
	}
	DRL(x_u("\nafter hints: colsleft=%d, colsright=%d\n\n"), current->colsleft, current->colsright);
	refresh_end_chars(current);
	/* (g) move the cursor to the correct place */
	cursor_up(current, displayrow - cursorrow);
	set_cursor_pos(current, cursorcol);
	/* (h) Update the number of rows if larger, but never reduce this */
	if (displayrow >= current->nrows) {
		current->nrows = displayrow + 1;
	}
	/* And remember the row that the cursor is on */
	current->rpos = cursorrow;
	refresh_end(current);
#ifdef DEBUG_REFRESHLINE
	fclose(dfh);
#endif
}

static void refresh_line(struct current *current)
{
	refresh_line_alt(current, current->prompt, x_strbuf_str(&current->buf), current->pos);
}

static void set_current(struct current *current, const x_uchar *str)
{
	x_strbuf_clear(&current->buf);
	x_strbuf_append(&current->buf, str);
	current->pos = x_strbuf_chars(&current->buf);
}

/**
 * Removes the char at 'pos'.
 *
 * Returns 1 if the line needs to be refreshed, 2 if not
 * and 0 if nothing was removed
 */
static int remove_char(struct current *current, int pos)
{
	if (pos >= 0 && pos < x_strbuf_chars(&current->buf)) {
		int offset = x_ustr_index(x_strbuf_str(&current->buf), pos);
		int nchars = x_ustr_index(x_strbuf_str(&current->buf) + offset, 1);
		int rc = 1;

		/* Now we try to optimise in the simple but very common case that:
		 * - output_chars() can be used directly (not win32)
		 * - we are removing the char at EOL
		 * - the buffer is not empty
		 * - there are columns available to the left
		 * - the char being deleted is not a wide or utf-8 character
		 * - no hints are being shown
		 */
		if (current->use_output && current->pos == pos + 1 && current->pos == x_strbuf_chars(&current->buf) && pos > 0) {
			/* Could implement x_utf8_prev_len() but simplest just to not optimise this case */
			x_uchar last = x_strbuf_str(&current->buf)[offset];
			if (current->colsleft > 0 && (last & 0x80) == 0) {
				/* Have cols on the left and not a UTF-8 char or continuation */
				/* Yes, can optimise */
				current->colsleft--;
				current->colsright++;
				rc = 2;
			}
		}
		x_strbuf_delete(&current->buf, offset, nchars);
		if (current->pos > pos) {
			current->pos--;
		}
		if (rc == 2) {
			if (refresh_show_hints(current, x_strbuf_str(&current->buf), current->colsright, 0)) {
				/* A hint needs to be shown, so can't optimise after all */
				rc = 1;
			}
			else {
				/* optimised output */
				output_chars(current, x_u("\b \b"), 3);
			}
		}
		return rc;
		return 1;
	}
	return 0;
}

/**
 * Insert 'ch' at position 'pos'
 *
 * Returns 1 if the line needs to be refreshed, 2 if not
 * and 0 if nothing was inserted (no room)
 */
static int insert_char(struct current *current, int pos, int ch)
{
	if (pos >= 0 && pos <= x_strbuf_chars(&current->buf)) {
		x_uchar buf[MAX_UTF8_LEN + 1];
		int offset = x_ustr_index(x_strbuf_str(&current->buf), pos);
		int n = x_ucode_to_ustr(ch, buf);
		int rc = 1;
		/* null terminate since x_strbuf_insert() requires it */
		buf[n] = 0;
		/* Now we try to optimise in the simple but very common case that:
		 * - output_chars() can be used directly (not win32)
		 * - we are inserting at EOL
		 * - there are enough columns available
		 * - no hints are being shown
		 */
		if (current->use_output && pos == current->pos && pos == x_strbuf_chars(&current->buf)) {
			int width = char_display_width(ch);
			if (current->colsright > width) {
				/* Yes, can optimise */
				current->colsright -= width;
				current->colsleft -= width;
				rc = 2;
			}
		}
		x_strbuf_insert(&current->buf, offset, buf);
		if (current->pos >= pos) {
			current->pos++;
		}
		if (rc == 2) {
			if (refresh_show_hints(current, x_strbuf_str(&current->buf), current->colsright, 0)) {
				/* A hint needs to be shown, so can't optimise after all */
				rc = 1;
			}
			else {
				/* optimised output */
				output_chars(current, buf, n);
			}
		}
		return rc;
	}
	return 0;
}

/**
 * Captures up to 'n' characters starting at 'pos' for the cut buffer.
 *
 * This replaces any existing characters in the cut buffer.
 */
static void capture_chars(struct current *current, int pos, int nchars)
{
	if (pos >= 0 && (pos + nchars - 1) < x_strbuf_chars(&current->buf)) {
		int offset = x_ustr_index(x_strbuf_str(&current->buf), pos);
		int nbytes = x_ustr_index(x_strbuf_str(&current->buf) + offset, nchars);
		if (nbytes > 0) {
			if (current->use_capture) {
				x_strbuf_clear(&current->capture);
			}
			else {
				x_strbuf_init(&current->capture);
				current->use_capture = true;
			}
			x_strbuf_append_len(&current->capture, x_strbuf_str(&current->buf) + offset, nbytes);
		}
	}
}

/**
 * Removes up to 'n' characters at cursor position 'pos'.
 *
 * Returns 0 if no chars were removed or non-zero otherwise.
 */
static int remove_chars(struct current *current, int pos, int n)
{
	int removed = 0;
	/* First save any chars which will be removed */
	capture_chars(current, pos, n);
	while (n-- && remove_char(current, pos)) {
		removed++;
	}
	return removed;
}
/**
 * Inserts the characters (string) 'chars' at the cursor position 'pos'.
 *
 * Returns 0 if no chars were inserted or non-zero otherwise.
 */
static int insert_chars(struct current *current, int pos, const x_uchar *chars)
{
	int inserted = 0;
	size_t len = x_ustrlen(chars);
	while (*chars) {
		uint32_t ch;
		int n = x_ustr_to_ucode(chars, len, &ch);
		if (insert_char(current, pos, ch) == 0) {
			break;
		}
		inserted++;
		pos++;
		chars += n;
		len -= n;
	}
	return inserted;
}

static int skip_space_nonspace(struct current *current, int dir, int check_is_space)
{
	int moved = 0;
	int checkoffset = (dir < 0) ? -1 : 0;
	int limit = (dir < 0) ? 0 : x_strbuf_chars(&current->buf);
	while (current->pos != limit && (get_char(current, current->pos + checkoffset) == ' ') == check_is_space) {
		current->pos += dir;
		moved++;
	}
	return moved;
}

static int skip_space(struct current *current, int dir)
{
	return skip_space_nonspace(current, dir, 1);
}

static int skip_nonspace(struct current *current, int dir)
{
	return skip_space_nonspace(current, dir, 0);
}

static void set_history_index(struct current *current, int new_index)
{
	if (history_len > 1) {
		/* Update the current history entry before to
		 * overwrite it with the next one. */
		free(history[history_len - 1 - history_index]);
		history[history_len - 1 - history_index] = x_ustrdup(x_strbuf_str(&current->buf));
		/* Show the new entry */
		history_index = new_index;
		if (history_index < 0) {
			history_index = 0;
		} else if (history_index >= history_len) {
			history_index = history_len - 1;
		} else {
			set_current(current, history[history_len - 1 - history_index]);
			refresh_line(current);
		}
	}
}

/**
 * Returns the keycode to process, or 0 if none.
 */
static int reverse_incremental_search(struct current *current)
{
	/* Display the reverse-i-search prompt and process chars */
	x_uchar rbuf[50];
	x_uchar rprompt[80];
	int rchars = 0;
	int rlen = 0;
	int searchpos = history_len - 1;
	int c;
	rbuf[0] = 0;
	while (1) {
		int n = 0;
		const x_uchar *p = NULL;
		int skipsame = 0;
		int searchdir = -1;
		x_snprintf(rprompt, x_arrlen(rprompt), x_u("(reverse-i-search)'%s': "), rbuf);
		refresh_line_alt(current, rprompt, x_strbuf_str(&current->buf), current->pos);
		c = fd_read(current);
		if (c == ctrl('H') || c == CHAR_DELETE) {
			if (rchars) {
				int p_ind = x_ustr_index(rbuf, --rchars);
				rbuf[p_ind] = 0;
				rlen = x_ustrlen(rbuf);
			}
			continue;
		}
#ifdef USE_TERMIOS
		if (c == CHAR_ESCAPE) {
			c = check_special(current->fd);
		}
#endif
		if (c == ctrl('R')) {
			/* Search for the previous (earlier) match */
			if (searchpos > 0) {
				searchpos--;
			}
			skipsame = 1;
		}
		else if (c == ctrl('S')) {
			/* Search for the next (later) match */
			if (searchpos < history_len) {
				searchpos++;
			}
			searchdir = 1;
			skipsame = 1;
		}
		else if (c == ctrl('P') || c == SPECIAL_UP) {
			/* Exit Ctrl-R mode and go to the previous history line from the current search pos */
			set_history_index(current, history_len - searchpos);
			c = 0;
			break;
		}
		else if (c == ctrl('N') || c == SPECIAL_DOWN) {
			/* Exit Ctrl-R mode and go to the next history line from the current search pos */
			set_history_index(current, history_len - searchpos - 2);
			c = 0;
			break;
		}
		else if (c >= ' ' && c <= '~') {
			/* >= here to allow for null terminator */
			if (rlen >= (int)x_arrlen(rbuf) - MAX_UTF8_LEN) {
				continue;
			}

			n = x_ucode_to_ustr(c, rbuf + rlen);
			rlen += n;
			rchars++;
			rbuf[rlen] = 0;

			/* Adding a new char resets the search location */
			searchpos = history_len - 1;
		}
		else {
			/* Exit from incremental search mode */
			break;
		}

		/* Now search through the history for a match */
		for (; searchpos >= 0 && searchpos < history_len; searchpos += searchdir) {
			p = x_ustrstr(history[searchpos], rbuf);
			if (p) {
				/* Found a match */
				if (skipsame && x_ustrcmp(history[searchpos], x_strbuf_str(&current->buf)) == 0) {
					/* But it is identical, so skip it */
					continue;
				}
				/* Copy the matching line and set the cursor position */
				history_index = history_len - 1 - searchpos;
				set_current(current,history[searchpos]);
				current->pos = x_ustr_charcnt(history[searchpos], p - history[searchpos]);
				break;
			}
		}
		if (!p && n) {
			/* No match, so don't add it */
			rchars--;
			rlen -= n;
			rbuf[rlen] = 0;
		}
	}
	if (c == ctrl('G') || c == ctrl('C')) {
		/* ctrl-g terminates the search with no effect */
		set_current(current, x_u(""));
		history_index = 0;
		c = 0;
	}
	else if (c == ctrl('J')) {
		/* ctrl-j terminates the search leaving the buffer in place */
		history_index = 0;
		c = 0;
	}
	/* Go process the char normally */
	refresh_line(current);
	return c;
}

static int edit_line(struct current *current)
{
	history_index = 0;
	refresh_line(current);
	while(1) {
		int c = fd_read(current);
#ifndef NO_COMPLETION
		/* Only autocomplete when the callback is set. It returns < 0 when
		 * there was an error reading from fd. Otherwise it will return the
		 * character that should be handled next. */
		if (c == '\t' && current->pos == x_strbuf_chars(&current->buf) && completionCallback != NULL) {
			c = completeLine(current);
		}
#endif
		if (c == ctrl('R')) {
			/* reverse incremental search will provide an alternative keycode or 0 for none */
			c = reverse_incremental_search(current);
			/* go on to process the returned char normally */
		}

#ifdef USE_TERMIOS
		if (c == CHAR_ESCAPE) {   /* escape sequence */
			c = check_special(current->fd);
		}
#endif
		if (c == -1) {
			/* Return on errors */
			return x_strbuf_len(&current->buf);
		}

		switch(c) {
			case SPECIAL_NONE:
				break;
			case '\r':    /* enter/CR */
			case '\n':    /* LF */
				history_len--;
				free(history[history_len]);
				current->pos = x_strbuf_chars(&current->buf);
				if (mlmode || hints_cb) {
					showhints = 0;
					refresh_line(current);
					showhints = 1;
				}
				return x_strbuf_len(&current->buf);
			case ctrl('C'):     /* ctrl-c */
				errno = EAGAIN;
				return -1;
			case ctrl('Z'):     /* ctrl-z */
#ifdef SIGTSTP
				/* send ourselves SIGSUSP */
				disable_raw_mode(current);
				raise(SIGTSTP);
				/* and resume */
				enable_raw_mode(current);
				refresh_line(current);
#endif
				continue;
			case CHAR_DELETE:   /* backspace */
			case ctrl('H'):
				if (remove_char(current, current->pos - 1) == 1) {
					refresh_line(current);
				}
				break;
			case ctrl('D'):     /* ctrl-d */
				if (x_strbuf_len(&current->buf) == 0) {
					/* Empty line, so EOF */
					history_len--;
					free(history[history_len]);
					return -1;
				}
				/* Otherwise fall through to delete char to right of cursor */
				/* fall-thru */
			case SPECIAL_DELETE:
				if (remove_char(current, current->pos) == 1) {
					refresh_line(current);
				}
				break;
			case SPECIAL_INSERT:
				/* Ignore. Expansion Hook.
				 * Future possibility: Toggle Insert/Overwrite Modes
				 */
				break;
			case meta('b'):    /* meta-b, move word left */
				if (skip_nonspace(current, -1)) {
					refresh_line(current);
				}
				else if (skip_space(current, -1)) {
					skip_nonspace(current, -1);
					refresh_line(current);
				}
				break;
			case meta('f'):    /* meta-f, move word right */
				if (skip_space(current, 1)) {
					refresh_line(current);
				}
				else if (skip_nonspace(current, 1)) {
					skip_space(current, 1);
					refresh_line(current);
				}
				break;
			case ctrl('W'):    /* ctrl-w, delete word at left. save deleted chars */
				/* eat any spaces on the left */
				{
					int pos = current->pos;
					while (pos > 0 && get_char(current, pos - 1) == ' ') {
						pos--;
					}

					/* now eat any non-spaces on the left */
					while (pos > 0 && get_char(current, pos - 1) != ' ') {
						pos--;
					}

					if (remove_chars(current, pos, current->pos - pos)) {
						refresh_line(current);
					}
				}
				break;
			case ctrl('T'):    /* ctrl-t */
				if (current->pos > 0 && current->pos <= x_strbuf_chars(&current->buf)) {
					/* If cursor is at end, transpose the previous two chars */
					int fixer = (current->pos == x_strbuf_chars(&current->buf));
					c = get_char(current, current->pos - fixer);
					remove_char(current, current->pos - fixer);
					insert_char(current, current->pos - 1, c);
					refresh_line(current);
				}
				break;
			case ctrl('V'):    /* ctrl-v */
				/* Insert the ^V first */
				if (insert_char(current, current->pos, c)) {
					refresh_line(current);
					/* Now wait for the next char. Can insert anything except \0 */
					c = fd_read(current);

					/* Remove the ^V first */
					remove_char(current, current->pos - 1);
					if (c > 0) {
						/* Insert the actual char, can't be error or null */
						insert_char(current, current->pos, c);
					}
					refresh_line(current);
				}
				break;
			case ctrl('B'):
			case SPECIAL_LEFT:
				if (current->pos > 0) {
					current->pos--;
					refresh_line(current);
				}
				break;
			case ctrl('F'):
			case SPECIAL_RIGHT:
				if (current->pos < x_strbuf_chars(&current->buf)) {
					current->pos++;
					refresh_line(current);
				}
				break;
			case SPECIAL_PAGE_UP: /* move to start of history */
				set_history_index(current, history_len - 1);
				break;
			case SPECIAL_PAGE_DOWN: /* move to 0 == end of history, i.e. current */
				set_history_index(current, 0);
				break;
			case ctrl('P'):
			case SPECIAL_UP:
				set_history_index(current, history_index + 1);
				break;
			case ctrl('N'):
			case SPECIAL_DOWN:
				set_history_index(current, history_index - 1);
				break;
			case ctrl('A'): /* Ctrl+a, go to the start of the line */
			case SPECIAL_HOME:
				current->pos = 0;
				refresh_line(current);
				break;
			case ctrl('E'): /* ctrl+e, go to the end of the line */
			case SPECIAL_END:
				current->pos = x_strbuf_chars(&current->buf);
				refresh_line(current);
				break;
			case ctrl('U'): /* Ctrl+u, delete to beginning of line, save deleted chars. */
				if (remove_chars(current, 0, current->pos)) {
					refresh_line(current);
				}
				break;
			case ctrl('K'): /* Ctrl+k, delete from current to end of line, save deleted chars. */
				if (remove_chars(current, current->pos, x_strbuf_chars(&current->buf) - current->pos)) {
					refresh_line(current);
				}
				break;
			case ctrl('Y'): /* Ctrl+y, insert saved chars at current position */
				if (current->use_capture && insert_chars(current, current->pos, x_strbuf_str(&current->capture))) {
					refresh_line(current);
				}
				break;
			case ctrl('L'): /* Ctrl+L, clear screen */
				x_edit_clear_screen();
				/* Force recalc of window size for serial terminals */
				current->cols = 0;
				current->rpos = 0;
				refresh_line(current);
				break;
			default:
				if (c >= meta('a') && c <= meta('z')) {
					/* Don't insert meta chars that are not bound */
					break;
				}
				/* Only tab is allowed without ^V */
				if (c == '\t' || c >= ' ') {
					if (insert_char(current, current->pos, c) == 1) {
						refresh_line(current);
					}
				}
				break;
		}
	}
	return x_strbuf_len(&current->buf);
}

int x_edit_columns(void)
{
	struct current current;
	current.use_output = false;
	enable_raw_mode (&current);
	get_window_size (&current);
	disable_raw_mode (&current);
	return current.cols;
}

/**
 * Reads a line from the file handle (without the trailing NL or CRNL)
 * and returns it in a x_strbuf.
 * Returns NULL if no characters are read before EOF or error.
 *
 * Note that the character count will *not* be correct for lines containing
 * utf8 sequences. Do not rely on the character count.
 */
int x_strbuf_getline(FILE *fh, x_strbuf *sb)
{
	int c;
	int n = 0;
	x_strbuf_init(sb);

	while ((c = getc(fh)) != EOF) {
		x_uchar ch;
		n++;
		if (c == '\r') {
			/* CRLF -> LF */
			continue;
		}
		if (c == '\n' || c == '\r') {
			break;
		}
		ch = c;
		/* ignore the effect of character count for partial utf8 sequences */
		x_strbuf_append_len(sb, &ch, 1);
	}
	if (n == 0 || x_strbuf_str(sb) == NULL) {
		x_strbuf_free(sb);
		return -1;
	}
	return 0;
}

x_uchar *x_edit_readline2(const x_uchar *prompt, const x_uchar *initial)
{
	int count;
	struct current current;
	x_strbuf sb;
	memset(&current, 0, sizeof(current));
	if (enable_raw_mode(&current) == -1) {
		x_printf(x_u("%s"), prompt);
		fflush(stdout);
		if (x_strbuf_getline(stdin, &sb) && !fd_isatty(&current)) {
			x_printf(x_u("%s\n"), x_strbuf_str(&sb));
			fflush(stdout);
		}
	}
	else {
		x_strbuf_init(&current.buf);
		current.pos = 0;
		current.nrows = 1;
		current.prompt = prompt;

		/* The latest history entry is always our current buffer */
		x_edit_history_add(initial);
		set_current(&current, initial);

		count = edit_line(&current);

		disable_raw_mode(&current);
		x_printf(x_u("\n"));

		x_strbuf_free(&current.capture);
		if (count == -1) {
			x_strbuf_free(&current.buf);
			return NULL;
		}
		sb = current.buf;
	}
	return x_strbuf_data(&sb);
}

x_uchar *x_edit_readline(const x_uchar *prompt)
{
	return x_edit_readline2(prompt, x_u(""));
}

/* Using a circular buffer is smarter, but a bit more complex to handle. */
static int x_edit_history_add_allocated(x_uchar *line) {

	if (history_max_len == 0) {
notinserted:
		free(line);
		return 0;
	}
	if (history == NULL) {
		history = (x_uchar **)calloc(sizeof(x_uchar*), history_max_len);
	}
	/* do not insert duplicate lines into history */
	if (history_len > 0 && x_ustrcmp(line, history[history_len - 1]) == 0) {
		goto notinserted;
	}
	if (history_len == history_max_len) {
		free(history[0]);
		memmove(history,history+1,sizeof(x_uchar*)*(history_max_len-1));
		history_len--;
	}
	history[history_len] = line;
	history_len++;
	return 1;
}

int x_edit_history_add(const x_uchar *line)
{
	return x_edit_history_add_allocated(x_ustrdup(line));
}

int x_edit_history_maxlen(void)
{
	return history_max_len;
}

int x_edit_history_set_maxlen(int len)
{
	x_uchar **newHistory;
	if (len < 1) return 0;
	if (history) {
		int tocopy = history_len;
		newHistory = (x_uchar **)calloc(sizeof(x_uchar*), len);
		/* If we can't copy everything, free the elements we'll not use. */
		if (len < tocopy) {
			int j;
			for (j = 0; j < tocopy-len; j++) free(history[j]);
			tocopy = len;
		}
		memcpy(newHistory,history+(history_len-tocopy), sizeof(x_uchar*) * tocopy);
		free(history);
		history = newHistory;
	}
	history_max_len = len;
	if (history_len > history_max_len)
		history_len = history_max_len;
	return 1;
}

/* Save the history in the specified file. On success 0 is returned
 * otherwise -1 is returned. */
int x_edit_history_save(const x_uchar *filename) {
	FILE *fp = x_fopen(filename, x_u("w"));
	int j;

	if (fp == NULL) return -1;
	for (j = 0; j < history_len; j++) {
		const x_uchar *str = history[j];
		/* Need to encode backslash, nl and cr */
		while (*str) {
			if (*str == '\\')
				x_fputs(x_u("\\\\"), fp);
			else if (*str == '\n')
				x_fputs(x_u("\\n"), fp);
			else if (*str == '\r')
				x_fputs(x_u("\\r"), fp);
			else
				x_fputc(*str, fp);
			str++;
		}
		x_fputc(x_u('\n'), fp);
	}
	fclose(fp);
	return 0;
}

/* Load the history from the specified file.
 *
 * If the file does not exist or can't be opened, no operation is performed
 * and -1 is returned.
 * Otherwise 0 is returned.
 */
int x_edit_history_load(const x_uchar *filename)
{
	FILE *fp = x_fopen(filename, x_u("r"));
	x_strbuf sb;
	if (!fp)
		return -1;
	while (!x_strbuf_getline(fp, &sb)) {
		/* Take the x_strbuf and decode backslash escaped values */
		x_uchar *buf = x_strbuf_data(&sb);
		x_uchar *dest = buf;
		const x_uchar *src;
		for (src = buf; *src; src++) {
			x_uchar ch = *src;
			if (ch == '\\') {
				src++;
				if (*src == 'n') {
					ch = '\n';
				}
				else if (*src == 'r') {
					ch = '\r';
				} else {
					ch = *src;
				}
			}
			*dest++ = ch;
		}
		*dest = 0;

		x_edit_history_add_allocated(buf);
	}
	fclose(fp);
	return 0;
}

/* Provide access to the history buffer.
 *
 * If 'len' is not NULL, the length is stored in *len.
 */
x_uchar **x_edit_history(int *len)
{
	if (len)
		*len = history_len;
	return history;
}
