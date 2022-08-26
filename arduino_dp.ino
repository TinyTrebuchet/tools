#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define ONE 8
#define TWO 9
#define THREE 10
#define FOUR 11

String command;
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);
  
  pinMode(ONE, OUTPUT);
  pinMode(TWO, OUTPUT);
  pinMode(THREE, OUTPUT);
  pinMode(FOUR, OUTPUT);

  digitalWrite(ONE, HIGH);
  digitalWrite(TWO, HIGH);
  digitalWrite(THREE, HIGH);
  digitalWrite(FOUR, HIGH);

  lcd.begin();
  lcd.backlight();

  lcd.print("Hello World");
  delay(2000);
  lcd.setCursor(0,0);
  lcd.print("1 - OFF ");
  lcd.setCursor(8,0);
  lcd.print("2 - OFF ");
  lcd.setCursor(0,1);
  lcd.print("3 - OFF ");
  lcd.setCursor(8, 1);
  lcd.print("4 - OFF ");
  
}

void loop() {
  // put your main code here, to run repeatedly:

  while (Serial.available())
  {
    delay(10);
    
    char c = Serial.read();
    if (c == '#') break;
    command += c;
    
  }

  if (command.length() > 0) 
  {
    Serial.println(command);

    if (command == "*one on")
    {
      digitalWrite(ONE, LOW);
      lcd.setCursor(0,0);
      lcd.print("1 - ON  ");
    }
    else if (command == "*one off")
    {
      digitalWrite(ONE, HIGH);
      lcd.setCursor(0,0);
      lcd.print("1 - OFF ");
    }
    else if (command == "*two on")
    {
      digitalWrite(TWO, LOW);
      lcd.setCursor(8,0);
      lcd.print("2 - ON  ");
    }
    else if (command == "*two off")
    {
      digitalWrite(TWO, HIGH);
      lcd.setCursor(8,0);
      lcd.print("2 - OFF ");
    }
    else if (command == "*three on")
    {
      digitalWrite(THREE, LOW);
      lcd.setCursor(0,1);
      lcd.print("3 - ON  ");
    }
    else if (command == "*three off")
    {
      digitalWrite(THREE, HIGH);
      lcd.setCursor(0,1);
      lcd.print("3 - OFF ");
    }
    else if (command == "*four on")
    {
      digitalWrite(FOUR, LOW);
      lcd.setCursor(8, 1);
      lcd.print("4 - ON  ");
    }
    else if (command == "*four off")
    {
      digitalWrite(FOUR, HIGH);
      lcd.setCursor(8, 1);
      lcd.print("4 - OFF ");
    }

    command = "";
  }
}
