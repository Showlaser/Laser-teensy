void setup() {
  Serial.begin(9600);
}

void loop() {
  if (Serial.available()) {
    Serial.println("hello");
  // put your main code here, to run repeatedly:
  String data = Serial.readString();
  Serial.println(data);
  }  
}
