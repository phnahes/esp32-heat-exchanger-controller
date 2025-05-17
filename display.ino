byte chardino1[8] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B10000,
  B10000,
  B11000,
  B11111
};

byte chardino2[8] = {
  B00000,
  B00001,
  B00001,
  B00001,
  B00001,
  B00011,
  B01111,
  B11111
};
byte chardino3[8] = {
  B11111,
  B10111,
  B11111,
  B11111,
  B11100,
  B11111,
  B11100,
  B11100
};
byte chardino4[8] = {
  B10000,
  B11000,
  B11000,
  B11000,
  B00000,
  B10000,
  B00000,
  B00000
};
byte chardino5[8] = {
  B11111,
  B11111,
  B01111,
  B00111,
  B00011,
  B00011,
  B00010,
  B00011
};
byte chardino6[8] = {
  B11111,
  B11111,
  B11111,
  B11111,
  B10110,
  B00010,
  B00010,
  B00011
};

byte chardino7[8] = {
  B11111,
  B11001,
  B10000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
};

void desplazar_dino() {
  lcd.createChar(1, chardino1);
  lcd.createChar(2, chardino2);
  lcd.createChar(3, chardino3);
  lcd.createChar(4, chardino4);
  lcd.createChar(5, chardino5);
  lcd.createChar(6, chardino6);
  lcd.createChar(7, chardino7);
  for (int a = 0; a <= 18; a++) {


    if (a >= 3) {
      lcd.setCursor(a - 3, 0);
      lcd.write(1);
    }
    if (a >= 2) {
      lcd.setCursor(a - 2, 0);
      lcd.write(2);
    }
    if (a >= 1) {
      lcd.setCursor(a - 1, 0);
      lcd.write(3);
    }
    lcd.setCursor(a, 0);
    lcd.write(4);

    if (a >= 3) {
      lcd.setCursor(a - 3, 1);
      lcd.write(5);
    }
    if (a >= 2) {
      lcd.setCursor(a - 2, 1);
      lcd.write(6);
    }
    if (a >= 1) {
      lcd.setCursor(a - 1, 1);
      lcd.write(7);
    }

    delay(animationDelay);
    lcd.clear();
  }
}