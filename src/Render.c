#define NCURSES_WIDECHAR 1

#include <stdlib.h>
#include <malloc.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include "Render.h"
#include "Util.h"

#define BODY 'O'
#define STAR '*'

extern pthread_mutex_t rDataLock;
extern pthread_cond_t dataCopyCond;
WINDOW* mainWin;
extern VBuffer renderData;
extern const char* myID;

void TransformToScreen(Point base, Point* target) {
	target->x = (target->x - base.x)/10 + ((WINDOW_WIDTH-2)/2) + 1;
	target->y = (target->y - base.y)/10 + ((WINDOW_HEIGHT-2)/2) + 1;
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
		for (int j=0; j<len; ++j) {
			waddch(win, data[j]);
		}
		if (i<renderCount-1)
			waddstr(win, ", ");
	}
}

int ScoreCmp(const void* a, const void* b) {
	int aScore = ((int*)a)[1];
	int bScore = ((int*)b)[1];
	if (aScore > bScore) return -1;
	if (aScore < bScore) return 1;
	return 0;
}

void* Render() {
	int isColor;

	setlocale(LC_CTYPE, "ko_KR.utf-8");

	initscr();
	isColor = has_colors();
	if (isColor)
	{
		start_color();
		int ignore[] = {0, 16, 8};
		// COLOR_PAIR 번호는 1~227
		for (int i=0, j=0; i<227; ++i, ++j) {
			for (int t=0; t < (sizeof(ignore)/sizeof(*ignore)); ++t) {
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
	cbreak();
	curs_set(0);

	mainWin = newwin(WINDOW_HEIGHT, WINDOW_WIDTH, 0, 0);
	noecho();
	keypad(mainWin, TRUE);

	box(mainWin, 0, 0);

	VBuffer localBuf = VBCreate(50);
	VBuffer scores = VBCreate(80);
	VBuffer ids = VBCreate(40);	// [#len|data(without null)]
	while(1) {
		// 렌더링 데이터 복사
		pthread_mutex_lock(&rDataLock);
		pthread_cond_wait(&dataCopyCond, &rDataLock);
		VBReplace(&localBuf, renderData.ptr, renderData.len);
		pthread_mutex_unlock(&rDataLock);

		char* pData = localBuf.ptr;
		int nPlayer = *(int*)MovePointer(&pData, sizeof(int));
		char* pRender = pData;

		char playerID[11];
		int myIndex = -1;
		Point camPos;

		VBClear(&scores);
		VBClear(&ids);

		int i;

		// Get scores, ids, camera pos
		for (i=0; i<nPlayer; ++i) {
			VBAppend(&scores, &i, sizeof(int));
			VBAppend(&scores, (int*)MovePointer(&pData, sizeof(int)), sizeof(int));
			MovePointer(&pData, sizeof(int));
			int* idLen = (int*)MovePointer(&pData, sizeof(int));
			VBAppend(&ids, &idLen, sizeof(idLen));

			size_t rCount = (*idLen > 10)?10:*idLen;
			strncpy(playerID, (char*)MovePointer(&pData, *idLen), rCount);
			playerID[rCount] = 0;
			if (myIndex < 0 && strcmp(playerID, myID) == 0) {
				myIndex = i;
			}

			int nPoints = *(int*)MovePointer(&pData, sizeof(int));
			int j=0;
			if (myIndex == i) {
				camPos = *(Point*)MovePointer(&pData, sizeof(Point));
				j++;
			}

			for(; j<nPoints; ++j) {
				MovePointer(&pData, sizeof(Point));
			}
		}

		// Draw Status Bar
		if (myIndex >= 0)
			DrawStatusBar(mainWin, camPos, scores.ptr[myIndex]);

		// Draw players
		Point p1, p2;
		for (i=0; i<nPlayer; ++i) {
			MovePointer(&pRender, sizeof(int));
			int color = *(int*)MovePointer(&pRender, sizeof(int));
			int idLen = *(int*)MovePointer(&pRender, sizeof(int));
			char* id = (char*)MovePointer(&pRender, idLen);

			int nPoints = *(int*)MovePointer(&pRender, sizeof(int));
			p1 = *(Point*)MovePointer(&pRender, sizeof(Point));
			TransformToScreen(camPos, &p1);
			wattron(mainWin, COLOR_PAIR(1));
			mvwaddch(mainWin, p1.y, p1.x, BODY);
			wattroff(mainWin, COLOR_PAIR(1));

			wattron(mainWin, COLOR_PAIR(color));
			int j;
			for (j=1; j<nPoints; ++j) {
				p2 = *(Point*)MovePointer(&pRender, sizeof(Point));
				TransformToScreen(camPos, &p2);
				DrawLine(mainWin, p1, p2);
				p1 = p2;
			}
			wattroff(mainWin, COLOR_PAIR(color));
		}

		int nStar = *(int*)MovePointer(&pRender, sizeof(int));
		Point star;
		for (i=0; i<nStar; ++i) {
			star = *(Point*)MovePointer(&pRender, sizeof(Point));
			TransformToScreen(camPos, &star);
			mvwaddch(mainWin, star.y, star.x, STAR);
		}

		qsort(scores.ptr, scores.len/(sizeof(int)*2), sizeof(int)*2, ScoreCmp);
		DrawRankingBar(mainWin, (const int*)scores.ptr, (const size_t*)ids.ptr, nPlayer);

		wrefresh(mainWin);
	}

	VBDestroy(&scores);
	VBDestroy(&ids);
	VBDestroy(&localBuf);
	endwin();
}
