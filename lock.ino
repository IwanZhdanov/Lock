#include <Servo.h>
#include <EEPROM.h>

// Подключение устройств
#define RED 12      // Красный светодиод
#define GREEN 7     // Зелёный светодиод
#define BTN 4       // Кнопка открытия замка изнутри
#define KEYS A1     // Клавиатура
#define DOOR 2      // Датчик открытия двери
#define POWER 5     // Переключатель рабочего режима
#define ServaD 9    // Сервопривод

// Настройки
#define MAXENTER 15                   // Макс. кол-во нажатий кнопок до блокировки клавиатуры
#define TIME 10000                    // Время до сброса
String MASTERCODE = "994038660909";   // Мастер-код для полного сброса замка
String CORRECTCODE  = "1234";         // Код открытия по-умолчанию
String CHPASSCODE = "9876";           // Код смены пароля по-умолчанию

// Инициализация переменных
Servo srv1;
bool flag = true;
unsigned long timerTime = 0;
String code = "";
String code2 = "";
String code3 = "";
int mode = 0;
int codeNum = 0;
int ch_attempt = 0;
String ch_code = "";
String ch_code2 = "";
int entered = 0;
bool needBlinkEnd = false;
long ServaTmr = 0;

// Значения уровней для резистивной клавиатуры
int vals[] = {465,413,576,938,390,534,831,370,496,744,351,676};
int defVals[] = {465,413,576,938,390,534,831,370,496,744,351,676};
int newVals[] = {0,0,0,0,0,0,0,0,0,0,0,0};


void setup() {
  pinMode (RED, OUTPUT);
  pinMode (GREEN, OUTPUT);
  pinMode (BTN, INPUT);
  pinMode (KEYS, INPUT);
  pinMode (POWER, INPUT);
  srv1.attach(ServaD);
  srv1.write (0);
  loadCodes ();
  lock (true);
}

// Сохранение кодов в энергонезависимую память
void saveCodes () {
  int addr = 100;
  int a, i;
  String c = CORRECTCODE;
  int len = c.length();
  EEPROM.write(addr, len);
  addr = addr + 1;
  for (a=0;a<len;a++) {
    i = c.charAt(a);
    EEPROM.write(addr+a, i);
  }
  addr = addr + len;
  c = CHPASSCODE;
  len = c.length();
  EEPROM.write(addr, len);
  addr = addr + 1;
  for (a=0;a<len;a++) {
      i = c.charAt(a);
      EEPROM.write (addr+a, i);
  }
  addr = addr + len;
}

// Загрузка кодов из энергонезависимой памяти
void loadCodes () {
 int addr = 100;
 int a, i;
 int len = EEPROM.read (addr);
 addr = addr + 1;
 String c = "";
 for (a=0;a<len;a++) {
    i = EEPROM.read (addr+a);
    c = c + "0";
    c.setCharAt(a, i);
 }
 addr = addr + len;
 CORRECTCODE = c;
 c = "";
 len = EEPROM.read(addr);
 addr = addr + 1;
 for (a=0;a<len;a++) {
    i = EEPROM.read (addr+a);
    c = c + "0";
    c.setCharAt(a, i);
 }
 CHPASSCODE = c;
 addr = addr + len;
}

// Мигание светодиода
void blk (int pin, int sho, int hid, int qua) {
  digitalWrite (RED, LOW);
  digitalWrite (GREEN, LOW);
  int a;
  for (a=0;a<qua;a++) {
    digitalWrite (pin, HIGH);
    delay (sho);
    digitalWrite (pin, LOW);
    delay (hid);
  }
  rg (!flag, RED, GREEN);
}

// Отображение состояния замка на светодиодах
void rg (bool flag, int r, int g) {
  if (flag) {
    digitalWrite (r, LOW);
    digitalWrite (g, HIGH);
  } else {
    digitalWrite (r, HIGH);
    digitalWrite (g, LOW);
  }
}

// Рестарт таймера
void startTimer (int tim = 0) {
  if (tim == 0) tim = TIME;
  needBlinkEnd = (tim > 1000);
  timerTime = millis() + tim;
}

// Проверка таймера и выполнение нужных действий
bool checkTimer () {
  int a;
  if ((timerTime > 0) && (millis() >= timerTime)) {
    if (entered == 13) {
      for (a=0;a<12;a++) vals[a] = newVals[a];
    }
    if (entered == 14) {
      for (a=0;a<12;a++) vals[a] = defVals[a];
    }
    if (needBlinkEnd) {
      if (mode > 0) blk (RED, 150, 100, 3);
       else blk (RED, 25, 25, 1);
    }
    timerTime = 0;
    code = "";
    code2 = "";
    code3 = "";
    entered = 0;
    mode = 0;
    return true;
  }
  return false;
}

// Вычисление номера нажатой кнопки по уровню потенциала
int decodeKey (int level) {
  int a, fnd, ptr, i;
  fnd = 100;
  ptr = -1;
  for (a=0;a<12;a++) {
    i = abs (level - vals[a]);
    if (i < fnd) {
      fnd = i;
      ptr = a;
    }
  }
  return ptr;
}

// Ожидание отпускания цифровой кнопки
void waitDigitalKeyOut (int pin) {
  while (digitalRead(pin)) delay(50);
}

// Ожидание отпускания кнопки клавиатуры
void waitAnalogKeyOut (int pin) {
  while (analogRead(pin) > 50) delay(50);
}

// Полный сброс замка
void restorePassCode () {
  CORRECTCODE  = "1234";
  CHPASSCODE = "9876";
  code = "";
  code2 = "";
  code3 = "";
  entered = 0;
  mode = 0;
  saveCodes ();
  blk (RED, 300, 200, 5);
}

// Вход в режим смены кода
void startChangeCode () {
  mode = 1;
  ch_attempt = 0;
  ch_code = "";
  ch_code2 = "";
  entered = 0;
  blk (RED, 50, 50, 5);
  startTimer ();
}

// Управление задвижкой замка
void lock (bool flg) {
  ServaTmr = millis() + 5000;
  if (flg) srv1.write(0);
   else srv1.write(180);
  rg (!flg, RED, GREEN);
  flag = flg;
}

// Обработка нажатия кнопки клавиатуры для каждого кода
String addToCode (String cd, int k, String mx) {
  cd = cd + k;
  int len = mx.length();
  int newlen = cd.length();
  if (newlen > len) cd = cd.substring(newlen-len, len+1);
  return cd;
}

// Обработка нажатия кнопки клавиатуры
void applyKey (int key) {
  code = addToCode (code, key, CORRECTCODE);
  code2 = addToCode (code2, key, CHPASSCODE);
  code3 = addToCode (code3, key, MASTERCODE);
  entered = entered + 1;
}

// Выполнение нужного действия в зависимости от набранного кода
void applyChCode (int key) {
  if (mode == 1) {
    if (key == 1) {
      codeNum = 1;
      mode = 2;
      blk (RED, 75, 25, 2);
    } else
    if (key == 2) {
      codeNum = 2;
      mode = 2;
      blk (RED, 75, 25, 2);
    } else {
      blk (RED, 150, 100, 3);
      startTimer(10);
    }
  } else
  if (mode == 2) {
    if (key > 9) {
      if (ch_code == "") {
        blk (RED, 150, 100, 3);
        startTimer(10);
      } else {
        mode = 3;
        blk (GREEN, 75, 25, 3);
      }
    } else {
      if (ch_code.length() >= 12) {
        blk (RED, 150, 100, 3);
        startTimer(10);
      } else ch_code = ch_code + key;
    }
  } else
  if (mode == 3) {
    if (key > 9) {
      if (ch_code != ch_code2) {
        blk (RED, 150, 100, 3);
        startTimer (10);
      } else {
        if (codeNum == 1) CORRECTCODE = ch_code;
         else CHPASSCODE = ch_code;
        blk (GREEN, 150, 100, 3);
        mode = 0;
        startTimer(10);
        saveCodes();
      }
    } else ch_code2 = ch_code2 + key;
  }
}

// Основной цикл программы
void loop() {
  int v, key;
  if (digitalRead (DOOR) && digitalRead(BTN)) lock (false);
  if (!digitalRead (POWER) || digitalRead(DOOR)) startTimer(10);
  if (digitalRead (POWER) && !digitalRead(DOOR)) {
    if (checkTimer ()) {
      lock (true);
    }
    if (digitalRead (BTN)) {
      waitDigitalKeyOut (BTN);
      if (entered >=15) entered = 0;
      lock (!flag);
      startTimer();
    }
    if (entered < MAXENTER) {
      v = analogRead (KEYS);
      key = decodeKey (v);
      if (key > -1) {
        waitAnalogKeyOut (KEYS);
        startTimer();
        if (mode == 0) {
          if (entered < 12) newVals[entered] = v;
          applyKey (key);
          if (code == CORRECTCODE) {
            entered = 0;
            lock (false);
            code = "";
          } else lock (true);
          if (code2 == CHPASSCODE) {
            entered = 0;
            code2 = "";
            startChangeCode();
          }
          if (code3 == MASTERCODE) restorePassCode();
        } else applyChCode (key);
      }
    } else {
      blk (RED, 75, 25, 1);
      blk (GREEN, 75, 25, 1);
    }
  }
  delay (100);
}
