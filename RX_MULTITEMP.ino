#include "LowPower.h"

#include <nRF24L01.h>
#include <RF24.h>
#include <Wire.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET 4
#define DEBUG 0
Adafruit_SSD1306 tft(OLED_RESET);

float temp1, temp2, temp3;

RF24 radio(9, 10);
const uint64_t pipes[3] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0E2LL, 0xF0F0F0F0E3LL };

void setup(void) {
//Serial.begin(9600);
tft.begin(SSD1306_SWITCHCAPVCC, 0x3C);
tft.display();
delay(20);
tft.clearDisplay();

radio.begin();
radio.setPALevel ( RF24_PA_MAX ) ;
radio.setDataRate(RF24_250KBPS);
radio.openReadingPipe(1, pipes[1]);
radio.openReadingPipe(2, pipes[2]);
radio.openReadingPipe(3, pipes[3]);
radio.startListening();




tft.setCursor(0,10);
tft.setTextSize(2);
tft.setTextColor(BLACK, WHITE);
tft.print("Wi-Fi");

tft.setCursor(0,30);
tft.setTextSize(1);
tft.setTextColor(BLACK, WHITE);
tft.print("Thermometer");

tft.display();
delay(2000);

}

void loop(void)
{

if ( radio.available() )
{
delay(50);
radio.read(&temp1, sizeof(temp1));//view themperature from sensor 1

Serial.println(temp1);

tft.setCursor(0,0);
tft.setTextSize(2);
tft.setTextColor(WHITE, BLACK);
tft.print("Wi-Fi");
tft.setTextSize(1);
tft.print("Thermometer");
tft.setCursor(0,17);
tft.setTextSize(1);
tft.print("                     ");
tft.setCursor(0,32);
tft.setTextSize(2);
tft.setTextColor(WHITE, BLACK);
tft.print("T1:");
tft.setTextSize(2);;
tft.print(temp1,1);
tft.print((char)247);
tft.print('C');

tft.display();
delay(50);
tft.clearDisplay();

radio.read(&temp2, sizeof(temp2));//view themperature from sensor 2

Serial.println(temp2);

tft.setCursor(0,50);
tft.setTextSize(2);
tft.setTextColor(WHITE, BLACK);
tft.print("T2:");
tft.setTextSize(2);
tft.print(temp2,1);
tft.print((char)247);
tft.print('C');
}
else
{
tft.setCursor(0,0);
tft.setTextSize(2);
tft.setTextColor(WHITE, BLACK);
tft.print("Wi-Fi");
tft.setTextSize(1);
tft.print("Thermometer");
tft.setCursor(0,17);
tft.setTextSize(1);
tft.print("_____________________");

tft.setCursor(0,32);
tft.setTextSize(2);
tft.setTextColor(WHITE, BLACK);
tft.print("T1:");
tft.setTextSize(2);;
tft.print(temp1,1);
tft.print((char)247);
tft.print('C');

tft.setCursor(0,50);
tft.setTextSize(2);
tft.setTextColor(WHITE, BLACK);
tft.print("T2:");
tft.setTextSize(2);
tft.print(temp2,1);
tft.print((char)247);
tft.print('C');

tft.display();
delay(50);
//tft.clearDisplay();
}

//delay(1000);
//LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
}
