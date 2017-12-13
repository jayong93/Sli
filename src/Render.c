#define NCURSES_WIDECHAR 1

#include <stdlib.h>
#include <malloc.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sched.h>
#include <locale.h>
#include "Render.h"
#include "Util.h"

#define BODY 'O'
#define STAR '*'

extern pthread_mutex_t rDataLock;
extern pthread_mutex_t inputLock;
extern pthread_cond_t inputCond;
WINDOW* mainWin = NULL;
WINDOW* backWin = NULL;
extern VBuffer renderData;
extern int isUpdated;
extern const char* myID;

int ScoreCmp(const void* a, const void* b) {
	int aScore = ((int*)a)[1];
	int bScore = ((int*)b)[1];
	if (aScore > bScore) return -1;
	if (aScore < bScore) return 1;
	return 0;
}

int InitWindow(WINDOW* win, void* data) {
	keypad(win, TRUE);
	nodelay(win, TRUE);
	return 0;
}

int CopyToMain(WINDOW* win, void* data) {
	WINDOW* src = (WINDOW*)data;
	overwrite(src, win);
	wrefresh(win);
}

int RenderScreen(WINDOW* win, void* data) {
	int nColor = **(int**)(data+sizeof(void*));
	VBuffer* bufList = *(VBuffer**)(data);
	VBuffer* localBuf = bufList;
	VBuffer* scores = bufList+1;
	VBuffer* ids = bufList+2;
	char* pData = localBuf->ptr;
	int nPlayer = *(int*)MovePointer(&pData, sizeof(int));
	char* pRender = pData;

	char playerID[11];
	int myIndex = -1;
	Point camPos;

	VBClear(scores);
	VBClear(ids);

	int i;

	// Get scores, ids, camera pos
	for (i=0; i<nPlayer; ++i) {
		VBAppend(scores, &i, sizeof(int));
		VBAppend(scores, (int*)MovePointer(&pData, sizeof(int)), sizeof(int));
		MovePointer(&pData, sizeof(unsigned char));
		unsigned int* idLen = (unsigned int*)MovePointer(&pData, sizeof(unsigned int));
		VBAppend(ids, &idLen, sizeof(idLen));

		size_t rCount = (*idLen > 10)?10:*idLen;
		strncpy(playerID, (char*)MovePointer(&pData, *idLen), rCount);
		playerID[rCount] = 0;
		if (myIndex < 0 && strcmp(playerID, myID) == 0) {
			myIndex = i;
		}

		unsigned int nPoints = *(unsigned int*)MovePointer(&pData, sizeof(unsigned int));
		int j=0;
		if (myIndex == i) {
			camPos = *(Point*)MovePointer(&pData, sizeof(Point));
			j++;
		}

		for(; j<nPoints; ++j) {
			MovePointer(&pData, sizeof(Point));
		}
	}

	werase(win);

	// Draw screen edge
	mvwaddch(win, 0, 0, '+');
	mvwaddch(win, 0, WINDOW_WIDTH-1, '+');
	mvwaddch(win, WINDOW_HEIGHT-1, WINDOW_WIDTH-1, '+');
	mvwaddch(win, WINDOW_HEIGHT-1, 0, '+');
	mvwhline(win, 0, 1, '-', WINDOW_WIDTH-2);
	mvwhline(win, WINDOW_HEIGHT-1, 1, '-', WINDOW_WIDTH-2);
	mvwvline(win, 1, 0, '|', WINDOW_HEIGHT-2);
	mvwvline(win, 1, WINDOW_WIDTH-1, '|', WINDOW_HEIGHT-2);

	// Draw Status Bar
	if (myIndex >= 0)
		DrawStatusBar(win, camPos, scores->ptr[myIndex]);

	// Draw players
	Point p1, p2;
	for (i=0; i<nPlayer; ++i) {
		MovePointer(&pRender, sizeof(int));
		unsigned char color = *(unsigned char*)MovePointer(&pRender, sizeof(unsigned char));
		color = color%nColor + 1;
		unsigned int idLen = *(unsigned int*)MovePointer(&pRender, sizeof(unsigned int));
		char* id = (char*)MovePointer(&pRender, idLen);

		unsigned int nPoints = *(unsigned int*)MovePointer(&pRender, sizeof(unsigned int));
		p1 = *(Point*)MovePointer(&pRender, sizeof(Point));
		TransformToScreen(camPos, &p1);
		Point head = p1;

		// Draw body
		wattron(win, COLOR_PAIR(color));
		int j;
		for (j=1; j<nPoints; ++j) {
			p2 = *(Point*)MovePointer(&pRender, sizeof(Point));
			TransformToScreen(camPos, &p2);
			DrawLine(win, p1, p2);
			p1 = p2;
		}
		wattroff(win, COLOR_PAIR(color));
		// Draw head
		wattron(win, COLOR_PAIR(1));
		mvwaddch(win, head.y, head.x, BODY);
		wattroff(win, COLOR_PAIR(1));
	}

	// Draw stars
	int nStar = *(int*)MovePointer(&pRender, sizeof(int));
	Point star;
	for (i=0; i<nStar; ++i) {
		star = *(Point*)MovePointer(&pRender, sizeof(Point));
		TransformToScreen(camPos, &star);
		if (star.y < 1 || star.y > WINDOW_HEIGHT-2) continue;
		if (star.x < 1 || star.x > WINDOW_WIDTH-2) continue;
		mvwaddch(win, star.y, star.x, STAR);
	}

	qsort(scores->ptr, scores->len/(sizeof(int)*2), sizeof(int)*2, ScoreCmp);
	DrawRankingBar(win, (const int*)scores->ptr, (const size_t*)ids->ptr, nPlayer);
	return 0;
}

void TransformToScreen(Point base, Point* target) {
	target->x = (int)floor((target->x - base.x)/10. + 0.5) + ((WINDOW_WIDTH-2)/2) + 1;
	target->y = (WINDOW_HEIGHT-1) - ((int)floor((target->y - base.y)/10. + 0.5) + ((WINDOW_HEIGHT-2)/2) + 1);
}

void DrawLine(WINDOW* win, Point start, Point end) {
	int xoffset = (end.x > start.x)?1:((end.x < start.x)?-1:0);
	int yoffset = (end.y > start.y)?1:((end.y < start.y)?-1:0);

	if (xoffset == 0 && yoffset == 0) return;
	int cx = start.x + xoffset, cy = start.y + yoffset;

	for (; cx-xoffset != end.x || cy-yoffset != end.y; cx += xoffset, cy += yoffset) {
		if (cx < 1 || cx > WINDOW_WIDTH-2) break;
		if (cy < 1 || cy > WINDOW_HEIGHT-2) break;
		mvwaddch(win, cy, cx, BODY);
	}
}

void DrawStatusBar(WINDOW* win, Point pos, unsigned int score) {
	char scoreBuf[20];
	mvwprintw(win, WINDOW_HEIGHT-1, 2, "X, Y: %d, %d", pos.x, pos.y);
	sprintf(scoreBuf, "Score: %d", score);
	mvwaddstr(win, WINDOW_HEIGHT-1, WINDOW_WIDTH-2-strlen(scoreBuf), scoreBuf);
}

void DrawRankingBar(WINDOW* win, const int* idxList, const size_t* idList, int playerNum) {
	wmove(win, 0, 2);
	waddstr(win, "Ranking: ");
	int renderCount = (playerNum < 3)?playerNum:3;
	int i;
	for (i=0; i<renderCount; ++i) {
		const int* pId = (const int*)idList[idxList[i*2]];
		int len = *pId++;
		const char* data = (const char*)pId;

		wprintw(win, "%d. ", i);
		int j;
		for (j=0; j<len; ++j) {
			waddch(win, data[j]);
		}
		if (i<renderCount-1)
			waddstr(win, ", ");
	}
}

void* Render() {
	int nColor;
	setlocale(LC_ALL, "");

	pthread_mutex_lock(&inputLock);

	initscr();
	backWin = newwin(WINDOW_HEIGHT, WINDOW_WIDTH, 0, 0);
	mainWin = newwin(WINDOW_HEIGHT, WINDOW_WIDTH, 0, 0);
	cbreak();
	noecho();
	curs_set(0);

	if (has_colors())
	{
		start_color();
		int ignore[] = {0, 16, 8};
		// COLOR_PAIR 번호는 1~64
		int i, j;
		nColor = (COLOR_PAIRS<227)?COLOR_PAIRS:227;
		for (i=0, j=0; i<nColor; ++i, ++j) {
			int t;
			for (t=0; t < (sizeof(ignore)/sizeof(*ignore)); ++t) {
				if (ignore[t] == j) {
					++j;
					t = -1;
				}
			}
			init_pair(i+1, j, COLOR_BLACK);
		}
	}
	else {
		endwin();
		printf("this terminal has no colors.\n");
		exit(6);
	}

	use_window(mainWin, InitWindow, NULL);
	
	pthread_cond_signal(&inputCond);
	pthread_mutex_unlock(&inputLock);

	VBuffer buf[3];
	buf[0] = VBCreate(50);
	buf[1] = VBCreate(80);
	buf[2] = VBCreate(40);	// [#len|data(without null)]
	void* renDatas[2] = {buf, &nColor};
	while(1) {
		// 렌더링 데이터 복사
		pthread_mutex_lock(&rDataLock);
		if (isUpdated) {
			VBReplace(buf, renderData.ptr, renderData.len);
			isUpdated = 0;
			pthread_mutex_unlock(&rDataLock);

			RenderScreen(backWin, renDatas);
			use_window(mainWin, CopyToMain, backWin);
		}
		else {
			pthread_mutex_unlock(&rDataLock);
			sched_yield();
		}
	}

	VBDestroy(buf);
	VBDestroy(buf+1);
	VBDestroy(buf+2);
	endwin();
}
