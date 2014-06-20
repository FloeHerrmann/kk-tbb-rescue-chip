#include <SPI.h>
#include <SD.h>
#include <TouchScreen.h>
#include <SoftwareSerial.h>
#include <GPRS_Shield_Arduino.h>
#include <Wire.h>
#include <Suli.h>
#include <Adafruit_GPS.h>
#include <Ethernet.h>
#include <IniFile.h>

#define COLOR_BLUE 0x0339
#define COLOR_WHITE 0xFFFF
#define COLOR_GREY 0x8410
#define COLOR_YELLOW 0xffe0
#define COLOR_ORANGE 0xFBE0
#define COLOR_RED 0xf800
#define COLOR_GREEN 0x07E0

#define COLOR_BACKGROUND 0xFFFF
#define COLOR_FOREGROUND 0x0000
#define COLOR_FOREGROUND_2 0x0339

#define ALARM_TIME 5000
#define URGENT_TIME 10000
#define GPS_UPDATE_TIME 50000
#define DISPLAY_UPDATE_TIME 500
#define QUALITY_UPDATE_TIME 300000

#define MARGIN_TOP 10
#define MARGIN_RIGHT 16
#define MARGIN_BOTTOM 10
#define MARGIN_LEFT 10

#define SD_SELECT_PIN 4

String ALARM_NUMBER = "01624220560";
String URGENT_NUMBER = "01785516487";

boolean clearButtonIsPressed;
boolean alertButtonIsPressed;

long clearButtonStartTime;
long alertButtonStartTime;

long clearButtonPressedTime;
long alertButtonPressedTime;

boolean isAlertState1;
boolean isAlertState2;
boolean isAlertClear;

String mobileIMEI;
int mobileSignal;

ulong GPSTimer = millis();
ulong DisplayTimer = millis();
ulong QualityTimer = millis();
bool firstGPSData = false;

TouchScreen Display = TouchScreen( A0 , A1 , A2 , A3 );
GPRS GPRSModem( 13 , 12 , 19200 , "internet.t-mobile" , "t-mobile" , "tm" );
Adafruit_GPS GPS( &Serial3 );

void setup() {

	isAlertState1 = false;
	isAlertState2 = false;
	isAlertClear = true;

	SPI.begin();

	Display.init();
	Display.setContentArea( MARGIN_TOP , MARGIN_RIGHT , MARGIN_BOTTOM , MARGIN_LEFT );
	Display.clear( );

	Display.drawStringCenter30px( "GPS" , 60 , COLOR_FOREGROUND , COLOR_BACKGROUND );
	Display.drawStringCenter30px( "RESCUE" , 140 , COLOR_FOREGROUND , COLOR_BACKGROUND );
	Display.drawStringCenter30px( "SYSTEM" , 220 , COLOR_FOREGROUND , COLOR_BACKGROUND );

	delay( 2000 );

	Display.clear( );

	uint yPosition = 18;
	uint yInterval = 18;

	Display.drawStringLeft8px( "Pin 4 als AUSGANG festlegen" , yPosition , COLOR_FOREGROUND , COLOR_BACKGROUND );
	pinMode( SD_SELECT_PIN , OUTPUT );
	digitalWrite( SD_SELECT_PIN , HIGH );
	Display.drawStringRight8px( "OK" , yPosition , COLOR_GREEN , COLOR_BACKGROUND );
	yPosition += yInterval;

	Display.drawStringLeft8px( "Pin 17 als AUSGANG festlegen" , yPosition , COLOR_FOREGROUND , COLOR_BACKGROUND );
	pinMode( 9 , OUTPUT );
	Display.drawStringRight8px( "OK" , yPosition , COLOR_GREEN , COLOR_BACKGROUND );
	yPosition += yInterval;

	Display.drawStringLeft8px( "Pin 17 als AUSGANG festlegen" , yPosition , COLOR_FOREGROUND , COLOR_BACKGROUND );
	pinMode( 17 , OUTPUT );
	Display.drawStringRight8px( "OK" , yPosition , COLOR_GREEN , COLOR_BACKGROUND );
	yPosition += yInterval;

	Display.drawStringLeft8px( "Pin 18 als Eingang festlegen" , yPosition , COLOR_FOREGROUND , COLOR_BACKGROUND );
	pinMode( 18 , INPUT_PULLUP );
	clearButtonIsPressed = false;
	Display.drawStringRight8px( "OK" , yPosition , COLOR_GREEN , COLOR_BACKGROUND );
	yPosition += yInterval;

	Display.drawStringLeft8px( "Pin 19 als Eingang festlegen" , yPosition , COLOR_FOREGROUND , COLOR_BACKGROUND );
	pinMode( 19 , INPUT_PULLUP );
	alertButtonIsPressed = false;
	Display.drawStringRight8px( "OK" , yPosition , COLOR_GREEN , COLOR_BACKGROUND );
	yPosition += yInterval;

	Display.drawStringLeft8px( "INI Datei lesen" , yPosition , COLOR_FOREGROUND , COLOR_BACKGROUND );
	if( !SD.begin( SD_SELECT_PIN ) ){
		Display.drawStringRight8px( "ERROR" , yPosition , COLOR_RED , COLOR_BACKGROUND );
	} else {
		const char *fileName = "config.ini";
		IniFile ini( fileName );
		if( !ini.open() ) {
			Display.drawStringRight8px( "ERROR" , yPosition , COLOR_RED , COLOR_BACKGROUND );
		} else {
			Display.drawStringRight8px( "OK" , yPosition , COLOR_GREEN , COLOR_BACKGROUND );
			const size_t bufferLen = 200;
 			char buffer[ bufferLen ];
 			yPosition += yInterval;
 			Display.drawStringLeft8px( "INI Datei validieren" , yPosition , COLOR_FOREGROUND , COLOR_BACKGROUND );
			if( !ini.validate( buffer , bufferLen ) ) {
				Display.drawStringRight8px( "ERROR" , yPosition , COLOR_RED , COLOR_BACKGROUND );
			} else {
				Display.drawStringRight8px( "OK" , yPosition , COLOR_GREEN , COLOR_BACKGROUND );

				yPosition += yInterval;
 				Display.drawStringLeft8px( "Telefonnummer 1 einlesen" , yPosition , COLOR_FOREGROUND , COLOR_BACKGROUND );
				if( ini.getValue( "settings" , "phone_01" , buffer , bufferLen ) ) {
					ALARM_NUMBER = String( buffer );
					Display.drawStringRight8px( "OK" , yPosition , COLOR_GREEN , COLOR_BACKGROUND );
				} else {
					Display.drawStringRight8px( "ERROR" , yPosition , COLOR_RED , COLOR_BACKGROUND );
				}

				yPosition += yInterval;
 				Display.drawStringLeft8px( "Telefonnummer 2 einlesen" , yPosition , COLOR_FOREGROUND , COLOR_BACKGROUND );
				if( ini.getValue( "settings" , "phone_02" , buffer , bufferLen ) ) {
					URGENT_NUMBER = String( buffer );
					Display.drawStringRight8px( "OK" , yPosition , COLOR_GREEN , COLOR_BACKGROUND );
				} else {
					Display.drawStringRight8px( "ERROR" , yPosition , COLOR_RED , COLOR_BACKGROUND );
				}
			}
		}
	}
	yPosition += yInterval;

	
	Display.drawStringLeft8px( "Starte GPS Sensor" , yPosition , COLOR_FOREGROUND , COLOR_BACKGROUND );
	GPS.begin( 9600 );
	GPS.sendCommand( PMTK_SET_NMEA_OUTPUT_RMCGGA );
	GPS.sendCommand( PMTK_SET_NMEA_UPDATE_1HZ );
	GPS.sendCommand( PGCMD_ANTENNA );
	Display.drawStringRight8px( "OK" , yPosition , COLOR_GREEN , COLOR_BACKGROUND );
	yPosition += yInterval;

	Display.drawStringLeft8px( "Starte GPRS Modem" , yPosition , COLOR_FOREGROUND , COLOR_BACKGROUND );
	digitalWrite( 9 , HIGH );
	delay( 1000 );
	digitalWrite( 9 , LOW );
	Display.drawStringRight8px( "OK" , yPosition , COLOR_GREEN , COLOR_BACKGROUND );
	yPosition += yInterval;

	Display.drawStringLeft8px( "Einbuchen (20 Sekunden)" , yPosition , COLOR_FOREGROUND , COLOR_BACKGROUND );
	for( int i = 0 ; i < 20 ; i++ ) {
		Display.drawString8px( "." , 130 + i * 3 , yPosition , COLOR_FOREGROUND , COLOR_BACKGROUND );
		delay( 1000 );
	}
	if( 0 != GPRSModem.init() ) {
		Display.drawStringRight8px( "ERROR" , yPosition , COLOR_RED , COLOR_BACKGROUND );
	} else {
		Display.drawStringRight8px( "OK" , yPosition , COLOR_GREEN , COLOR_BACKGROUND );
	}
	yPosition += yInterval;

	Display.drawStringLeft8px( "IMEI abfragen" , yPosition , COLOR_FOREGROUND , COLOR_BACKGROUND );
	mobileIMEI = GPRSModem.IMEI();
	Display.drawStringRight8px( "OK" , yPosition , COLOR_GREEN , COLOR_BACKGROUND );
	yPosition += yInterval;

	Display.drawStringLeft8px( "Signalstarke abfragen" , yPosition , COLOR_FOREGROUND , COLOR_BACKGROUND );
	mobileSignal = GPRSModem.SignalQuality();
	Display.drawStringRight8px( "OK" , yPosition , COLOR_GREEN , COLOR_BACKGROUND );
	yPosition += yInterval;

	delay( 1000 );
	Display.clear( );
	DrawGPSInfo();
	DrawGPRSInfo();
	DrawAlertBar( alertButtonPressedTime );
	DrawClearBar( clearButtonPressedTime );
}

void loop() {

	char c = GPS.read();

	if( GPS.newNMEAreceived() ) {
		GPS.parse( GPS.lastNMEA() );
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
		if( alertButtonPressedTime < ALARM_TIME ) alertButtonPressedTime = 0;
		if( alertButtonPressedTime >= ALARM_TIME && alertButtonPressedTime < URGENT_TIME ) alertButtonPressedTime = ALARM_TIME;
		if( alertButtonPressedTime >= URGENT_TIME ) alertButtonPressedTime = URGENT_TIME;
		SendAlertTextMessage();
		alertButtonIsPressed = false;
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
		isAlertState1 = false;
		isAlertState2 = false;
		isAlertClear = true;
		alertButtonPressedTime = 0;
		alertButtonIsPressed = false;
		Display.fillRectangle( 0 , 250 , 240 , 60 , COLOR_WHITE );
		digitalWrite( 17 , HIGH );
		delay( 300 );
		digitalWrite( 17 , LOW );
	}

	if( ( millis() - DisplayTimer ) > DISPLAY_UPDATE_TIME ) {
		DrawAlertBar( alertButtonPressedTime );
		DrawClearBar( clearButtonPressedTime );
		DisplayTimer = millis();
	}

	if( ( millis() - QualityTimer ) > QUALITY_UPDATE_TIME ) {
		mobileSignal = GPRSModem.SignalQuality();
		DrawGPRSInfo();
		QualityTimer = millis();
	}

	if( ( millis() - GPSTimer ) > GPS_UPDATE_TIME ) {
		DrawGPSInfo();
		GPSTimer = millis();
	}	
}

void DrawGPSInfo(){
	uint yPosition = 18;
	uint yInterval = 18;

	Display.drawStringLeft8px( "Datum" , yPosition , COLOR_FOREGROUND , COLOR_BACKGROUND );
	String date = "        ";
	date = date + GPS.day;
	date = date + ".";
	if( GPS.month < 10 ) date = date + "0";
	date = date + GPS.month;
	date = date + ".20";
	date = date + GPS.year;
	Display.drawStringRight8px( date , yPosition , COLOR_FOREGROUND_2 , COLOR_BACKGROUND );
	yPosition += yInterval;

	Display.drawStringLeft8px( "Uhrzeit" , yPosition , COLOR_FOREGROUND , COLOR_BACKGROUND );
	String time = "        ";
	if( ( GPS.hour + 2 ) < 10 ) time = time + "0";
	time = time + (GPS.hour+2);
	time = time + ":";
	if( GPS.minute < 10 ) time = time + "0";
	time = time + GPS.minute;
	time = time + ":";
	if( GPS.seconds < 10 ) time = time + "0";
	time = time + GPS.seconds;
	Display.drawStringRight8px( time , yPosition , COLOR_FOREGROUND_2 , COLOR_BACKGROUND );
	yPosition += yInterval;

	Display.drawStringLeft8px( "Satelliten" , yPosition , COLOR_FOREGROUND , COLOR_BACKGROUND );
	String satellites = "    ";
	satellites = satellites + GPS.satellites;
	Display.drawStringRight8px( satellites , yPosition , COLOR_FOREGROUND_2 , COLOR_BACKGROUND );
	yPosition += yInterval;

	CordinateToString( GPS.latitude );

	Display.drawStringLeft8px( "Breitengrad" , yPosition , COLOR_FOREGROUND , COLOR_BACKGROUND );
	String latitude = "    ";
	latitude = latitude + CordinateToString( GPS.latitude );
	latitude = latitude + GPS.lat;
	Display.drawStringRight8px( latitude , yPosition , COLOR_FOREGROUND_2 , COLOR_BACKGROUND );
	yPosition += yInterval;

	CordinateToString( GPS.longitude );

	Display.drawStringLeft8px( "Langengrad" , yPosition , COLOR_FOREGROUND , COLOR_BACKGROUND );
	String longitude = "    ";
	longitude = longitude + CordinateToString( GPS.longitude );
	longitude = longitude + GPS.lon;
	Display.drawStringRight8px( longitude , yPosition , COLOR_FOREGROUND_2 , COLOR_BACKGROUND );
	yPosition += yInterval;

	Display.drawLine( MARGIN_LEFT ,  yPosition , 240 - MARGIN_RIGHT , yPosition ,  COLOR_FOREGROUND );
}

void DrawGPRSInfo(){
	uint yPosition = 118;
	uint yInterval = 18;

	Display.drawStringLeft8px( "IMEI" , yPosition , COLOR_FOREGROUND , COLOR_BACKGROUND );
	Display.drawStringRight8px( mobileIMEI , yPosition , COLOR_FOREGROUND_2 , COLOR_BACKGROUND );
	yPosition += yInterval;

	Display.drawStringLeft8px( "Dampfung" , yPosition , COLOR_FOREGROUND , COLOR_BACKGROUND );
	int signalLossI = mobileSignal * 2 - 113;
	if( mobileSignal == 99 ) {
		Display.drawStringRight8px( "-dBm" , yPosition , COLOR_RED , COLOR_BACKGROUND );
	} else {
		String signalLossS = "";
		signalLossS = signalLossS + signalLossI;
		signalLossS = signalLossS + "dBm";
		Display.drawStringRight8px( signalLossS , yPosition , COLOR_FOREGROUND_2 , COLOR_BACKGROUND );
	}
	yPosition += yInterval;

	int Width = 208;
	if( mobileSignal == 99 ) {
		Display.drawRectangle( MARGIN_LEFT , yPosition , 240 - MARGIN_RIGHT , 10 , COLOR_FOREGROUND );
		Display.fillRectangle( MARGIN_LEFT + 1 , yPosition + 1 , 240 - MARGIN_RIGHT - 2 , 8 , COLOR_BACKGROUND );
	} else {
		float Factor = float( Width ) / 100.0;
		float FillWidth = ( float( ( -113 - signalLossI ) * -1) / 0.62 ) * Factor;
		Display.drawRectangle( MARGIN_LEFT , yPosition , Width + 2 , 10 , COLOR_FOREGROUND );
		Display.fillRectangle( MARGIN_LEFT + 1 , yPosition + 1 , FillWidth , 8 , COLOR_FOREGROUND_2 );
		Display.fillRectangle( MARGIN_LEFT + 1 + FillWidth , yPosition + 1 , Width - FillWidth , 8 , COLOR_BACKGROUND );
	}
	yPosition += yInterval;

	Display.drawLine( MARGIN_LEFT ,  yPosition , 240 - MARGIN_RIGHT , yPosition ,  COLOR_FOREGROUND );
}

void DrawAlertBar( uint milliseconds ) {
	const uint width = 240 - MARGIN_LEFT - MARGIN_RIGHT;
	const uint height = 10;
	const uint yPos = 200;
	const uint xPos = MARGIN_LEFT;

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
	
	Display.drawLine( xPos + width , yPos - 5 , xPos + width , yPos - 1 , COLOR_RED );
	Display.drawLine( xPos + width , yPos + height + 1 , xPos + width , yPos + height + 5 , COLOR_RED );
}

void DrawClearBar( uint milliseconds ) {
	const uint width = 240 - MARGIN_LEFT - MARGIN_RIGHT;
	const uint height = 10;
	const uint yPos = 225;
	const uint xPos = MARGIN_LEFT;

	float factor = float(width)/100.0;
	float percentage = ( (float) milliseconds / (float) ALARM_TIME ) * 100.0;
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
	
	Display.drawLine( xPos + width , yPos - 5 , xPos + width , yPos - 1 , COLOR_GREEN );
	Display.drawLine( xPos + width , yPos + height + 1 , xPos + width , yPos + height + 5 , COLOR_GREEN );
}

void SendAlertTextMessage(){
	char MessageArray[160];
	char NumberArray[160];
	if( alertButtonPressedTime >= ALARM_TIME && alertButtonPressedTime < URGENT_TIME ) {
		String Message = "";
		String latitude = CordinateToString( GPS.latitude );
		String longitude = CordinateToString( GPS.longitude );
		Message = "Hilfe: Ich hatte einen Unfall!\r\nLangengrad: ";
		Message += latitude;
		Message += GPS.lat;
		Message += "\r\nBreitengrad: ";
		Message += longitude;
		Message += GPS.lon;
		Message += "\r\nIMEI: ";
		Message += mobileIMEI;
		Message += "\r\nAnfordern: POLIZEI ";

		String Output = "SMS an ";
		Output += ALARM_NUMBER;
		Display.drawStringLeft8px( Output , 250 , COLOR_FOREGROUND , COLOR_BACKGROUND );
		Message.toCharArray( MessageArray , Message.length() + 1 );
		ALARM_NUMBER.toCharArray( NumberArray , ALARM_NUMBER.length() + 1 );
		int check = GPRSModem.sendSMS( NumberArray , MessageArray );

		delay( 1000 );

		Message = "Google Maps: http://maps.google.de/maps?q=";
		Message += latitude;
		Message += ",";
		Message += longitude;
		Message += "&t=h&z=14 ";
	
		Message.toCharArray( MessageArray , Message.length() + 1 );
		GPRSModem.sendSMS( NumberArray , MessageArray );

		if( check == 0 ) {
			Display.drawStringRight8px( "OK" , 250 , COLOR_GREEN , COLOR_BACKGROUND );
			digitalWrite( 17 , HIGH );
			delay( 300 );
			digitalWrite( 17 , LOW );
		}
		if( check == 1 ) Display.drawStringRight8px( "ERROR" , 250 , COLOR_RED , COLOR_BACKGROUND );
	} else if( alertButtonPressedTime >= URGENT_TIME ){
		String Message = "";
		String Message2 = "";
		String latitude = CordinateToString( GPS.latitude );
		String longitude = CordinateToString( GPS.longitude );
		Message = "HILFE! Es gibt einen NOTFALL!\r\nLangengrad: ";
		Message += latitude;
		Message += GPS.lat;
		Message += "\r\nBreitengrad: ";
		Message += longitude;
		Message += GPS.lon;
		Message += "\r\nIMEI: ";
		Message += mobileIMEI;
		Message += "\r\nAnfordern: POLIZEI, NOTARZT ";

		Message2 += "Google Maps: http://maps.google.de/maps?q=";
		Message2 += latitude;
		Message2 += ",";
		Message2 += longitude;
		Message2 += "&t=h&z=14 ";

		String Output = "SMS an ";
		Output += ALARM_NUMBER;
		Display.drawStringLeft8px( Output , 250 , COLOR_FOREGROUND , COLOR_BACKGROUND );

		Message.toCharArray( MessageArray , Message.length() + 1 );
		ALARM_NUMBER.toCharArray( NumberArray , ALARM_NUMBER.length() + 1 );
		int check = GPRSModem.sendSMS( NumberArray , MessageArray );

		delay( 1000 );

		Message2.toCharArray( MessageArray , Message2.length() + 1 );
		GPRSModem.sendSMS( NumberArray , MessageArray );

		if( check == 0 ) {
			Display.drawStringRight8px( "OK" , 250 , COLOR_GREEN , COLOR_BACKGROUND );
			digitalWrite( 17 , HIGH );
			delay( 300 );
			digitalWrite( 17 , LOW );
		}	
		if( check == 1 ) Display.drawStringRight8px( "ERROR" , 250 , COLOR_RED , COLOR_BACKGROUND );

		delay( 1000 );

		Output = "SMS an ";
		Output += URGENT_NUMBER;
		Display.drawStringLeft8px( Output , 268 , COLOR_FOREGROUND , COLOR_BACKGROUND );

		Message.toCharArray( MessageArray , Message.length() + 1 );
		URGENT_NUMBER.toCharArray( NumberArray , ALARM_NUMBER.length() + 1 );
		check = GPRSModem.sendSMS( NumberArray , MessageArray );

		delay( 1000 );

		Message2.toCharArray( MessageArray , Message2.length() + 1 );
		GPRSModem.sendSMS( NumberArray , MessageArray );

		if( check == 0 ) {
			Display.drawStringRight8px( "OK" , 268 , COLOR_GREEN , COLOR_BACKGROUND );
			digitalWrite( 17 , HIGH );
			delay( 300 );
			digitalWrite( 17 , LOW );
		}	
		if( check == 1 ) Display.drawStringRight8px( "ERROR" , 268 , COLOR_RED , COLOR_BACKGROUND );

	}
}

String CordinateToString( float value ) {
	value = value * 10000.0;
	long cordinateI = (long)value;

	String finalString = "";
	String workingString = "";
	workingString += cordinateI;

	finalString += workingString.substring( 0 , workingString.length() - 6 );
	if( finalString.length() == 0 && value >= 0 ) finalString += "0";
	if( finalString.equals( "-" ) ) finalString += "0";
	finalString += ".";

	workingString.replace( "." , "" );
	workingString = workingString.substring( workingString.length() - 6 , workingString.length() );
	char tarray[10]; 
	workingString.toCharArray(tarray, workingString.length()+1);
	long digits = atol( tarray );
	digits /= 60;
	finalString += digits;
	return finalString;
}