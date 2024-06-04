#include <Arduino.h>
#include <Wire.h>
#include "SSD1306Wire.h"
#include "OLEDDisplayUi.h"
#include "ControllerTouch.h"
#include "ControllerGps.h"

// Definiciones y constantes
#define DISPLAY_ADDRESS 0x3c
#define SDA_PIN 5
#define SCL_PIN 4
#define FPS 30
#define MAX_SCREENS 5
#define PROGRESS_BAR_UPDATE 5
#define TASK_STACK_SIZE 10000
#define TASK_PRIORITY 1

// Variables para el manejo de las tareas
TaskHandle_t Task1;
TaskHandle_t Task2;

// Inicialización de la pantalla
SSD1306Wire display(DISPLAY_ADDRESS, SDA_PIN, SCL_PIN);
OLEDDisplayUi ui(&display);

// Definición de estructuras y variables
typedef struct {
  String titulo;
  String texto;
  String code;
} modelo_panel;

modelo_panel pantallas[MAX_SCREENS] = {
    {"Pantalla 0", "Texto de la pantalla 0", "pantalla0"},
    {"Pantalla 1", "Texto de la pantalla 1", "pantalla1"},
    {"Pantalla 2", "Texto de la pantalla 2", "pantalla2"},
    {"Pantalla 3", "Texto de la pantalla 3", "pantalla3"},
    {"Pantalla 4", "Texto de la pantalla 4", "pantalla4"}
};

int pantallaActual = 0;
int counter = 0;
int code_selected = 0;
// Variables de estado de conexión
bool gpsConnected = true;
bool gsmConnected = true;

// Arrays de las imágenes (flechas)
const uint8_t arrow_up_bits[] PROGMEM = {
  0x00, 0x18, 0x3C, 0x7E, 0xFF, 0x00, 0x00, 0x00
};

const uint8_t arrow_down_bits[] PROGMEM = {
  0x00, 0x00, 0x00, 0xFF, 0x7E, 0x3C, 0x18, 0x00
};


// Definición de imágenes (GPS y GSM)
#define gps_connected_width 16
#define gps_connected_height 16
static const unsigned char gps_connected_bits[] PROGMEM = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x78, 0x00, 0xCC, 0x00,
   0x84, 0x01, 0x84, 0x01, 0xCC, 0x00, 0x78, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

#define gps_disconnected_width 16
#define gps_disconnected_height 16
static const unsigned char gps_disconnected_bits[] PROGMEM = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x78, 0x00, 0xCC, 0x00,
   0x84, 0x01, 0x84, 0x01, 0x84, 0x01, 0x84, 0x01,
   0xCC, 0x00, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00 };

#define gsm_connected_width 16
#define gsm_connected_height 16
static const unsigned char gsm_connected_bits[] PROGMEM = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0xFE, 0x03, 0x82, 0x02,
   0x82, 0x02, 0x82, 0x02, 0x82, 0x02, 0x82, 0x02,
   0x82, 0x02, 0x82, 0x02, 0xFE, 0x03, 0x00, 0x00 };

#define gsm_disconnected_width 16
#define gsm_disconnected_height 16
static const unsigned char gsm_disconnected_bits[] PROGMEM = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0xFE, 0x03, 0x82, 0x02,
   0x82, 0x02, 0x82, 0x02, 0x82, 0x02, 0x82, 0x02,
   0x82, 0x02, 0x82, 0x02, 0x7C, 0x01, 0x00, 0x00 
   };

void setup() {
  Serial.begin(115200);
  initGps();
  display.init();
  display.setFont(ArialMT_Plain_10);
  ui.disableAllIndicators();
  ui.setTargetFPS(FPS);

  xTaskCreatePinnedToCore(Task1code, "Task1", TASK_STACK_SIZE, NULL, TASK_PRIORITY, &Task1, 1);
  xTaskCreatePinnedToCore(Task2code, "Task2", TASK_STACK_SIZE, NULL, TASK_PRIORITY, &Task2, 1);
}

void mostrarPantalla() {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  
  if (pantallaActual >= 0 && pantallaActual < MAX_SCREENS) {
    // Centramos el título y el texto verticalmente
    display.drawStringMaxWidth(64, 16, 128, pantallas[pantallaActual].titulo); // Y = 16 para centrar el título
    display.drawStringMaxWidth(64, 32, 128, pantallas[pantallaActual].texto); // Y = 32 para centrar el texto
    code_selected = pantallaActual;
    // Mostrar flechas de navegación centradas horizontal y verticalmente
    display.drawXbm(60, 0, 8, 8, arrow_up_bits);     // Flecha hacia arriba (centrada horizontalmente)
    display.drawXbm(60, 56, 8, 8, arrow_down_bits);  // Flecha hacia abajo (centrada horizontalmente)
  } else if (pantallaActual == -10) {
    int progress = (counter / PROGRESS_BAR_UPDATE) % 100;
    display.drawStringMaxWidth(64, 15, 128, pantallas[code_selected].titulo); // Y = 15 para centrar el código seleccionado
    display.drawProgressBar(4, 30, 120, 10, progress); // Centrar la barra de progreso
    display.drawStringMaxWidth(64, 45, 128, "Enviando: " + String(progress) + "%"); // Y = 45 para centrar el texto de progreso 
  }
  // Mostrar el estado de conexión
  display.drawXbm(32, 0, 16, 16, gpsConnected ? gps_connected_bits : gps_disconnected_bits);
  display.drawXbm(70, 0, 16, 16, gsmConnected ? gsm_connected_bits : gsm_disconnected_bits);
  
  display.display();
}

void actualizarPantalla(int accion) {
  if (pantallaActual == -10) {
    if (accion == 1) {
      pantallaActual = 0;
    } else if (accion == 3) {
      pantallaActual = MAX_SCREENS - 1;
    } 
  } else {
    if (accion == 1) {
      pantallaActual++;
      if (pantallaActual >= MAX_SCREENS) pantallaActual = 0;
    } 
    if (accion == 3) {
      pantallaActual--;
      if (pantallaActual < 0) pantallaActual = MAX_SCREENS - 1;
    } 
    if (accion == 2) {
      counter=0;
      pantallaActual = -10;
    }
  }
}

void loop() {
  // No se utiliza loop ya que estamos usando FreeRTOS
  counter++;
  delay(10);
}

void Task1code(void *parameter) {
  for (;;) {
    int touch_res = loopTouch();
    actualizarPantalla(touch_res);
    delay(100); // Añadido para evitar respuestas demasiado rápidas
  }
}

void Task2code(void *parameter) {
  for (;;) {
    mostrarPantalla();    
    delay(100); // Control de la tasa de refresco
  }
}


/*#define TINY_GSM_MODEM_SIM800
#include <TinyGsmClient.h>
#include <PubSubClient.h>

// Define los pines para la comunicación serial con el SIM800
#define RXD2_SIMX 16
#define TXD2_SIMX 17

// Parámetros de la red y MQTT
const char* apn = "igprs.claro.com.ar";
const char* gprsUser = "";
const char* gprsPass = "";
const char* mqttServer = "64.226.117.238";
const int mqttPort = 1883;
const char* mqttUser = "tu_usuario_mqtt";
const char* mqttPassword = "tu_contraseña_mqtt";

// Cliente GSM usando Serial2
TinyGsm modem(Serial2);
TinyGsmClient client(modem);
PubSubClient mqtt(client);

void setup() {
  // Iniciar el Serial para depuración
  Serial.begin(9600);
  delay(10);
  Serial.println("Iniciando...");

  // Configurar comunicación serial con SIM800
  Serial2.begin(9600, SERIAL_8N1, RXD2_SIMX, TXD2_SIMX);
  delay(3000);

  // Verificar respuesta del módem
  if (!testModem()) {
    Serial.println("Fallo en la comunicación con el módem.");
    while(true);
  }

  // Reiniciar y configurar módulo GSM
  modem.restart();
  String modemInfo = modem.getModemInfo();
  Serial.print("Modem: ");
  Serial.println(modemInfo);
   String imei = getModemIMEI();
  Serial.print("IMEI: ");
  Serial.println(imei);
  delay(5000);
  // Conectar a la red GPRS con reintentos
    int maxAttempts = 10;
    int attempts = 0;
    bool connected = false;
    Serial.print("Conectando a la red GPRS...");
    while (!connected && attempts < maxAttempts) {
        if (modem.gprsConnect(apn, gprsUser, gprsPass)) {
            connected = true;
            Serial.println("conectado!");
        } else {
            attempts++;
            Serial.println("falló el intento #" + String(attempts) + ", reintentando...");
            delay(5000); // Esperar 5 segundos antes de reintentar
        }
    }

    if (!connected) {
        Serial.println("No se pudo conectar a la red GPRS después de " + String(maxAttempts) + " intentos.");
        while (true); // Detener ejecución si no se puede conectar
    }

  // Configurar el servidor MQTT
  mqtt.setServer(mqttServer, mqttPort);
  mqtt.setCallback(mqttCallback);

  // Conectar al servidor MQTT
  mqttConnect();
}

void loop() {
  if (!mqtt.connected()) {
    mqttConnect();
  }
  mqtt.loop();
  // Ejemplo de publicación de mensaje
  String message = "Hola Mundo!";
  mqtt.publish("tu/topico", message.c_str());
  delay(10000);  // Enviar mensaje cada 10 segundos
}

void mqttConnect() {
  Serial.print("Conectando a MQTT...");
  while (!mqtt.connect("GsmClientTest")) {
    Serial.print(".");
    delay(5000);
  }
  Serial.println("\nconectado");
}

// Callback para recibir mensajes MQTT
void mqttCallback(char* topic, byte* payload, unsigned int len) {
  Serial.print("Mensaje recibido en: ");
  Serial.print(topic);
  Serial.print(". Mensaje: ");
  for (int i = 0; i < len; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

boolean testModem() {
    Serial.println("\nTesting Modem Response...");
    unsigned long startTime = millis();
    do {
        if (!modem.testAT()) {
            delay(1000);
            Serial.println("Modem did not respond.");
            return false;
        } else {
            Serial.println("Modem responded.");
            return true;
        }
    } while (millis() - startTime < 1000);
}

String getModemIMEI() {
    String imei = modem.getIMEI();
    return imei;
}*/
