#define NCURSES_WIDECHAR 1

#include <locale.h>
#include <ncursesw/curses.h>
#include "Render.h"

int main(){
	WINDOW* mainWin;

	setlocale(LC_CTYPE, "ko_KR.utf-8");

	initscr();
	cbreak();
	curs_set(0);

	mainWin = newwin(WINDOW_HEIGHT, WINDOW_WIDTH, 0, 0);
	noecho();

	box(mainWin, 0, 0);
	wrefresh(mainWin);

	while(1) {
		wint_t ch;
		wget_wch(mainWin, &ch);
		if (ch == 'q' || ch == 'Q') break;
		mvwprintw(mainWin, 0, 0, "테스트용 문자열 : %c", ch);
	}

	endwin();
}
