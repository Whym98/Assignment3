#include <TridentTD_EasyFreeRTOS32.h>

#include <Ticker.h>

const byte POT = 4; //Pin Definitions
const byte WATCHDOG = 5;
const byte BUTTON = 22; 
const byte ERROR = 18;
const byte SIG = 17;
const byte TASK3 = 19;
volatile int count = 1; //global variables
volatile int avecount = 0;
volatile int POTval[4] = {1, 1, 1, 1};
volatile int POTvalave = 1;
volatile byte BUTTONSTATE = 0;
volatile byte error_code = 0;
volatile int SIGFREQ = 0;
Ticker periodicTicker; //ticker setup

void setup() 
{
  Serial.begin(115200); //serial setup
  pinMode(POT, INPUT); //IO input/output
  pinMode(WATCHDOG, OUTPUT);
  pinMode(BUTTON, INPUT);
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(ERROR, OUTPUT);
  pinMode(SIG, INPUT);
  pinMode(TASK3, OUTPUT);
  periodicTicker.attach_ms(2, TICKER); //ticker, 2ms
  xTaskCreate(&RUNWATCHDOG, "RUNWATCHDOG", 512,NULL,5,NULL ); //task 1
  xTaskCreate(&BUTTONREAD, "BUTTONREAD", 512,NULL,5,NULL ); //task 2
  xTaskCreate(&SIGREAD, "SIGREAD", 512,NULL,5,NULL ); //task 3
  xTaskCreate(&ADC, "ADC", 512,NULL,5,NULL ); //task 4
  xTaskCreate(&ADCAVE, "ADCAVE", 512,NULL,5,NULL ); //task 5
  xTaskCreate(&ASM, "ASM", 512,NULL,5,NULL ); //task 6
  xTaskCreate(&ERRORCALC, "ERRORCALC", 512,NULL,5,NULL ); //task 7
  xTaskCreate(&ERRORLED, "ERRORLED", 512,NULL,5,NULL ); //task 8
  xTaskCreate(&SERIALPRINT, "SERIALPRINT", 512,NULL,5,NULL ); //task 9
}


void TICKER() //run upon triggering ticker
{
  count++; //increment count upon ticker triggering
  if((count % 9) == 0) //the if loops determine the various task frequencies
  {
    RUNWATCHDOG(); //run task 1
  }
  if((count % 21) == 0)
  {
    ADC(); //task 4
    ADCAVE(); //task 5
  }
  if((count % 100) == 0)
  {
    BUTTONREAD(); //task 2
  }
  if((count % 50) == 0)
  {
    ASM(); //task 6
  }
  if((count % 500) == 0)
  {
    SIGREAD(); //task 3
  }
  if((count % 167) == 0)
  {
    ERRORCALC(); //task 7
    ERRORLED(); //task 8
  }
  if((count % 2500) == 0)
  {
    SERIALPRINT(); //task 9
  }
}

void loop() //holds for unused slot time
{
  
}

void RUNWATCHDOG()
{
  digitalWrite(WATCHDOG, HIGH); //blink watchdog led
  delayMicroseconds(50);
  digitalWrite(WATCHDOG, LOW);
}

void ADC()
{
  POTval[avecount] = analogRead(POT); //read POT, save data to apropriate array slot to memorise last 4 readings
  avecount++;
  if(avecount >= 3) //loop back around if avecount exceeds array size
  {
    avecount = 0;
  }
}

void ADCAVE()
{
  int loop;
  int TOT = 0;
  for(loop = 0; loop < 4; loop++) //add up last 4 readings
  {
      TOT = TOT + POTval[loop];
  }
  POTvalave = TOT / 4; //find average of readings and save to global variable 
} 

void BUTTONREAD()
{
  BUTTONSTATE = !digitalRead(BUTTON); //read button
}

void ASM()
{
  int i;
  for(i = 0; i < 1000; i++) //run "__asm__ __volatile__ ("nop");" 1000 times
  {
    __asm__ __volatile__ ("nop");
  }
}

void SIGREAD()
{
  digitalWrite(TASK3, HIGH); //output to test execution time
  int pulsetime = 0;
  pulsetime = (pulseIn(SIG, HIGH, 2500)) * 2; //uses pulseIn to find length of a single wavelength, includes maximum wait time 2500 if wave is outside of specified frequencies
  if(pulsetime != 0) //prevents divide by zero error
  {
    SIGFREQ = (1000000 / pulsetime)* 0.96; //calculate frequency and convert to apropriate units, *0.96 to account for predictable inaccuracy found during testing
  }
  digitalWrite(TASK3, LOW); //output to test execution time
}

void ERRORCALC()
{
  if(POTvalave > 2048) //if pot is above half way point, save error_code
  {
    error_code = 1;
  }
  else
  {
    error_code = 0;
  }
}

void ERRORLED()
{
  digitalWrite(ERROR, error_code); //set led to show error code
}

void SERIALPRINT()
{
  Serial.print(BUTTONSTATE); //serial print required data
  Serial.print(",");
  Serial.print(SIGFREQ);
  Serial.print(",");
  Serial.println(POTvalave);
}
