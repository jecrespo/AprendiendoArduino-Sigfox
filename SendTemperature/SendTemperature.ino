#include <SigFox.h>
#include <ArduinoLowPower.h>
#include <Timer.h>
#include <dht.h>

#define SIGFOX_FRAME_LENGTH 12
#define DHT22_PIN 7

// Set debug to false to enable continuous mode
// and disable serial prints
int debug = false;

Timer t;
dht DHT;

float umbral = 22;
int alarm_state = 0;  // alarm 0 = normal, 1 = alarm

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  if (debug == true) {
    Serial.begin(9600);
    while (!Serial) {}
  }

  if (!SigFox.begin()) {
    //something is really wrong, try rebooting
    reboot();
  }

  //Send module to standby until we need to send a message
  SigFox.end();

  if (debug == true) {
    // Enable debug prints and LED indication if we are testing
    SigFox.debug();
  }

  t.every(30000, takeReading);
  takeReading();  //When restart send data
}

void loop()
{
  t.update();
}

void reboot() {
  NVIC_SystemReset();
  while (1);
}

void takeReading() {
  // READ DATA
  Serial.print("DHT22, \t");

  int chk = DHT.read22(DHT22_PIN);

  switch (chk)
  {
    case DHTLIB_OK:
      Serial.print("OK,\t");
      break;
    case DHTLIB_ERROR_CHECKSUM:
      Serial.print("Checksum error,\t");
      break;
    case DHTLIB_ERROR_TIMEOUT:
      Serial.print("Time out error,\t");
      break;
    default:
      Serial.print("Unknown error,\t");
      break;
  }

  if (chk == DHTLIB_OK) {
    if (debug == true) {
      Serial.println("Sending data....");
      Serial.print(DHT.temperature, 1);
      Serial.print(",\t");
      Serial.print(DHT.humidity, 1);
      Serial.print(",\t");
    }

    SigFox.begin();

    if (debug == true) {
      Serial.println("Send Data");
    }
    delay(100);

    float temp = DHT.temperature;
    float hum = DHT.humidity;
    char alarm = '0';  // alarm: 0 = normal (no event), 1 = alarm triggered, 2 = restore alarm

    if ((temp > umbral) && (alarm_state == 0)) {
      alarm_state = 1;
      alarm = '1'; //alarm triggered
    }

    if ((temp < umbral) && (alarm_state == 1)) {
      alarm_state = 0;
      alarm = '2'; //alarm restored
    }

    if (debug == true) {
      Serial.print("Datos enviados: ");
      Serial.print(temp);
      Serial.print(hum);
      Serial.println(alarm);
      Serial.print("Tamaño datos enviados: ");
      Serial.println((String(temp) + String(hum) + String(alarm)).length());  //Todo lo que se manda es en caracteres
    }

    // 5 bytes (temp) + 5 bytes (hum) + 1 byte (alarm)  < 12 bytes
    // backend configuration temperature::char:5 humidity::char:5 alarm::char:1
    SigFox.beginPacket();
    SigFox.print(temp); //a value to send characters representing the value
    SigFox.print(hum);
    SigFox.print(alarm);
    int ret = SigFox.endPacket();

    if (debug == true) {
      Serial.println(SigFox.status(SIGFOX));
      Serial.println(SigFox.status(ATMEL));
    }

    // shut down module, back to standby
    SigFox.end();

    if (debug == true) {
      if (ret > 0) {
        Serial.println("No transmission");
      } else {
        Serial.println("Transmission ok");
      }
    }
    else {
      if (ret > 0) {
        digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
        delay(1000);                       // wait for a second
        digitalWrite(LED_BUILTIN, LOW);
      }
    }
  }
}
