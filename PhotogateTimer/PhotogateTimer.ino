/*
Open Science DAC - Charles Meyer - 2015
 
 This code records blocking and unblocking events on one or two photogates (or equivalent switches) with a precision of 1 us. 
 Use the serial monitor tool to view data. Data can be copied into a spreadsheet program for farther analysis.
 */
#include <SoftwareSerial.h>
#define bufferSize 150  // Sets the size of the circular buffer for storing interrupt data events. Increasing this may cause erratic behavior of the code.
#define DELIM '\t'   // this is the data delimitter. Default setting is a tab character ("\t")

// Basic Settings: Change these between true and false depending on your experiment.
const boolean dualGates = false; //Set to true if a second gate in connected.
const boolean recordBlocks = true; //Record when the sensor goes from unblocked to blocked.
const boolean recordUnblocks = true; //Record when the sensor goes from blocked to unblocked.
const boolean pendulumMode = false; //Ignore every other event. @notImplemented

//Advanced Settings

const int baudRate = 9600;  // Baud rate for serial communications. Increase this for high data rate applications

const int ledPin = 13;    // 
const int photogatePin = 2; // default pin D2 if plugged port 1
const int photogate2Pin = 3; // default pin D3 if plugged port 2

/* The following variables are all used and modified by the code. These should not be changed or re-named. */

unsigned long timerOffset = 0; 
unsigned int displayIndex; // the current item that has been displayed to the Serial Monitor from the data buffer.
unsigned int count;  // tracks the total # of data entries 
unsigned int bufferRestarted = 0;

/* These variables are all accessed and modified by the interrupt handler "PhotogateEvent" 
Variables used by the Interrupt handler must be defined as volatile. */
volatile int photogate = HIGH;
volatile int photogate2 = HIGH;
volatile int start = 0;  // 1 == start, 0 == stop
volatile unsigned int numBlocks;
volatile unsigned long startTime;  //Time in us
volatile unsigned long stopTime;  //Time in us
volatile byte dataIndex;
volatile byte displayCount;  // stores the number of items in data Buffer to be displayed
volatile byte state[bufferSize];
volatile unsigned long time_us[bufferSize]; // Time in us

void setup() 
{
  attachInterrupt(0, photogateEvent, CHANGE);
  if (dualGates) {
    if (recordBlocks && !recordUnblocks){
      attachInterrupt(0, photogateEvent, RISING);
      attachInterrupt(1, photogateEvent, RISING);
    }
    else if (!recordBlocks && recordUnblocks){
      attachInterrupt(0, photogateEvent, FALLING);
      attachInterrupt(1, photogateEvent, FALLING);
    }
    else {
      attachInterrupt(0, photogateEvent, CHANGE);
      attachInterrupt(1, photogateEvent, CHANGE);
    }
  }
  else {
    if (recordBlocks && !recordUnblocks){
      attachInterrupt(0, photogateEvent, RISING);
    }
    else if (!recordBlocks && recordUnblocks){
      attachInterrupt(0, photogateEvent, FALLING);
    }
    else {
      attachInterrupt(0, photogateEvent, CHANGE);
    }
  }
  pinMode(ledPin, OUTPUT);
  Serial.begin(baudRate);
  displayHeader();
};// end of setup

void loop ()
{ 

  if (displayCount > 0)  // only display to Serial monitor if an interrupt has added data to the data buffer.
  {
    count++;
    Serial.print(count);
    if (dualGates) {
      if( state[displayIndex] == 0 ) {
        Serial.print(DELIM);
        Serial.print(0);
        Serial.print(DELIM);
        Serial.print(0);
      }
      else if ( state[displayIndex] == 1 ) {
        Serial.print(DELIM);
        Serial.print(1);
        Serial.print(DELIM);
        Serial.print(0);
      }
      else if ( state[displayIndex] == 2 ) {
        Serial.print(DELIM);
        Serial.print(0);
        Serial.print(DELIM);
        Serial.print(1);
      }
      else {
        Serial.print(DELIM);
        Serial.print(1);
        Serial.print(DELIM);
        Serial.print(1);
      }
    }
    else {
      Serial.print(DELIM); //tab character
      Serial.print(state[displayIndex]);
    }
    
    
    if (displayIndex != 0) {
      Serial.print(DELIM); //tab character
      Serial.print((time_us[displayIndex] - timerOffset) / 1E6, 6);  // at least 6 sig figs
      Serial.print(DELIM); //tab character
      Serial.print((time_us[displayIndex] - time_us[displayIndex - 1]) / 1E6, 6);
    }
    else if (bufferRestarted == 1){
      // case where buffer just restarted;
      Serial.print(DELIM); //tab character
      Serial.print((time_us[displayIndex] - timerOffset) / 1E6, 6);  // at least 6 sig figs
      Serial.print(DELIM); //tab character
      Serial.print((time_us[displayIndex] - time_us[bufferSize]) / 1E6, 6);
    }
    else {
      //first record
      timerOffset = time_us[displayIndex]; // start time at 0
      Serial.print(DELIM); //tab character
      Serial.print((time_us[displayIndex] - timerOffset) / 1E6, 6);  // at least 6 sig figs
      Serial.print(DELIM); //delta t undefined
    }

    Serial.println();//new line
    
    displayIndex++;
    if(displayIndex >= bufferSize)
    {
      displayIndex = 0;
      bufferRestarted = 1;
    }
    displayCount--; // deduct one
  }
} // end of loop

/*************************************************
 * photogateEvent()
 * 
 * Interrupt service routine. Handles capturing 
 * the time and saving this to memory when the 
 * photogate(s) issue an interrupt.
 * 
 *************************************************/
void photogateEvent()
{ 
  time_us[dataIndex] = micros();

  photogate = digitalRead(photogatePin);

  if (dualGates){
    photogate2 = digitalRead(photogate2Pin);
    if ( photogate == 0 && photogate2 == 0 ){
      state[dataIndex] = 0; // both gates open
      digitalWrite(ledPin, LOW);  // LED off
    }
    else if ( photogate == 1 && photogate2 == 0 ){
      state[dataIndex] = 1; // gate 1 blocked       
      digitalWrite(ledPin, HIGH);   // LED on
    }
    else if ( photogate == 0 && photogate2 == 1 ){
      state[dataIndex] = 2; // gate 2 blocked       
      digitalWrite(ledPin, HIGH);   // LED on
    }
    else {
      state[dataIndex] = 3; // gate 1 and 2 blocked       
      digitalWrite(ledPin, HIGH);   // LED on
    }
  }
  else {
    if (photogate == 0) {
      state[dataIndex] = 0;  
      digitalWrite(ledPin, LOW);  // LED off
    }
    else {
      state[dataIndex] = 1;            
      digitalWrite(ledPin, HIGH);   // LED on
    }
  }
  displayCount++;  // add one to "to be displayed" buffer

  dataIndex++;
  if(dataIndex >= bufferSize)
  {
    dataIndex = 0;
  }
}

/*************************************************
 * displayHeader()
 *
 * Prints logger info, the column header info , and units
 *  
 *************************************************/
void displayHeader()
{
  Serial.println("Open Science DAC, Photogate Timer");
  Serial.println();
  Serial.print("Event");
  if (dualGates){
    Serial.print(DELIM);
    Serial.print("Gate 1 State");
    Serial.print(DELIM);
    Serial.print("Gate 2 State");
  }
  else {
    Serial.print(DELIM);
    Serial.print("Gate State");
  }
  Serial.print(DELIM);
  Serial.print("Time");
  Serial.print(DELIM);
  Serial.print("Delta t         ");   
  Serial.println();

// Units
  Serial.print("#");
  if (dualGates){
    Serial.print(DELIM);
    Serial.print("(1 = Blocked)");
    Serial.print(DELIM);
    Serial.print("(1 = Blocked)");
  }
  else {
    Serial.print(DELIM);
    Serial.print("(1 = Blocked)");
  }
  Serial.print(DELIM);
  Serial.print("(s)");
  Serial.print(DELIM);
  Serial.print("(s)         ");   
  Serial.println();
  
  
  Serial.println("--------------------------");  
}

