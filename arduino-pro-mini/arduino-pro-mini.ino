/*
  The MIT License (MIT)

  Copyright (c) 2015 AaronKow

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

int valve1Pin = 2;
int pump1Pin = 3;
int rainSensorPin = 4;
int waterLVL1Pin = 5;
int waterLVL2Pin = 6;
int waterLVL3Pin = 7;

int checker1, checker2, checker3, checker4;

void setup() {
  pinMode(valve1Pin, OUTPUT);
  pinMode(pump1Pin, OUTPUT);
  pinMode(rainSensorPin, INPUT);  // If rain present, it display "1"
  pinMode(waterLVL1Pin, INPUT);   // If water present, it display "1"
  pinMode(waterLVL2Pin, INPUT);
  pinMode(waterLVL3Pin, INPUT);

  /* HIGH is off, LOW is on in relay */
  digitalWrite(valve1Pin, HIGH);
  digitalWrite(pump1Pin, HIGH);
  Serial.begin(9600);
}

void loop() {
  setChecker();
  controlValve1();
  controlPump1();
  
  delay(20);
}

void setChecker(){
  checker1 = digitalRead(rainSensorPin);
  checker2 = digitalRead(waterLVL1Pin);
  checker3 = digitalRead(waterLVL2Pin);
  checker4 = digitalRead(waterLVL3Pin);
}

void controlValve1(){
  if(checker1 || checker2){            // when raining or barrel 1 is full
    digitalWrite(valve1Pin, HIGH);      // close valve
  }
  else if(!checker1 && !checker2){       // when no rain and barrel is not full
    digitalWrite(valve1Pin, LOW);       // open valve
  }
}

void controlPump1(){
  if (!checker3 || checker4){           // when barrel 1 no water or barrel 2 is full
    digitalWrite(pump1Pin, HIGH);       // deactivate pump
  }
  else if(checker3 && !checker4){       // when barrel 1 got water and barrel 2 not full
    digitalWrite(pump1Pin, LOW);        // activate pump
  }
}

void checkerDebug(){
  Serial.print("Checker 1:");
  Serial.println(checker1);
  Serial.print("Checker 2:");
  Serial.println(checker2);
  Serial.print("Checker 3:");
  Serial.println(checker3);
  delay(1000);
}
