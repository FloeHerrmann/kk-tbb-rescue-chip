#include <SPI.h>
#include <SD.h>
#include <TouchScreen.h>
#include <SoftwareSerial.h>
#include <GPRS_Shield_Arduino.h>
#include <Wire.h>
#include <Suli.h>
#include <Adafruit_GPS.h>

#define COLOR_BLUE 0x0339
#define COLOR_WHITE 0xFFFF
#define COLOR_GREY 0x8410
#define COLOR_YELLOW 0xffe0
#define COLOR_ORANGE 0xFBE0
#define COLOR_RED 0xf800
#define COLOR_GREEN 0x07E0

#define COLOR_BACKGROUND 0xFFFF
#define COLOR_FOREGROUND 0x0000

#define ALARM_TIME 5000
#define URGENT_TIME 10000
#define GPS_UPDATE_TIME 5000
#define DISPLAY_UPDATE_TIME 500

boolean clearButtonIsPressed;
boolean alertButtonIsPressed;

long clearButtonStartTime;
long alertButtonStartTime;

long clearButtonPressedTime;
long alertButtonPressedTime;

boolean isAlertState1;
boolean isAlertState2;
boolean isAlertClear;

ulong GPSTimer = millis();
ulong DisplayTimer = millis();

TouchScreen Display = TouchScreen( A0 , A1 , A2 , A3 );
GPRS GPRSModem( 51 , 49 , 19200 , "internet.t-mobile" , "t-mobile" , "tm" );
Adafruit_GPS GPS( &Serial3 );

void setup() {
	
	Serial.begin( 57600 );

	isAlertState1 = false;
	isAlertState2 = false;
	isAlertClear = true;

	Display.init();
	Display.setContentArea( 10 , 14 , 10 , 10 );
	Display.clear( );

	Display.drawStringCenter30px( "GPS" , 60 , COLOR_FOREGROUND , COLOR_BACKGROUND );
	Display.drawStringCenter30px( "RESCUE" , 140 , COLOR_FOREGROUND , COLOR_BACKGROUND );
	Display.drawStringCenter30px( "SYSTEM" , 220 , COLOR_FOREGROUND , COLOR_BACKGROUND );

	delay( 2000 );

	Display.clear( );

	uint yPosition = 10;

	Display.drawStringLeft8px( "Pin 17 als AUSGANG festlegen" , yPosition , COLOR_FOREGROUND , COLOR_BACKGROUND );
	pinMode( 9 , OUTPUT );
	Display.drawStringRight8px( "OK" , yPosition , COLOR_GREEN , COLOR_BACKGROUND );
	yPosition += 12;

	Display.drawStringLeft8px( "Pin 17 als AUSGANG festlegen" , yPosition , COLOR_FOREGROUND , COLOR_BACKGROUND );
	pinMode( 17 , OUTPUT );
	Display.drawStringRight8px( "OK" , yPosition , COLOR_GREEN , COLOR_BACKGROUND );
	yPosition += 12;

	Display.drawStringLeft8px( "Pin 18 als Eingang festlegen" , yPosition , COLOR_FOREGROUND , COLOR_BACKGROUND );
	pinMode( 18 , INPUT_PULLUP );
	clearButtonIsPressed = false;
	Display.drawStringRight8px( "OK" , yPosition , COLOR_GREEN , COLOR_BACKGROUND );
	yPosition += 12;

	Display.drawStringLeft8px( "Pin 19 als Eingang festlegen" , yPosition , COLOR_FOREGROUND , COLOR_BACKGROUND );
	pinMode( 19 , INPUT_PULLUP );
	alertButtonIsPressed = false;
	Display.drawStringRight8px( "OK" , yPosition , COLOR_GREEN , COLOR_BACKGROUND );
	yPosition += 12;
	
	Display.drawStringLeft8px( "Starte GPS Sensor" , yPosition , COLOR_FOREGROUND , COLOR_BACKGROUND );
	GPS.begin( 9600 );
	GPS.sendCommand( PMTK_SET_NMEA_OUTPUT_RMCGGA );
	GPS.sendCommand( PMTK_SET_NMEA_UPDATE_1HZ );
	GPS.sendCommand( PGCMD_ANTENNA );
	Display.drawStringRight8px( "OK" , yPosition , COLOR_GREEN , COLOR_BACKGROUND );
	yPosition += 12;

	Display.drawStringLeft8px( "Starte GPRS Modem" , yPosition , COLOR_FOREGROUND , COLOR_BACKGROUND );
	digitalWrite(9, HIGH);
	delay(1000);
	digitalWrite(9, LOW);
	Display.drawStringRight8px( "OK" , yPosition , COLOR_GREEN , COLOR_BACKGROUND );
	yPosition += 12;
	
	Display.drawStringLeft8px( "Einbuchen (20 Sekunden)" , yPosition , COLOR_FOREGROUND , COLOR_BACKGROUND );
	for( int i = 0 ; i < 20 ; i++ ) {
		Display.drawString8px( "." , 130 + i * 3 , yPosition , COLOR_FOREGROUND , COLOR_BACKGROUND );
		delay( 1000 );
	}
	if( 0 != GPRSModem.init() ) {
		Display.drawStringRight8px( "OK" , yPosition , COLOR_GREEN , COLOR_BACKGROUND );
	} else {
		Display.drawStringRight8px( "ERROR" , yPosition , COLOR_RED , COLOR_BACKGROUND );
	}

	delay( 1000 );
	Display.clear( );
	DrawGPSInfo();
	DrawAlertBar( alertButtonPressedTime );
	DrawClearBar( clearButtonPressedTime );

}

void loop() {

	char c = GPS.read();
	if( c ) Serial.print( c );

	if( GPS.newNMEAreceived() ) {
		if( !GPS.parse( GPS.lastNMEA() ) )
			return;
	}

	if( ( millis() - GPSTimer ) > GPS_UPDATE_TIME ) {
		GPSTimer = millis();
	}

	if( digitalRead(18) == LOW && clearButtonIsPressed == false) {
		clearButtonIsPressed = true;
		clearButtonStartTime = millis();
	} else if( digitalRead(18) == LOW && clearButtonIsPressed == true ) {
		clearButtonPressedTime = millis() - clearButtonStartTime;
	} else if( digitalRead(18) == HIGH && clearButtonIsPressed == true ) {
		clearButtonPressedTime = 0;
		clearButtonIsPressed = false;
	}

	if( digitalRead(19) == LOW && alertButtonIsPressed == false && isAlertState1 == false && isAlertState2 == false ) {
		alertButtonIsPressed = true;
		alertButtonStartTime = millis();
		if( isAlertClear == true ) {
			digitalWrite( 17 , HIGH );
			delay( 100 );
			digitalWrite( 17 , LOW );
		}
	} else if( digitalRead(19) == LOW && alertButtonIsPressed == true ) {
		alertButtonPressedTime = millis() - alertButtonStartTime;
	} else if( digitalRead(19) == HIGH && alertButtonIsPressed == true ) {
		//alertButtonPressedTime = millis() - alertButtonStartTime;
		//alertButtonPressedTime = 0;
		if( alertButtonPressedTime < ALARM_TIME ) alertButtonPressedTime = 0;
		if( alertButtonPressedTime >= ALARM_TIME && alertButtonPressedTime < URGENT_TIME ) alertButtonPressedTime = ALARM_TIME;
		if( alertButtonPressedTime >= URGENT_TIME ) alertButtonPressedTime = URGENT_TIME;
		alertButtonIsPressed = false;
	}

	if( ( millis() - DisplayTimer ) > DISPLAY_UPDATE_TIME ) {
		DrawGPSInfo();
		DrawAlertBar( alertButtonPressedTime );
		DrawClearBar( clearButtonPressedTime );
		//Serial.println( alertButtonPressedTime );
		DisplayTimer = millis();
	}

	if( alertButtonPressedTime >= ALARM_TIME && isAlertState1 == false ) {
		isAlertState1 = true;
		isAlertState2 = false;
		isAlertClear = false;
		digitalWrite( 17 , HIGH );
		delay( 300 );
		digitalWrite( 17 , LOW );
	} else if( alertButtonPressedTime >= URGENT_TIME && isAlertState2 == false && isAlertState1 == true ) {
		isAlertState1 = true;
		isAlertState2 = true;
		digitalWrite( 17 , HIGH );
		delay( 300 );
		digitalWrite( 17 , LOW );
		delay( 300 );
		digitalWrite( 17 , HIGH );
		delay( 300 );
		digitalWrite( 17 , LOW );
	}

	if( clearButtonPressedTime >= ALARM_TIME && isAlertClear == false ) {
		Serial.println( "Alert cleared" );
		isAlertState1 = false;
		isAlertState2 = false;
		isAlertClear = true;
		alertButtonPressedTime = 0;
		alertButtonIsPressed = false;
	}
}

void DrawGPSInfo(){
	uint yPosition = 10;

	Display.drawStringLeft8px( "Datum" , yPosition , COLOR_FOREGROUND , COLOR_BACKGROUND );
	String date = "        ";
	date = date + GPS.day;
	date = date + ".";
	if( GPS.month < 10 ) date = date + "0";
	date = date + GPS.month;
	date = date + ".20";
	date = date + GPS.year;
	Display.drawStringRight8px( date , yPosition , COLOR_FOREGROUND , COLOR_BACKGROUND );
	yPosition += 14;

	Display.drawStringLeft8px( "Uhrzeit" , yPosition , COLOR_FOREGROUND , COLOR_BACKGROUND );
	String time = "        ";
	if( GPS.hour < 10 ) time = time + "0";
	time = time + (GPS.hour+2);
	time = time + ":";
	if( GPS.minute < 10 ) time = time + "0";
	time = time + GPS.minute;
	time = time + ":";
	if( GPS.seconds < 10 ) time = time + "0";
	time = time + GPS.seconds;
	Display.drawStringRight8px( time , yPosition , COLOR_FOREGROUND , COLOR_BACKGROUND );
	yPosition += 14;

	Display.drawStringLeft8px( "Satteliten" , yPosition , COLOR_FOREGROUND , COLOR_BACKGROUND );
	String latitude = "    ";
	latitude = latitude + GPS.latitude;
	latitude = latitude + GPS.lat;
	Display.drawStringRight8px( latitude , yPosition , COLOR_FOREGROUND , COLOR_BACKGROUND );
	yPosition += 14;

	Display.drawStringLeft8px( "Langengrad" , yPosition , COLOR_FOREGROUND , COLOR_BACKGROUND );
	String longitude = "    ";
	longitude = longitude + GPS.longitude;
	longitude = longitude + GPS.lon;
	Display.drawStringRight8px( longitude , yPosition , COLOR_FOREGROUND , COLOR_BACKGROUND );
}

void DrawAlertBar( uint milliseconds ) {
	const uint width = 200;
	const uint height = 10;
	const uint yPos = 220;
	const uint xPos = 20;

	float factor = float(width)/100.0;
	float percentage = ( (float) milliseconds / (float) URGENT_TIME ) * 100.0;
	if( percentage > 100.0 ) percentage = 100.0;
	float fillWidth = float( percentage ) * factor;
	if( milliseconds == 0 ) {
		Display.fillRectangle( xPos + int(fillWidth) + 1 , yPos + 1 , width - int(fillWidth) - 2 , height - 2 , COLOR_BACKGROUND );
	} else if( milliseconds != 0 && milliseconds < ALARM_TIME ) {
		Display.fillRectangle( xPos + 1 , yPos + 1 , int( fillWidth ) - 2 , height - 2 , COLOR_BLUE );
		Display.fillRectangle( xPos + int(fillWidth) + 1 , yPos + 1 , width - int(fillWidth) - 2 , height - 2 , COLOR_BACKGROUND );
	} else if( milliseconds >= ALARM_TIME && milliseconds < URGENT_TIME ) {
		Display.fillRectangle( xPos + 1 , yPos + 1 , int( fillWidth ) - 2 , height - 2 , COLOR_ORANGE );
		Display.fillRectangle( xPos + int(fillWidth) + 1 , yPos + 1 , width - int(fillWidth) - 2 , height - 2 , COLOR_BACKGROUND );
	} else if( milliseconds >= URGENT_TIME ) {
		Display.fillRectangle( xPos + 1 , yPos + 1 , width - 2 , height - 2 , COLOR_RED );
	}
	
	Display.drawLine( xPos , yPos , xPos + width - 1 , yPos , COLOR_FOREGROUND );
	Display.drawLine( xPos , yPos + height , xPos + width - 1 , yPos + height , COLOR_FOREGROUND );

	Display.drawLine( xPos , yPos - 5 , xPos , yPos + height + 5 , COLOR_FOREGROUND );
	Display.drawLine( xPos + width , yPos , xPos + width , yPos + height , COLOR_FOREGROUND );

	Display.drawLine( xPos + width/2 , yPos - 5 , xPos + width/2 , yPos - 1 , COLOR_ORANGE );
	Display.drawLine( xPos + width/2 , yPos + height + 1 , xPos + width/2 , yPos + height + 5 , COLOR_ORANGE );
	
	Display.drawLine( xPos + width , yPos - 5 , xPos + 200 , yPos - 1 , COLOR_RED );
	Display.drawLine( xPos + width , yPos + height + 1 , xPos + 200 , yPos + height + 5 , COLOR_RED );
}

void DrawClearBar( uint milliseconds ) {
	const uint width = 200;
	const uint height = 10;
	const uint yPos = 245;
	const uint xPos = 20;

	float factor = float(width)/100.0;
	float percentage = ( (float) milliseconds / (float) ALARM_TIME ) * 100.0;
	//Serial.println( milliseconds );
	if( percentage > 100.0 ) percentage = 100.0;
	float fillWidth = float( percentage ) * factor;

	if( milliseconds == 0 ) {
		Display.fillRectangle( xPos + 1 , yPos + 1 , width - 2 , height - 2 , COLOR_BACKGROUND);
	} else if( milliseconds != 0 && milliseconds < ALARM_TIME ) {
		Display.fillRectangle( xPos + 1 , yPos + 1 , int( fillWidth ) - 2 , height - 2 , COLOR_BLUE );
		Display.fillRectangle( xPos + int(fillWidth) + 1 , yPos + 1 , width - int(fillWidth) - 2 , height - 2 , COLOR_BACKGROUND );
	} else if( milliseconds != 0  && milliseconds >= ALARM_TIME ) {
		Display.fillRectangle( xPos + 1 , yPos + 1 , width - 2 , height - 2 , COLOR_GREEN );
	}
	
	Display.drawLine( xPos , yPos , xPos + width - 1 , yPos , COLOR_FOREGROUND );
	Display.drawLine( xPos , yPos + height , xPos + width - 1 , yPos + height , COLOR_FOREGROUND );

	Display.drawLine( xPos , yPos - 5 , xPos , yPos + height + 5 , COLOR_FOREGROUND );
	Display.drawLine( xPos + width , yPos , xPos + width , yPos + height , COLOR_FOREGROUND );
	
	Display.drawLine( xPos + width , yPos - 5 , xPos + 200 , yPos - 1 , COLOR_GREEN );
	Display.drawLine( xPos + width , yPos + height + 1 , xPos + 200 , yPos + height + 5 , COLOR_GREEN );
}