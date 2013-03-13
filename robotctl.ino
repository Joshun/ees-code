/* robotctl - EES robot control program
 * Joshua O'Leary and L3 TRL
 * 2013 */

#include <SoftwareSerial.h>

#include <PWMServo.h>
//#include <Servo.h>

#include <string.h>

#define RF_BAUD 9600
#define RF_TIMEOUT 150000
#define BUFFSIZE 20

#define GPS_BAUD 4800
#define GPS_MODE "$GPGGA,"
#define MAX_GPS_DATA 80

long lastMillis;

/* Standard PWM DC control pins */
const char E1 = 5;     //M1 Speed Control
const char E2 = 6;     //M2 Speed Control
const char M1 = 4;    //M1 Direction Control
const char M2 = 7;    //M1 Direction Control

/* Metal detection pin and debug LED */
const char detectPin = 8;
const char LED = 12;

/* GPS Software Serial */
SoftwareSerial gpsSerial = SoftwareSerial(2,3); /* rx, tx */
char gpsdata[MAX_GPS_DATA];

/* Servo variables */
const char panServoPin = 9;
const char tiltServoPin = 10;
PWMServo panServo;
PWMServo tiltServo;
//Servo panServo;
//Servo tiltServo;

void setup()
{
  /* Initialise serial I/O */
  Serial.begin(RF_BAUD);
  
  /* Initialise metal detector and LED */
  pinMode(detectPin,INPUT);
  pinMode(LED,OUTPUT);

  /* Initialise servo variables and move them to 90 degrees */
  panServo.attach(panServoPin);
  panServo.write(90);
  
  tiltServo.attach(tiltServoPin);
  tiltServo.write(90);
  
  /* Initialise gps I/O */
  gpsSerial.begin(GPS_BAUD);
  
  /* Initialise the timeout counter */
  lastMillis = millis();
}

int metaldetect()
{
  int detection = digitalRead(detectPin);
  if ( detection == LOW )
  {
    digitalWrite(LED,HIGH);
    return 1;
  }
  else
  {
    digitalWrite(LED,LOW);
    return 0;
  }
}

void mvpanservo(const unsigned char degrees)
{
  panServo.write(degrees);
}

void mvtiltservo(const unsigned char degrees)
{
  tiltServo.write(degrees);
}

void stop(void) // Stop
{
  digitalWrite(E1,LOW);
  digitalWrite(E2,LOW);
}
void advance(const char a, const char b) // Move forward
{
  analogWrite (E1,a); /* PWM speed */
  digitalWrite(M1,HIGH); /* Direction */
  analogWrite (E2,b);
  digitalWrite(M2,LOW);
}
void reverse (const char a, const char b) // Move backward
{
  analogWrite (E1,a);
  digitalWrite(M1,LOW);
  analogWrite (E2,b);
  digitalWrite(M2,HIGH);
}
void turn_L (const char a, const char b) // Turn Left
{
  analogWrite (E1,a);
  digitalWrite(M1,HIGH);
  analogWrite (E2,b);
  digitalWrite(M2,HIGH);
}
void turn_R (const char a, const char b) //Turn Right
{
  analogWrite (E1,a);
  digitalWrite(M1,LOW);
  analogWrite (E2,b);
  digitalWrite(M2,LOW);
}

void motorctl(const char direction,const char speed)
{
  if ( speed == 0 ) { stop(); }
  else
  {
    switch(direction)
    {
      case 'f':
        advance(speed,speed);
        break;
      case 'b':
        reverse(speed,speed);
        break;
      case 'l':
        turn_L(speed,speed);
        break;
      case 'r':
        turn_R(speed,speed);
        break;
    }
  }
}

void gpsread()
{
	if ( gpsSerial.available() )
	{
		if( gpsSerial.read() == '\n')
		{
			int i;
			char modecheck[7];
			for(i=0; i<7; i++){ modecheck[i] = '\0'; }
			gpsSerial.readBytes(modecheck, strlen(GPS_MODE));
			if ( strstr(modecheck, GPS_MODE) )
			{
				for(i=0; i<MAX_GPS_DATA; i++){ gpsdata[i] = '\0'; }
				gpsSerial.readBytesUntil('\n', gpsdata, ( MAX_GPS_DATA - 1 ));
			}
		}
	}
}

void rfread()
{
    char rfbuffer[BUFFSIZE]; for(int i=0; i<BUFFSIZE; i++) { rfbuffer[i] = '\0'; }
    
    char command;
    int value;

    Serial.readBytesUntil('\n', rfbuffer, BUFFSIZE);
    sscanf(rfbuffer, "%c%d", &command, &value);
    
    char rf_feedback[BUFFSIZE];
    snprintf(rf_feedback, BUFFSIZE, "c%s\n", rfbuffer);
    Serial.write(rf_feedback);
    
    switch(command)
    { /* If command is a direction, give the command and value to motorcontrol function */
        case 'f':
        case 'b':
        case 'l':
        case 'r':
                motorctl(command, value);
                break;
        case 'p':
				mvpanservo(value);
				//Serial.write("Servo Pos:"); Serial.print(value);
				//panServo.write(value);
				break;
		case 't':
				mvtiltservo(value);
				//Serial.write("Servo Pos:"); Serial.print(value);
				//tiltServo.write(value);
				break;
        case 'd':
				char rf_out[(BUFFSIZE + MAX_GPS_DATA)];
				snprintf(rf_out, (BUFFSIZE + MAX_GPS_DATA), "d%d,%s\n", metaldetect(), gpsdata);
				Serial.write(rf_out);										
				break;

    }
}

void check_timeout()
{
	if(  (millis() - lastMillis) > RF_TIMEOUT )
	{
		stop();
	}
}

void loop()
{
	/* Run RF interpreter if new commands are incoming, then reset the timeout */
	if( Serial.available() )
	{
		rfread();
		lastMillis = millis();
	}
	else
	{
		check_timeout();
	}
	/* Get updated gps data ready */
	gpsread();
}
