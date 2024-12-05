// 비상벨 - 버튼 led_red buzzer  ring
// 휴지통 - 모터 초음파 2 led trash
// 시계 - 7-segment clock

#include <stdio.h>  //입출력 관련 헤더파일 포함
#include <wiringPi.h> //GPIO 핀을 사용하기 위한 헤더파일
#include <time.h> //time 라이브러리 사용
#include <softPwm.h> //서보모터 PWM을 제어하기 위한 헤더파일
#include <softTone.h> //Tone wave signal 관련 헤더파일 선언
#include <pthread.h> //thread를 사용하기 위한 헤더파일


//휴지통 서보모터 핀 정의 
#define SERVO 26

//휴지통 LED 핀 정의
#define LED_BLUE 13

//비상벨 LED
#define LED_RED 27 //LED 포트 RED핀 정의
#define KEYPAD_PB1 18  //KEYPAD 포트 BT1 핀 정의
#define MAX_KEY_BT_NUM 1 //KEYPAD 버튼 수 정의

#define BUZZER_PIN 5 //Buzzer Pin 정의

//7음계에서 각 음에 해당하는 주파수 값 정의
#define DO_L 523   //낮은 도
#define DO_H 1046 //높은 도

//사람인식 초음파 핀 정의
#define TP_s 14
#define EP_s 15

//높이인식 초음파 핀 정의
#define TP_h 23
#define EP_h 24

#define DIG_NUM    5
#define FND_NUM    9

/*시:분*/
/*00:00*/
#define DIG1 22   // '시'의 10자리 FND 핀 정의
#define DIG2 25   // '시'의 1의 자리 FND 핀 정의
#define FND_D5D6  12   // ":" 의 핀 정의
#define DIG3 2   // '분'의 10자리 FND 핀 정의 
#define DIG4 20   // '분'의 1의자리 FND 핀 정의 
#define DIG5 21  
//FND A~DP핀 정의
#define FND_A  10  
#define FND_B  6   
#define FND_C  16    
#define FND_D  4   
#define FND_E   3   
#define FND_F   9  
#define FND_G   17   
#define FND_DP  19  

//FND ON,OFF 정의
#define DIG_ON     1
#define DIG_OFF    0
#define FND_ON     0
#define FND_OFF    1

#define BUF_SIZE   5 //BUF_SIZE 정의

//초음파 센서를 이용한 거리 측정 함수_사람인식
float getDistance_s(void){
	float fDistance_s; //거리 값 변수
	int nStartTime, nEndTime; //측정 시간 변수
	
	digitalWrite(TP_s, LOW); //TP에 LOW(0) 출력
	delayMicroseconds(2); //2us
	
	digitalWrite(TP_s, HIGH); //TP에 HIGH(1) 출력
	delayMicroseconds(10); // 10us
	digitalWrite(TP_s, LOW); //TP에 LOW(0) 출력

	while (digitalRead(EP_s) == LOW); //EP을 읽어 LOW인 동안 대기
	nStartTime = micros(); //EP가 HIGH 일 때, 시간 측정
	while (digitalRead(EP_s) == HIGH); //EP을 읽어 HIGH인 동안 대기
	nEndTime = micros(); //EP가 LOW 일 때, 시간 측정
	fDistance_s = (nEndTime - nStartTime) *0.034 / 2.; //EP가 HIGH인 시간을 측정하여 거리 산출(cm)
	return fDistance_s; //거리 값 반환
}

//초음파 센서를 이용한 거리 측정 함수_높이인식
float getDistance_h(void){
	float fDistance_h; //거리 값 변수
	int nStartTime, nEndTime; ////측정 시간 변수

	digitalWrite(TP_h, LOW); //TP에 LOW(0) 출력
	delayMicroseconds(2); //2us
	
	digitalWrite(TP_h, HIGH); //TP에 HIGH(1) 출력
	delayMicroseconds(10); // 10us
	digitalWrite(TP_h, LOW); //TP에 LOW(0) 출력

	while (digitalRead(EP_h) == LOW); //EP을 읽어 LOW인 동안 대기
	nStartTime = micros(); //EP가 HIGH 일 때, 시간 측정
	while (digitalRead(EP_h) == HIGH); //EP을 읽어 HIGH인 동안 대기
	nEndTime = micros(); //EP가 LOW 일 때, 시간 측정
	fDistance_h = (nEndTime - nStartTime) *0.034 / 2.; //EP가 HIGH인 시간을 측정하여 거리 산출(cm)
	return fDistance_h; //거리 값 반환
}

void initDigit();
void initFnd();
void displayValue(int value[]);
void displayDigitValue(int value);

//  FND     =  A  B  C  D  E  F  G
int ZERO[] = { 1, 1, 1, 1, 1, 1, 0 }; //0일 때 
int ONE[] = { 0, 1, 1, 0, 0, 0, 0 }; //1일 때 
int TWO[] = { 1, 1, 0, 1, 1, 0, 1 }; //2일 때 
int THREE[] = { 1, 1, 1, 1, 0, 0, 1 }; //3일 때 
int FOUR[] = { 0, 1, 1, 0, 0, 1, 1 }; //4일 때 
int FIVE[] = { 1, 0, 1, 1, 0, 1, 1 }; //5일 때 
int SIX[] = { 1, 0, 1, 1, 1, 1, 1 }; //6일 때 
int SEVEN[] = { 1, 1, 1, 0, 0, 1, 0 }; //7일 때 
int EIGHT[] = { 1, 1, 1, 1, 1, 1, 1 }; //8일 때 
int NINE[] = { 1, 1, 1, 1, 0, 1, 1 }; //9일 때 

/*FND를 사용하기 위한 테이블 선언*/
int segments[] = { FND_A, FND_B, FND_C, FND_D, FND_E, FND_F, FND_G, FND_DP, FND_D5D6 };
int digits[] = { DIG1, DIG2, DIG3, DIG4, DIG5 };

/*FND num(0~9) 테이블 선언*/
int nums[10][7] = {
   {1, 1, 1, 1, 1, 1, 0},
   {0, 1, 1, 0, 0, 0, 0},
   {1, 1, 0, 1, 1, 0, 1},
   {1, 1, 1, 1, 0, 0, 1},
   {0, 1, 1, 0, 0, 1, 1},
   {1, 0, 1, 1, 0, 1, 1},
   {1, 0, 1, 1, 1, 1, 1},
   {1, 1, 1, 0, 0, 1, 0},
   {1, 1, 1, 1, 1, 1, 1},
   {1, 1, 1, 1, 0, 1, 1}

};
int *display[4];

/*DIG 초기화_초기값 OFF*/
void initDigit() {
	int i;
	for (i = 0; i < DIG_NUM; i++) {
		digitalWrite(digits[i], DIG_OFF);
	}
}

/*FND 초기화_초기값 OFF*/
void initFnd() {
	int i;
	for (i = 0; i < FND_NUM; i++) {
		digitalWrite(segments[i], FND_OFF);
	}
}

/*FND ON, OFF 함수*/
void displayValue(int value[]) {
	int i;
	for (i = 0; i < 7; i++) {
		if (value[i] == 1) {
			digitalWrite(segments[i], FND_ON);
		}
		else if (value[i] == 0) {
			digitalWrite(segments[i], FND_OFF);
		}
	}
}
//각 숫자에 해당하는 출력을 선택하는 함수
void displayDigitValue(int value) {
	switch (value) {
	case 0:
		displayValue(ZERO);
		break;
	case 1:
		displayValue(ONE);
		break;
	case 2:
		displayValue(TWO);
		break;
	case 3:
		displayValue(THREE);
		break;
	case 4:
		displayValue(FOUR);
		break;
	case 5:
		displayValue(FIVE);
		break;
	case 6:
		displayValue(SIX);
		break;
	case 7:
		displayValue(SEVEN);
		break;
	case 8:
		displayValue(EIGHT);
		break;
	case 9:
		displayValue(NINE);
		break;
	}
}

unsigned int SevenScale(unsigned char scale) //각 음계에 해당하는 주파수 값을 선택하는 함수
{
	unsigned int _ret = 0;

	switch (scale)	{
	case 0: //낮은 도
		_ret = DO_L; 
		break;
	case 1: //높은 도
		_ret = DO_H;
		break;
	}
	return _ret;
}

void Change_FREQ(unsigned int freq)  //주파수를 변경하는 함수
{
	softToneWrite(BUZZER_PIN, freq);
}

void STOP_FREQ(void)  //주파수 발생 정지 함수
{
	softToneWrite(BUZZER_PIN, 0);
}
void Buzzer_Init(void) //Buzzer 초기 설정
{
	softToneCreate(BUZZER_PIN);
	STOP_FREQ();
}

const int KeypadTable[1] = { KEYPAD_PB1 }; //KEYPAD 핀 테이블 선언

//KEYPAD 버튼 눌림을 확인하여 nKeypadstate 상태 변수에 값을 저장
int KeypadRead(void)   
{
	int i, nKeypadstate;
	nKeypadstate = 0;
	for (i = 0; i < MAX_KEY_BT_NUM; i++) {
		if (!digitalRead(KeypadTable[i])) {
			nKeypadstate |= (1 << i);
		}
	}
	return nKeypadstate;
}

//휴지통 함수
void* trash_f(void* num) {
	pinMode(LED_BLUE, OUTPUT); //LED
	digitalWrite(LED_BLUE, LOW);
	softPwmCreate(SERVO, 0, 200); //서보모터

	//사람초음파
	pinMode(TP_s, OUTPUT);  //Trigger 핀 OUTPUT 설정
	pinMode(EP_s, INPUT);   //Echo 핀 INPUT 설정
	//높이초음파
	pinMode(TP_h, OUTPUT); //Trigger 핀 OUTPUT 설정
	pinMode(EP_h, INPUT); //Echo 핀 INPUT 설정

	while (1) {
		float fDistance_s = getDistance_s(); //사람 인식_함수로부터 반환된 거리 값 저장
		float fDistance_h = getDistance_h(); //높이 인식_함수로부터 반환된 거리 값 저장
		
		/*사람 인식 10cm 미만, 높이 인식 3cm 미만일 경우*/
		if (fDistance_s < 10 && fDistance_h < 3) { 
			digitalWrite(LED_BLUE, HIGH); //LED ON
			softPwmWrite(SERVO, 25); //휴지통 OPEN
			delay(2000);//2초 후 휴지통 다시 CLOSE
			softPwmWrite(SERVO, 15);
		}

		/*사람 인식 10cm 미만, 높이 인식 3cm 이상일 경우*/
		else if (fDistance_s < 10 && fDistance_h >= 3) {
			digitalWrite(LED_BLUE, LOW);  //LED OFF
			softPwmWrite(SERVO, 25); //휴지통 OPEN
			delay(2000); //2초 후 휴지통 다시 CLOSE
			softPwmWrite(SERVO, 15);
		}

		/*사람 인식 10cm 이상, 높이 인식 3cm 미만일 경우*/
		else if (fDistance_s >= 10 && fDistance_h < 3) {
			digitalWrite(LED_BLUE, HIGH);  //LED ON
		}

		/*사람 인식 10cm 이상, 높이 인식 3cm 이상일 경우*/
		else if (fDistance_s >= 10 && fDistance_h >= 3) {
			digitalWrite(LED_BLUE, LOW);  //LED OFF
		}
	}
}

//시간 함수
void* clock_f(void* num) {
	time_t timer; //시간 저장 변수
	char strFormat[BUF_SIZE];
	int digitNum[4];
	int i = 0;
	int j = 0;
	
	wiringPiSetupGpio(); //GPIO를 사용하기 위함
	for (i = 0; i < DIG_NUM; i++) { //DIG 출력 설정
		pinMode(digits[i], OUTPUT);
	}
	for (i = 0; i < FND_NUM; i++) { //FND 출력 설정
		pinMode(segments[i], OUTPUT);
	}

	initDigit();
	initFnd();

	while (1) {
		time(&timer); //시간 추출

		strftime(strFormat, BUF_SIZE, "%H%M", localtime(&timer)); // 지역 시간 hour/minute 추출 
		//반복문을 통하여 추출한 시간에 대해 출력
		for (i = 0; i < 4; i++) { //DIG1~4
			display[i] = nums[strFormat[i] - '0'];
			for (j = 0; j < 7; j++) { //FNDA~G
				if (display[i][j] == 1) { //시간에 해당하는 숫자 FND ON
					digitalWrite(segments[j], FND_ON);
				}
				else { //시간에 해당하지 않는 FND OFF
					digitalWrite(segments[j], FND_OFF);
				}
			}
			digitalWrite(digits[i], DIG_ON);
			delay(0.1);  //Interval 
			digitalWrite(digits[i], DIG_OFF);

			digitalWrite(FND_D5D6, FND_ON); // ':' ON
			digitalWrite(DIG5, DIG_ON); //DIG5 ON
			delay(0.1); //Interval 
			digitalWrite(DIG5, DIG_OFF); //DIG5 OFF
		}
	}
}
//비상벨 함수
void* ring_f(void* num) {
	pinMode(KEYPAD_PB1, INPUT); //Button 입력 설정
	pinMode(LED_RED, OUTPUT); //LED 출력 설정
	digitalWrite(LED_RED, LOW); //LED 초기값 OFF
	Buzzer_Init();

	int i;
	int j = 0;
	int nKeypadstate;
	

	for (i = 0; i < MAX_KEY_BT_NUM; i++) { //KEYPAD 핀 입력 설정
		pinMode(KeypadTable[i], INPUT);
	}
	while (1) {
		digitalWrite(LED_RED, LOW);
		STOP_FREQ();
		nKeypadstate = KeypadRead(); //KEYPAD로부터 버튼 입력을 읽어 상태를 변수에 저장
		for (i = 0; i < MAX_KEY_BT_NUM; i++) {

			if ((nKeypadstate & (1 << i))) { //비상벨이 눌렸을 경우
					for (i = 0; i < 5; i++) { //Buzzer 소리와 LED 반복
						digitalWrite(LED_RED, HIGH); //LED ON
						Change_FREQ(SevenScale(1)); //높은 도
						delay(500); //0.5초
						STOP_FREQ();

						digitalWrite(LED_RED, LOW); //LED OFF
						Change_FREQ(SevenScale(0)); //낮은 도
						delay(500); //0.5초
						STOP_FREQ();
					}
					digitalWrite(LED_RED, LOW); //LED OFF로 종료
			}
		}
	}
	return 0;
}

int main() {
	//threadA, threadB, threadC 선언
	pthread_t pthread_A, pthread_B, pthread_C;

	if (wiringPiSetupGpio() == -1) return 1; //Wiring Pi의 GPIO를 사용하기 위한 설정

	//thread A - 휴자통 생성
	pthread_create(&pthread_A, NULL, trash_f, NULL);
	//thread A - 시계 생성
	pthread_create(&pthread_B, NULL, clock_f, NULL);
	//thread A - 비상벨 생성
	pthread_create(&pthread_C, NULL, ring_f, NULL);

	while (1) {
		delay(1);
	}
	return 1;
}






