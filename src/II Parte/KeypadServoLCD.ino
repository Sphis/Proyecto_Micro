// Basado en https://www.instructables.com/Connecting-a-4-x-4-Membrane-Keypad-to-an-Arduino/
// Para este caso 8, 7, 6, 5 keypad con 13, 12, 11, 10 en el arduino respectivamente
// 4, 3, 2, 1 keypad con 9, 8, 7, 6 en el arduino
// Para alimentar el servo, se usa el pin de 3.3 V
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

Servo myservo; // Objeto servo

int DoorFlag = 0; // Puerta cerrada: DoorFlag = 0, puerta abierta: DoorFlag = 1. Se asume que la puerta empieza cerrada
int cursor = 0; // Posicion de impresion del display, empieza en fila 1

const byte ROWS = 4; // Filas
const byte COLS = 4; // Columnas
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {13, 12, 11, 10}; // Pines del arduino a usar para filas, empieza con el pin 8 del keypad
byte colPins[COLS] = {9, 8, 7, 6}; // Pines del arduino a usar para columnas

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );
LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C address 0x27 (from DIYables LCD), 16 column and 2 rows

const String password = "1234"; // La clave a usar
String input_password;

void setup(){
  Serial.begin(9600);
  input_password.reserve(32); // Se pueden reservar un maximo de 33 caracteres

  // Iniciar servo
  myservo.attach(5);  // Servo usa el pin 5
  myservo.write(40);   // Iniciar en 40 grados de la referencia

  // Iniciar display
  lcd.init(); // Initialize the LCD I2C display
  lcd.backlight();

  // Poner pin digital 2 en alto para el display
  //pinMode(3, OUTPUT);
}

// Funcion que reseta los valores en el display
int reset_disp() {
  cursor = 0; // Volver a posicion inicial
  lcd.clear();
  lcd.setCursor(0,0); // Columna 0, fila 0
  lcd.print("Clave:");
  return cursor;
}

void loop(){
  //digitalWrite(3, HIGH); // Poner pin en alto para alimentar display
  char key = keypad.getKey();
  lcd.setCursor(0,0); // Columna 0, fila 0
  lcd.print("Clave:");

  if (key){
    lcd.setCursor(cursor, 1); // Mover puntero a columna 0, fila cursor
    lcd.print(key);
    cursor++; // Aumentar posicion

    Serial.println(key);

    if (cursor == 16) { // Limpiar display si se llega a la ultima posicion
      reset_disp();
      lcd.setCursor(0, 1);
      lcd.print("Clave excedida");
      delay(2500); // Mostrar el msj por tres segundos
      reset_disp();
    }

    if(key == '*') {
      Serial.println("Reset");
      lcd.setCursor(0, 1);
      lcd.print("Reset");
      delay(2500); // Mostrar el msj por tres segundos
      reset_disp();
      input_password = ""; // Limpiar contrasena con tecla *
    }
    else if(key == '#') {
      // Caso de contrasena correcta
      if(password == input_password) {
        Serial.println("Clave correcta");
        lcd.setCursor(0, 1);
        lcd.print("Clave correcta");
        delay(2500); // Mostrar el msj por tres segundos
        reset_disp();

        // Abrir la puerta
        if (DoorFlag == 0) {
          DoorFlag = 1; // Estado de puerta abierta
          for (int pos = 40; pos <= 130; pos += 1) {  // rotar de 40 a 130 grados en pasos de 1 grado
            myservo.write(pos);
            delay(10); // Introducir delay para que no sea tan rapido
          }
        }
      }
      else {
        Serial.println("Contraseña incorrecta, intente de nuevo");
        lcd.setCursor(0, 1);
        lcd.print("Clave incorrecta");
        delay(2500); // Mostrar el msj por tres segundos
        reset_disp();
      }
      input_password = ""; // Limpiar
    }
    else if(key == 'A') { // Para cerrar la puerta se pulsa la tecla A
      if(DoorFlag == 1) {
        DoorFlag = 0; // Estado de puerta cerrada
        Serial.println("Cerrar puerta");

        lcd.setCursor(0, 1);
        lcd.print("Puerta Cerrando");
        delay(2500); // Mostrar el msj por tres segundos
        reset_disp();

        // Cerrar la puerta con la tecla A
        for (int pos = 130; pos >= 40; pos -= 1) {  // rotar de 130 a 40 grados
          myservo.write(pos);
          delay(10);
        }
        input_password = ""; // Limpiar
      }
    }
    else {
      input_password += key; // Agregar nuevo caracter
    }
  }
}
