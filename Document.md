

# Client
## 참고 사항
Screen Size = 133 * 40

### Render Data 구조
#user|(score|color|#id|str_id|#points|points(X,Y world coord))|...|#star|stars(X,Y world coord)  
실제 데이터 바로 앞에 데이터의 전체 크기가 int형으로 온다.

## Function

### void* Render()
Rendering Thread의 함수

	lock
	copy render data to local
	unlock

	for (users) {
		save user scores
		save user id
		save camera pos
	}

	draw status bar

	for (users) {
		transform world coord to screen coord
		draw player
	}
	for (stars) {
		draw star
	}

	draw ranking bar

	refresh


### void* RecvMsg() 
서버의 메세지를 받는 Thread 함수

	while(1) {
		read 4byte for checking data size
		if(data size is lager than 0){
			lock
			if (data size is larger than prev size){
				realloc
			}
			recv data in buffer
			unlock
		}
	}

### void* SendMsg() 
서버에게 메세지를 보내는 Thread 함수
	wait user input
	check input is valid
	send data

### void DrawLine(WINDOW* win, Point start, Point end)
시작점과 끝점 사이에 선을 그린다. 시작점에는 그리지 않고 그 다음 칸부터 끝점까지 그린다.


### void TransformToScreen(Point base, Point* target)
주어진 기준점과 화면 크기를 이용해서 target의 좌표를 화면 좌표계로 변환한다.  
(target-base)+((WINDOW_SIZE-2)/2)+1


### void DrawStatusBar(WINDOW* win, Point pos, int score)
자신의 캐릭터의 위치와 점수를 상태표시줄에 그린다.


### void DrawRankingBar(WINDOW* win, const int* idxList, const size_t* idList, int playerNum)
idxList에 있는 상위 3 플레이어의 index를 통해 순위를 출력한다.  
총 플레이어가 3명 미만일 경우에는 없는 플레이어에 대해 문자열 길이를 0으로 표시한다.


### void ConnectToServer()
서버에 닉네임을 통해 접속. 닉네임은 전역변수에 저장됨.


### void* MovePointer(char** ptr, int offset)
### void* MovePointerStep(char** ptr, int step, int offset)
주어진 포인터를 이동해가며 offset만큼 읽고 반환

### VBuffer VBCreate(size_t size)
VBuffer 생성. size가 0이면 VB는 0으로 채워진다.

### int VBAppend(VBuffer* buf, void* source, size_t size)
VBuffer 뒤에 size만큼 source의 내용을 붙여넣는다.  
source가 NULL이면 size만큼의 공간을 확보한다.  
이미 공간이 충분하면 아무 일도 하지 않는다. VB 크기가 충분하지 않으면 realloc한다.

### int VBReplace(VBuffer* buf, void* source, size_t size)
VBuffer의 내용을 source의 size만큼으로 처음부터 채워 넣는다.  
VB 크기가 충분하지 않으면 free한 후 malloc한다.

### void VBDestroy(VBuffer* buf)
VBuffer를 제거한다. VB 안은 0으로 채워진다.

### int VBClear(VBuffer* buf)
VBuffer의 내용을 제거한다. 정확히는 len값을 초기화한다.

## Structure

	struct Point {
		int x, y;
	}

	struct VBuffer {
		char* head;
		size_t maxLen;
		size_t len;
	}
