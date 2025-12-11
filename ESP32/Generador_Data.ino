
#include <Arduino.h>
#include "hourly_profile_week.h" // <-- asegúrate que este archivo está en la misma carpeta del sketch
#include <WiFi.h>
#include <time.h>

//CREDENCIALES PARA INTERNET
const char* ssid     = "*********";
const char* password = "******";

// Configuracion de Zona Horaria Guayaquil/Quito
const long gmtOffset_sec = -5 * 3600; // -18000 segundos
const int daylightOffset_sec = 0;


// Globales para actualizar la hora
struct tm currentTime;    // copia local de la hora
int currentDayIdx = -1;   // Monday=0 .. Sunday=6
int currentHour = -1;     // 0..23
bool hasTime = false;     // indica si hemos sincronizado alguna vez

// Estructura para devolver los valores de la DATABASE
struct Profile {
  float p_kw;
  float q_kw;
  float voltage_v;
  float current_a;
  bool valid;
};

//Definicion de todas las funciones que utilizamos
void printLocalTime();
int convierteFormatoLunDom(int tm_wday);
bool ActualizarHoraLocal();
Profile getProfileAt(int dayIdx, int hour);
void printProfileData();
void setup() {
  Serial.begin(115200);
  delay(300);

  //PROCEDIMIENTO PARA CONECTAR AL WIFI
      Serial.printf("Conectando a %s\n", ssid);
      WiFi.begin(ssid, password);
      if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("No se pudo conectar a WiFi");
        return;
      }
      Serial.println("WiFi conectado");
  
  //PROCEDIMIENTO PARA ACCEDER A LA HORA
      //  Configura NTP (Quito/Guayaquil UTC-5)
      configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.google.com");
      Serial.println("Esperando sincronización NTP...");
      // Espera a obtener hora
      struct tm timeinfo;
      int tries = 0;
      while (!getLocalTime(&timeinfo) && tries < 15) {
        delay(500);
        tries++;
      }
      if (tries >= 15) Serial.println("Advertencia: no se sincronizó NTP aún");
      else Serial.println("Hora sincronizada");

  printLocalTime();
  printProfileData();
}

void loop() {

  printLocalTime();
  printProfileData();
  delay(60000);
}
//directamente llamamos desde el loop a esta funcion
void printLocalTime() {
  //Actualiza la hora local y veririfica si logró hacerlo con un true
  if (!ActualizarHoraLocal()) {
    Serial.println("No se obtuvo la hora NTP (aún).");
    return;
  }
  //Imprimir el tiempo actual
  char buf[64];
  // Formato: YYYY-MM-DD HH:MM:SS
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &currentTime);
  Serial.println(buf);
}
//convierte el formato de la semana domingo-sabado a lunes-domingo el formato correcto
int convierteFormatoLunDom(int tm_wday) {
  return (tm_wday == 0) ? 6 : (tm_wday - 1);
}
//hace el get a al server NTP y lo copia a la variable global
bool ActualizarHoraLocal(){
  struct tm ti;
  if (!getLocalTime(&ti)) {
    hasTime = false;
    return false;
  }
  // Copiamos la hora a la variable global
  currentTime = ti;
  currentDayIdx = convierteFormatoLunDom(ti.tm_wday);//Convierte el formato de (0=Sunday..6=Saturday) a (Monday=0..Sunday=6)
  currentHour = ti.tm_hour;
  hasTime = true;
  return true;
}
//Consulta en la base de datos local que valores de consumo electrico en esa hora
Profile getProfileAt(int dayIdx, int hour){
  Profile r;
  r.valid = false;
  if (dayIdx < 0 || dayIdx > 6 || hour < 0 || hour > 23) return r;
  r.p_kw = p_kw[dayIdx][hour];
  r.q_kw = q_kw[dayIdx][hour];
  r.voltage_v = voltage_v[dayIdx][hour];
  r.current_a = current_a[dayIdx][hour];
  r.valid = true;
  return r;
}
//Imprime la consulta de la base de datos
void printProfileData(){
  Profile p = getProfileAt(currentDayIdx, currentHour);
  if (!p.valid) {
    Serial.println("Índice fuera de rango al intentar leer el profile.");
    return;
  }
    Serial.printf("Profile: p_kw=%.3f kW, q_kw=%.3f kW, V=%.3f V, I=%.3f A\n",
                              p.p_kw, p.q_kw, p.voltage_v, p.current_a);
}
