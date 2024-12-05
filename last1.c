//미세먼지, 날씨 : 미세먼지센서, 온습도 센서, LCD 
// 가로등: 조도센서, LED 2개

#include <stdlib.h> //문자열 변환 헤더파일
#include <stdint.h> // 크기별로 정수 자료형이 정의된 헤더 파일
#include <lcd.h> //lcd 사용을 위한 헤더파일 포함
#include <stdio.h>  //입출력 관련 헤더파일 포함
#include <string.h> //문자열 관련 헤더파일 포함
#include <errno.h> //에러 관련 헤더파일 포함
#include <wiringPi.h> //GPIO 핀을 사용하기 위한 헤더파일
#include <wiringPiSPI.h> //GPIO access library 헤더파일 포함
#include <pthread.h> //thread를 사용하기 위한 헤더파일

//온습도 핀 설정
#define MAXTIMINGS 83 //80us
#define DHTPIN 17

//미세먼지 핀 설정
#define CS_MCP3208 8 //MCP3208 CS 핀 정의
#define SPI_CHANNEL 0 //SPI 통신 채널 정의
#define SPI_SPEED 1000000 //SPI 통신 속도 정의
#define LED_PIN 26 //LED 핀 설정

//가로등 LED핀 설정
#define LED_WHITE_1 23
#define LED_WHITE_2 24

//온습도 테이블 생성
int dht11_dat[5] = { 0, };

//온습도센서를 사용하기 위한 함수
void read_dht11_dat()
{
	uint8_t laststate = HIGH;
	uint8_t counter = 0;
	uint8_t j = 0, i;
        
       //8bit 습도. 8bit 습도, 8bit 온도, 8bit 온도, 8bit checksum
	dht11_dat[0] = dht11_dat[1] = dht11_dat[2] = dht11_dat[3] = dht11_dat[4] = 0;

	pinMode(DHTPIN, OUTPUT); //온습도 센서 출력 설정

        //18ms 동안 LOW 출력(MCU 시작 신호 전송)
	digitalWrite(DHTPIN, LOW);
	delay(18);

        //20~40us 동안 HIGH 출력
	digitalWrite(DHTPIN, HIGH);
	delayMicroseconds(30);

	pinMode(DHTPIN, INPUT); //온습도 센서 입력 설정

	for (i = 0; i < MAXTIMINGS; i++) {
		counter = 0;

		while (digitalRead(DHTPIN) == laststate) { //핀에서 LOW-HIGH 변화가 있을 때
			counter++; //us count
			delayMicroseconds(1);
			if (counter == 200) break; //counter 값이 200이상이면(HIGH-LOW 변화가 없으면) while문 빠져나옴
		}
		laststate = digitalRead(DHTPIN);

		if (counter == 200) break;

               /*Resp, Pull-up ready,Start to Tx를 제외하고 4번째부터 시작하여 받아들임,
                 짝수일 경우(=송신일경우)에만 값을 받아들임*/
		if ((i >= 4) && (i % 2 == 0)) {
			dht11_dat[j / 8] <<= 1;
			if (counter > 20) dht11_dat[j / 8] |= 1;
			j++;
		}
	}
}

//Mcp3208ADC를 사용하기 위한 함수
int ReadMcp3208ADC(unsigned char adcChannel)
{
       /* 0  0   0  0  0  Start  SGL  D2
          D1 D0  x  x  x    x     x    x
          x  x   x  x  x    x     x    x*/ 
	unsigned char buff[3]; //3Bytes Command Data
	int nAdcValue = 0;
	buff[0] = 0x06 | ((adcChannel & 0x07) >> 2);
	buff[1] = ((adcChannel & 0x07) << 6);
	buff[2] = 0x00;

	digitalWrite(CS_MCP3208, 0); //Chip Select Low 신호 출력
	wiringPiSPIDataRW(SPI_CHANNEL, buff, 3);//설정한 채널로부터 데이터 획득

	buff[1] = 0x0F & buff[1]; //하위 4자리 masking
	nAdcValue = (buff[1] << 8) | buff[2];//12비트 변환 값 저장
	digitalWrite(CS_MCP3208, 1); //Chip Select High신호 출력
	return nAdcValue; //변환 값 반환
}

//온도와 미세먼지를 LCD에 출력하기 위한 함수
void* weather_f(void* num) {
	int dustChannel = 3;
	float Vo_Val = 0;
	float Voltage = 0;
	float dustDensity = 0;

	int samplingTime = 280; //0.28ms
	int delayTime = 40; //0.32-0.28 = 0.04ms
	int offTime = 9600; //10-0.32 = 9.68ms
	int disp1; //lcd 사용 변수 

       //LCD핀 연결
	disp1 = lcdInit(2, 16, 4, 2, 4, 20, 21, 12, 16, 0, 0, 0, 0);
	lcdClear(disp1);

	pinMode(CS_MCP3208, OUTPUT);
	pinMode(LED_PIN, OUTPUT);

	while (1) {
		digitalWrite(LED_PIN, LOW); //IRED ON
		delayMicroseconds(samplingTime);
		Vo_Val = ReadMcp3208ADC(dustChannel); //sensor read

		delayMicroseconds(delayTime);
		digitalWrite(LED_PIN, HIGH); //IRED OFF
		delayMicroseconds(offTime);

               //미세먼지 값 계산
		Voltage = (Vo_Val*3.3 / 1024.0) / 3.0;
		dustDensity = (0.162*(Voltage)-0.0999) * 1000;

		read_dht11_dat(); //dht11값 read
		delay(60000);

		lcdClear(disp1); //lcd 초기값 clear
		if (dustDensity > 80) { //미세먼지 농도 80이상일 경우 "BAD" 출력
			lcdPosition(disp1, 0, 0);
			lcdPrintf(disp1, "Temp >> %d.%d", dht11_dat[2], dht11_dat[3]); //현재 온도 출력 
			lcdPosition(disp1, 0, 1);
			lcdPuts(disp1, "Dust >> BAD"); //미세먼지 상태 출력
		}
		else if (dustDensity <= 80) { //미세먼지 농도 80이하일 경우 "GOOD"출력
			lcdPosition(disp1, 0, 0);
			lcdPrintf(disp1, "Temp >> %d.%d", dht11_dat[2], dht11_dat[3]); //현재 온도 출력 
			lcdPosition(disp1, 0, 1);
			lcdPuts(disp1, "Dust >> GOOD"); //미세먼지 상태 출력
		}
		delay(60000);
	}
	return 0;
}

//조도 센서를 사용하기 위한 함수
void* light_f(void* num) {
	int nCdsChannel = 0; //ADC채널 변수 선언
	int nCdsValue = 0; //ADC 값 변수 선언

        //LED 초기값 설정 
	pinMode(LED_WHITE_1, OUTPUT);
	pinMode(LED_WHITE_2, OUTPUT);
	digitalWrite(LED_WHITE_1, LOW);
	digitalWrite(LED_WHITE_2, LOW);

	pinMode(CS_MCP3208, OUTPUT);   //MCP3208 CS 핀 출력 설정
	while (1)
	{
		nCdsValue = ReadMcp3208ADC(nCdsChannel);    //ADC 채널에 입력되는 값을 변환하여 ADC 값 변수에 저장

		if (nCdsValue < 1500) { //조도 센서 값이 1500 이하일 경우 LED ON
			digitalWrite(LED_WHITE_1, LOW); //컨버터를 거치면 원하는 값과 반대로 출력되어 //LOW로 작성 
			digitalWrite(LED_WHITE_2, LOW);
		}
		else { //조도 센서 값이 1500이상일 경우 LED OFF
			digitalWrite(LED_WHITE_1, HIGH); //컨버터를 거치면 원하는 값과 반대로 출력되어 //HIGH로 작성
			digitalWrite(LED_WHITE_2, HIGH);
		}

		delay(1000); //값 모니터링
	}
	return 0;
}

//main 함수(thread 사용)
int main() {

	pthread_t pthread_A, pthread_B;  //threadA, threadB 선언

	if (wiringPiSetupGpio() == -1) { //wiring Pi의 GPIO를 사용하기 위한 설정
		fprintf(stdout, "Not start wiringPi : %s\n", strerror(errno));
		return 1;
	}   
	if (wiringPiSPISetup(SPI_CHANNEL, SPI_SPEED) == -1) { //wiring Pi의 SPI를 사용하기 위한 설정
		fprintf(stdout, "wiringPiSPISetup Failed : %s\n", strerror(errno));
		return 1;
	}

	//thread A-온도,미세먼지 생성
	pthread_create(&pthread_A, NULL, weather_f, NULL);

	//thread B-가로등 생성
	pthread_create(&pthread_B, NULL, light_f, NULL);

	while (1) {
		delay(1);
	}
	return 1;
}

