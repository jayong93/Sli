#define NCURSES_WIDECHAR 1

#include <string.h>
#include <unistd.h>
#include <locale.h>
#include <ncursesw/curses.h>
#include "Render.h"

int main(){
	WINDOW* mainWin;
	int isColor;

	setlocale(LC_CTYPE, "ko_KR.utf-8");

	initscr();
	isColor = has_colors();
	if (isColor)
	{
		start_color();
		init_pair(1, 12, COLOR_BLACK);
	}
	else {
		endwin();
		printf("this terminal has no colors.\n");
		return 0;
	}
	cbreak();
	curs_set(0);

	mainWin = newwin(WINDOW_HEIGHT, WINDOW_WIDTH, 0, 0);
	noecho();

	box(mainWin, 0, 0);

	char buf[19];
	char* p_buf = buf;
	*(int*)p_buf = 7;
	p_buf += sizeof(int);
	strncpy(p_buf, "abcdefg", 7);
	p_buf += 7;
	*((int*)p_buf) = 0;
	p_buf += sizeof(int);
	*((int*)p_buf) = 0;
	p_buf += sizeof(int);

	Point pos = {100, 20};
	Point tail = {70, 20};
	Point tail2 = {70, 5};
	unsigned int score = 0;

	while(1){
		Point p = pos, t = tail, t2 = tail2;
		TransformToScreen(pos, &p);
		TransformToScreen(pos, &t);
		TransformToScreen(pos, &t2);
		score++;
		DrawRankingBar(mainWin, buf);
		DrawStatusBar(mainWin, pos, score);
		mvwprintw(mainWin, 20, 10, "%d", COLORS);
		wattron(mainWin, COLOR_PAIR(1));
		mvwaddch(mainWin, p.y, p.x, 'O');
		wattroff(mainWin, COLOR_PAIR(1));
		DrawLine(mainWin, p, t);
		DrawLine(mainWin, t, t2);
		wrefresh(mainWin);
		usleep(32);
	}

	wgetch(mainWin);

	endwin();
}
