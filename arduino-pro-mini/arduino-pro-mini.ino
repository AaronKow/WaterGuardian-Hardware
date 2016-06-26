/* define all pins */
int valve1Pin = 2;            // valve pin
int pump1Pin = 3;             // pump pin
int rainSensorPin = 4;        // rain sensor
int waterLVL1Pin = 5;         // water level sensor 1
int waterLVL2Pin = 6;         // water level sensor 2
int waterLVL3Pin = 7;         // water level sensor 3

/* checker used for controlling the valve and pump */
int checker1, checker2, checker3, checker4;

void setup() {
  pinMode(valve1Pin, OUTPUT);
  pinMode(pump1Pin, OUTPUT);
  pinMode(rainSensorPin, INPUT);  // If raining, it display "1"
  pinMode(waterLVL1Pin, INPUT);   // If reach water level, it display "1"
  pinMode(waterLVL2Pin, INPUT);
  pinMode(waterLVL3Pin, INPUT);

  /* HIGH is off, LOW is on in relay */
  digitalWrite(valve1Pin, HIGH);  // no water flow
  digitalWrite(pump1Pin, HIGH);   // pump is off
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
  if(checker1 || checker2){             // when raining or barrel 1 is full
    digitalWrite(valve1Pin, HIGH);      // close valve
  }
  else if(!checker1 && !checker2){      // when no rain and barrel is not full
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
