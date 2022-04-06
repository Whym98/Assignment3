
//#include "freertos/FreeRTOS.h"
//#include "freertos/task.h"

const byte POT = 4; //Pin Definitions
const byte WATCHDOG = 5;
const byte BUTTON = 22; 
const byte ERROR = 18;
const byte SIG = 17;
const byte TASK3 = 19;
volatile int avecount = 0;
volatile int POTval[4] = {1, 1, 1, 1};
volatile byte error_code = 0;
struct SerialVals{byte Button; int SignalFrequency; int POTAverage;};
SerialVals Vals = {1 , 1 , 1};
SemaphoreHandle_t ValsSemaphore;
xQueueHandle  xQueue;

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
  ValsSemaphore = xSemaphoreCreateBinary();
  xSemaphoreGive(ValsSemaphore);
  xQueue = xQueueCreate( 1, sizeof(int) );
  xTaskCreate(&RUNWATCHDOG, "RUNWATCHDOG", 512,NULL,1,NULL ); //task 1
  xTaskCreate(&BUTTONREAD, "BUTTONREAD", 512,NULL,1,NULL ); //task 2
  xTaskCreate(&SIGREAD, "SIGREAD", 1024,NULL,1,NULL ); //task 3
  xTaskCreate(&ADC, "ADC", 1024,NULL,1,NULL ); //task 4
  xTaskCreate(&ADCAVE, "ADCAVE", 1024,NULL,1,NULL ); //task 5
  xTaskCreate(&ASM, "ASM", 512,NULL,1,NULL ); //task 6
  xTaskCreate(&ERRORCALC, "ERRORCALC", 1024,NULL,1,NULL ); //task 7
  xTaskCreate(&ERRORLED, "ERRORLED", 512,NULL,1,NULL ); //task 8
  xTaskCreate(&SERIALPRINT, "SERIALPRINT", 1024,NULL,1,NULL ); //task 9
}

void loop() //holds for unused slot time
{
  
}

void RUNWATCHDOG(void *pvParameter)
{
  while(1)
  {
    //Serial.println("WATCHDOG");
    digitalWrite(WATCHDOG, HIGH); //blink watchdog led
    delayMicroseconds(50);
    digitalWrite(WATCHDOG, LOW);
    vTaskDelay(17);
  }
}

void ADC(void *pvParameter)
{
  while(1)
  {
    //Serial.println("ADC");
    POTval[avecount] = analogRead(POT); //read POT, save data to apropriate array slot to memorise last 4 readings
    avecount++;
    if(avecount >= 3) //loop back around if avecount exceeds array size
    {
      avecount = 0;
    }
    vTaskDelay(42);
  }
}

void ADCAVE(void *pvParameter)
{
  while(1)
  {
    //Serial.println("ADCAVE");
    int SendVal;
    xQueueSend( xQueue, &SendVal, portMAX_DELAY );
    xSemaphoreTake( ValsSemaphore, portMAX_DELAY );
    int loop;
    int TOT = 0;
    for(loop = 0; loop < 4; loop++) //add up last 4 readings
    {
        TOT = TOT + POTval[loop];
    }
    Vals.POTAverage = TOT / 4; //find average of readings and save to global variable 
    xQueueOverwrite(xQueue, &SendVal);
    xSemaphoreGive( ValsSemaphore);
    vTaskDelay(42);
  }
} 

void BUTTONREAD(void *pvParameter)
{
  while(1)
  {
    //Serial.println("BUTTONREAD");
    xSemaphoreTake( ValsSemaphore, portMAX_DELAY );
    Vals.Button = !digitalRead(BUTTON); //read button
    xSemaphoreGive( ValsSemaphore);
    vTaskDelay(200);
  }
}

void ASM(void *pvParameter)
{
  while(1)
  {
    //Serial.println("ASM");
    int i;
    for(i = 0; i < 1000; i++) //run "__asm__ __volatile__ ("nop");" 1000 times
    {
      __asm__ __volatile__ ("nop");
    }
    vTaskDelay(100);
  }
}

void SIGREAD(void *pvParameter)
{
  while(1)
  {
    //Serial.println("SIGREAD");
    xSemaphoreTake( ValsSemaphore, portMAX_DELAY );
    digitalWrite(TASK3, HIGH); //output to test execution time
    int pulsetime = 0;
    pulsetime = (pulseIn(SIG, HIGH, 2500)) * 2; //uses pulseIn to find length of a single wavelength, includes maximum wait time 2500 if wave is outside of specified frequencies
    if(pulsetime != 0) //prevents divide by zero error
    {
      Vals.SignalFrequency = (1000000 / pulsetime)* 0.96; //calculate frequency and convert to apropriate units, *0.96 to account for predictable inaccuracy found during testing
    }
    digitalWrite(TASK3, LOW); //output to test execution time
    xSemaphoreGive( ValsSemaphore);
    vTaskDelay(1000);
  }
}

void ERRORCALC(void *pvParameter)
{
  while(1)
  {
    //Serial.println("ERRORCALC");
    int POTvalave;
    xQueueReceive( xQueue, &POTvalave, portMAX_DELAY);
    if(POTvalave > 2048) //if pot is above half way point, save error_code
    {
      error_code = 1;
    }
    else
    {
      error_code = 0;
    }
    vTaskDelay(333);
  }
}

void ERRORLED(void *pvParameter)
{
  while(1)
  {
    //Serial.println("ERRORLED");
    digitalWrite(ERROR, error_code); //set led to show error code
    vTaskDelay(333);
  }
}

void SERIALPRINT(void *pvParameter)
{
  while(1)
  {
    if(Vals.Button != 0)
    {
      xSemaphoreTake( ValsSemaphore, portMAX_DELAY );
      Serial.print(Vals.Button); //serial print required data
      Serial.print(",");
      Serial.print(Vals.SignalFrequency);
      Serial.print(",");
      Serial.println(Vals.POTAverage);
      xSemaphoreGive( ValsSemaphore);
    }
    vTaskDelay(5000);
  }
}
