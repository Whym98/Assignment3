
//#include "freertos/FreeRTOS.h"
//#include "freertos/task.h"

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
#define configTICK_RATE_HZ 500

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
  xTaskCreate(&RUNWATCHDOG, "RUNWATCHDOG", 512,NULL,5,NULL ); //task 1
  xTaskCreate(&BUTTONREAD, "BUTTONREAD", 512,NULL,5,NULL ); //task 2
  xTaskCreate(&SIGREAD, "SIGREAD", 512,NULL,5,NULL ); //task 3
  xTaskCreate(&ADC, "ADC", 512,NULL,5,NULL ); //task 4
  xTaskCreate(&ADCAVE, "ADCAVE", 4096,NULL,5,NULL ); //task 5
  xTaskCreate(&ASM, "ASM", 4096,NULL,5,NULL ); //task 6
  xTaskCreate(&ERRORCALC, "ERRORCALC", 4096,NULL,5,NULL ); //task 7
  xTaskCreate(&ERRORLED, "ERRORLED", 4096,NULL,5,NULL ); //task 8
  xTaskCreate(&SERIALPRINT, "SERIALPRINT", 4096,NULL,5,NULL ); //task 9
}

/*

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
*/

void loop() //holds for unused slot time
{
  
}

void RUNWATCHDOG(void *pvParameter)
{
  while(1)
  {
    digitalWrite(WATCHDOG, HIGH); //blink watchdog led
    delayMicroseconds(50);
    digitalWrite(WATCHDOG, LOW);
    vTaskDelay(250);
  }
}

void ADC(void *pvParameter)
{
  while(1)
  {
    POTval[avecount] = analogRead(POT); //read POT, save data to apropriate array slot to memorise last 4 readings
    avecount++;
    if(avecount >= 3) //loop back around if avecount exceeds array size
    {
      avecount = 0;
    }
    vTaskDelay(21);
  }
}

void ADCAVE(void *pvParameter)
{
  while(1)
  {
    int loop;
    int TOT = 0;
    for(loop = 0; loop < 4; loop++) //add up last 4 readings
    {
        TOT = TOT + POTval[loop];
    }
    POTvalave = TOT / 4; //find average of readings and save to global variable 
    vTaskDelay(21);
  }
} 

void BUTTONREAD(void *pvParameter)
{
  while(1)
  {
    BUTTONSTATE = !digitalRead(BUTTON); //read button
    vTaskDelay(100);
  }
}

void ASM(void *pvParameter)
{
  while(1)
  {
    int i;
    for(i = 0; i < 1000; i++) //run "__asm__ __volatile__ ("nop");" 1000 times
    {
      __asm__ __volatile__ ("nop");
    }
    vTaskDelay(50);
  }
}

void SIGREAD(void *pvParameter)
{
  while(1)
  {
    digitalWrite(TASK3, HIGH); //output to test execution time
    int pulsetime = 0;
    pulsetime = (pulseIn(SIG, HIGH, 2500)) * 2; //uses pulseIn to find length of a single wavelength, includes maximum wait time 2500 if wave is outside of specified frequencies
    if(pulsetime != 0) //prevents divide by zero error
    {
      SIGFREQ = (1000000 / pulsetime)* 0.96; //calculate frequency and convert to apropriate units, *0.96 to account for predictable inaccuracy found during testing
    }
    digitalWrite(TASK3, LOW); //output to test execution time
    vTaskDelay(500);
  }
}

void ERRORCALC(void *pvParameter)
{
  while(1)
  {
    if(POTvalave > 2048) //if pot is above half way point, save error_code
    {
      error_code = 1;
    }
    else
    {
      error_code = 0;
    }
    vTaskDelay(167);
  }
}

void ERRORLED(void *pvParameter)
{
  while(1)
  {
    digitalWrite(ERROR, error_code); //set led to show error code
    vTaskDelay(167);
  }
}

void SERIALPRINT(void *pvParameter)
{
  while(1)
  {
    //if(digitalRead(BUTTON) == 0)
    //{
      Serial.print(BUTTONSTATE); //serial print required data
      Serial.print(",");
      Serial.print(SIGFREQ);
      Serial.print(",");
      Serial.println(POTvalave);
    //}
    vTaskDelay(2500);
  }
}
