

# Client
## 참고 사항
Screen Size = 133 * 40

### Render Data 구조
#user|(score|color|#id|str_id|#points|points(X,Y world coord))|...|#star|stars(X,Y world coord)

## Function

### void* Render()
Rendering Thread의 함수

	lock
	copy render data to local
	unlock
	draw status bar
	transform my world coord to screen coord
	draw my character

	for (users) {
		transform world coord to screen coord
		draw player
	}
	for (stars) {
		draw star
	}
	for (three scores) {
		draw ranking bar
	}

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


### void DrawRankingBar(WINDOW* win, char* ids)
	상위 3 플레이어의 이름을 출력한다.
	총 플레이어가 3명 미만일 경우에는 없는 플레이어에 대해 문자열 길이를 0으로 표시한다.


### void ConnectToServer(const char* id);
	서버에 닉네임을 통해 접속

## Structure

struct Point {
	int x, y;
}





