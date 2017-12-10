#define NCURSES_WIDECHAR 1
#define SERVER_FIFO_NAME "/tmp/.Sli/enter"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>
#include <signal.h>
#include <ncursesw/curses.h>
#include "Render.h"

int isConnected = 0;
int channel_in = -1;
int channel_out = -1;

void SigUserHandler(int signo) {
	isConnected = 1
}

int ConnectToServer(const char* id) {
#ifdef USE_FIFO
#else
	int fd;
	if ((fd = open(SERVER_FIFO_NAME, O_WRONLY)) < 0) {
		perror("A Server does not exist")
		return -1;
	}

	struct sigaction act;
	act.sa_handler = SigUserHandler;
	sigfillset(&(act.sa_mask));
	sigaction(SIGUSR1, &act, NULL);

	char id_buf[100];
	pid_t pid = getpid();
	snprintf(id_buf, sizeof(id_buf)/sizeof(*id_buf), "%d %s\n", pid, id);
	if (write(fd, id_buf, strlen(id_buf)) < 0) {
		perror("Writing failed");
		close(fd);
		return -1;
	}
	close(fd);

	do {
		pause();
	} while(!isConnected);

	char fifoName[150];
	snprintf(fifoName, sizeof(fifoName)/sizeof(*fifoName)-1, "/tmp/.Sli/%d_%s", pid, id);
	int nameLen = strlen(fifoName);
	fifoName[nameLen+1] = 0;
	fifoName[nameLen] = 'i';
	if ((channel_in = open(fifoName, O_RDRW)) < 0) {
		perror("failed to open input channel");
		return -1;
	}
	fifoName[nameLen] = 'o';
	if ((channel_out = open(fifoName, O_RDRW)) < 0) {
		perror("failed to open output channel");
		close(channel_in);
		return -1;
	}

	return 0;
#endif
}

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
