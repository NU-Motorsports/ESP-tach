/*
 * DJ Walsh
 * ESP Tach Code
 * 2/7/2023
 */

const byte hallpin = 4;
const byte ledpin = 2;

void setup() {
  // put your setup code here, to run once:
  
  Serial.begin(115200);
  pinMode(hallpin, INPUT);
  pinMode(ledpin, OUTPUT);
  

}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.print(digitalRead(hallpin));

}
