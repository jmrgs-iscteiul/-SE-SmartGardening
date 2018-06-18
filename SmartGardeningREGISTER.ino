/*
Liquid flow rate sensor -DIYhacking.com Arvind Sanjeev
 */
#include "DHT.h"
#include <TimeLib.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#define DHTPIN 3
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

// The hall-effect flow sensor outputs approximately 4.5 pulses per second per
// litre/minute of flow.
float calibrationFactor = 4.5;

volatile byte pulseCount;

//valores calibraveis
long WaterTiming = 1800;
int rise = 7;
int set = 21;

//valores giros de transmitir
float h = 0.0;
float t = 0.0;
unsigned long totalMilliLitres;
//***

float flowRate;
unsigned int flowMilliLitres;
unsigned long oldTime;

DHT dht(DHTPIN, DHTTYPE);

void setup(){
  // Initialize a serial connection for reporting values to the host
  Serial.begin(9600);

  dht.begin();

  setTime(9,30,00,27,05,2018); 

  pulseCount        = 0;
  flowRate          = 0.0;
  flowMilliLitres   = 0;
  totalMilliLitres  = 0;
  oldTime           = 0;

  // Inicialização do FAN
  DDRB = (1 <<DDB0);
  // Inicialização do light DHT water
  DDRD = (1 << DDD5) | (1 << DDD4);
  //LIMPAR O PINO D2 COM CIF
  DDRD &= ~(1 << DDD2);
  //LIGAR PULLUP RESISTOR
  PORTD |= (1 << PORTD2);
  
  EICRA |= (1 << ISC01);    // set INT0 to trigger on ANY logic change
  EIMSK |= (1 << INT0);     // Turns on INT0

  sei();                    // turn on interrupts
  
}

int full = 0;
long water_t = 0;

void light(){
  if(hour() >= set || hour() <= rise-1){
        PORTD = (0 << PORTD5);
  } else {
        PORTD = (1 << PORTD5);
     }
}

void flood(){
  //1800 = 30 min
  if(water_t + WaterTiming <= now() && !full){
      //digitalWrite(WATER, LOW);
    PORTD = (0<<DDD4);
      Serial.println("\nTank Flooded!");
      full=1;
  }
}


void drain()
{

  if ((millis() - oldTime) > 1000)   // Only process counters once per second
  {
    cli();
    
    flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / calibrationFactor;
    oldTime = millis();
    flowMilliLitres = (flowRate / 60) * 1000;
    totalMilliLitres += flowMilliLitres;

    unsigned int frac;

    // Reset the pulse counter so we can start incrementing again
    pulseCount = 0;

    // Enable the interrupt again now that we've finished sending output
    sei();
    }

  if(flowMilliLitres > 60 && full){
      PORTD=(1<<PORTD4);    
      water_t = now();
      Serial.println("\nTank drained!");
      full = 0;
    }
}

void tempControl() {
   // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  t = dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    return;
  }

   //Serial.println("\n");
   Serial.println(t);
   Serial.print("ºC ;");
   Serial.print(h);
   Serial.print("%");

 if(h > 85 || t > 32){
    PORTB = (1 << PORTB0);
    Serial.println("Fan ON!");
  } 
 
 if(h <= 75 && t <= 30){
    PORTB = (0 << PORTB0);
    Serial.println("Fan OFF!");
  }
}

void loop(){
  readData();
  flood();
  drain();
  light();
  tempControl();
  digitalClockDisplay();
  delay(3000);  
}


void digitalClockDisplay(){
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(" ");
  Serial.print(month());
  Serial.print(" ");
  Serial.print(year()); 
  Serial.println(); 
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

/*
Insterrupt Service Routine
 */
ISR(PCINT0_vect)
{
  // Increment the pulse counter
  pulseCount++;
}

void readData(){
  String data;
  if(Serial.available()>0)
  {
    data = Serial.readString();
  
    if(data == "fan on"){ 
      //  digitalWrite(FAN,HIGH);
      PORTB = (1 << PORTB0);
      Serial.println("Fan ON!");
      }

    else if(data == "fan off"){ 
      //  digitalWrite(FAN,LOW);
      PORTB = (0 << PORTB0);
      Serial.println("Fan OFF!");
      }

    else if(data == "water") water_t = 0;

    else if(data.indexOf("time")>-1){
     long pctime = data.substring(data.indexOf(" ")+1,data.length()).toInt();
     Serial.println("Time updated!");
     setTime(pctime); // Sync Arduino clock to the time received on the serial port
    }

    else{
      Serial.println("Unknown Comand!");
    }

    }
}

