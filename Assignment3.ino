
const byte POT = 4; //Pin Definitions
const byte WATCHDOG = 19;
const byte BUTTON = 22; 
const byte ERROR = 18;
const byte SIG = 17;
const byte TASK3 = 19;
volatile int avecount = 0; //Variable used in pot averageing
volatile int POTval[4] = {1, 1, 1, 1}; //last 4 pot readings
volatile byte error_code = 0; //error code
struct SerialVals{byte Button; int SignalFrequency; int POTAverage;}; //struct for serial values
SerialVals Vals = {1 , 1 , 1};
SemaphoreHandle_t ValsSemaphore; //semaphore and queue setup
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
  ValsSemaphore = xSemaphoreCreateBinary(); //Semaphore to protect access to serial print values 
  xSemaphoreGive(ValsSemaphore); //part of semaphore setup
  xQueue = xQueueCreate( 1, sizeof(int) );
  xTaskCreate(&RUNWATCHDOG, "RUNWATCHDOG", 500,NULL,1,NULL ); //task 1, stack size 512, priority 1
  xTaskCreate(&BUTTONREAD, "BUTTONREAD", 500,NULL,1,NULL ); //task 2
  xTaskCreate(&SIGREAD, "SIGREAD", 1100,NULL,1,NULL ); //task 3
  xTaskCreate(&ADC, "ADC", 600,NULL,1,NULL ); //task 4
  xTaskCreate(&ADCAVE, "ADCAVE", 800,NULL,1,NULL ); //task 5
  xTaskCreate(&ASM, "ASM", 600,NULL,1,NULL ); //task 6
  xTaskCreate(&ERRORCALC, "ERRORCALC", 800,NULL,1,NULL ); //task 7
  xTaskCreate(&ERRORLED, "ERRORLED", 500,NULL,1,NULL ); //task 8
  xTaskCreate(&SERIALPRINT, "SERIALPRINT", 1000,NULL,1,NULL ); //task 9
}

void loop() 
{
  
}

void RUNWATCHDOG(void *pvParameter)
{
  while(1)
  {
    digitalWrite(WATCHDOG, HIGH); //blink watchdog led
    delayMicroseconds(50);
    digitalWrite(WATCHDOG, LOW);
    vTaskDelay(17); //block tast for 17 ticks (1 tick = 1 ms)
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
    vTaskDelay(42); //block task for 42 ticks, this sets the task fequency
  }
}

void ADCAVE(void *pvParameter)
{
  while(1)
  {
    int SendVal; //value to be sent through the queue
    xSemaphoreTake( ValsSemaphore, portMAX_DELAY ); //take semaphore to protect struct, this blocks access nto struct until this proccess has ended
    int loop;
    int TOT = 0;
    for(loop = 0; loop < 4; loop++) //add up last 4 readings
    {
        TOT = TOT + POTval[loop];
    }
    Vals.POTAverage = TOT / 4; //find average of readings and save to global struct
    SendVal = TOT / 4; //set value to send to queue
    xQueueOverwrite(xQueue, &SendVal); //send value to queue, if queue hold a previous value due to unforseen issues it is overwritten
    xSemaphoreGive( ValsSemaphore); //return access to struct
    vTaskDelay(42); //set task frequency
  }
} 

void BUTTONREAD(void *pvParameter)
{
  while(1)
  {
    xSemaphoreTake( ValsSemaphore, portMAX_DELAY ); //take semaphore to protect struct
    Vals.Button = !digitalRead(BUTTON); //read button and save to struct
    xSemaphoreGive( ValsSemaphore); //return struct access
    vTaskDelay(200); //set task frequency
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
    vTaskDelay(100); //set task freq
  }
}

void SIGREAD(void *pvParameter)
{
  while(1)
  {
    xSemaphoreTake( ValsSemaphore, portMAX_DELAY ); //take semaphore to protect struct
    int pulsetime = 0;
    pulsetime = (pulseIn(SIG, HIGH, 2500)) * 2; //uses pulseIn to find length of a single wavelength, includes maximum wait time 2500 if wave is outside of specified frequencies
    if(pulsetime != 0) //prevents divide by zero error
    {
      Vals.SignalFrequency = (1000000 / pulsetime); //calculate frequency and convert to apropriate units, *0.96 to account for predictable inaccuracy found during testing
    }
    xSemaphoreGive( ValsSemaphore); //return struct access
    vTaskDelay(1000); //set task freq
  }
}

void ERRORCALC(void *pvParameter)
{
  while(1)
  {
    int POTvalave;
    xQueueReceive(xQueue, &POTvalave, portMAX_DELAY); //takes the pot averaged value from the queue
    if(POTvalave > 2048) //if pot is above half way point, save error_code
    {
      error_code = 1;
    }
    else
    {
      error_code = 0;
    }
    vTaskDelay(333); //set task freq
  }
}

void ERRORLED(void *pvParameter)
{
  while(1)
  {
    digitalWrite(ERROR, error_code); //set led to show error code
    vTaskDelay(333); //set task freq
  }
}

void SERIALPRINT(void *pvParameter)
{
  while(1)
  {
    if(Vals.Button != 0) //check button state, if it is pressed proceed, if not end the task
    {
      xSemaphoreTake( ValsSemaphore, portMAX_DELAY ); //take semaphore
      Serial.print(Vals.Button); //serial print required data
      Serial.print(",");
      Serial.print(Vals.SignalFrequency);
      Serial.print(",");
      Serial.println(Vals.POTAverage);
      xSemaphoreGive( ValsSemaphore); //give semaphore
    }
    vTaskDelay(5000); //set task freq
  }
}
