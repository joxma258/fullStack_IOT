#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include "hourly_profile_week.h" // <-- asegúrate que este archivo está en la misma carpeta del sketch

// CREDENCIALES PARA INTERNET
const char* ssid     = "------";
const char* password = "-------";

// Configuracion de Zona Horaria Guayaquil/Quito
const long gmtOffset_sec = -5 * 3600; // -18000 segundos
const int daylightOffset_sec = 0;

const char* dias_es_lunes0[] = { "Lunes", "Martes", "Miércoles", "Jueves", "Viernes", "Sábado", "Domingo" };

// Globales para actualizar la hora
struct tm currentTime;    // copia local de la hora
int currentDayIdx = -1;   // Monday=0 .. Sunday=6
int currentHour = -1;     // 0..23
bool hasTime = false;     // indica si hemos sincronizado alguna vez

// Estructura para devolver los valores de la "DB"
struct Profile {
  float p_kw;
  float q_kw;
  float voltage_v;
  float current_a;
  bool valid;
};

// Declaraciones / prototipos correctos
void printLocalTime();
int convierteFormatoLunDom(int tm_wday);
bool ActualizarHoraLocal();
Profile getProfileAt(int dayIdx, int hour);
void printProfileData();

// Temporizador para reintentar WiFi
unsigned long lastWiFiAttempt = 0;
const unsigned long wifiRetryInterval = 15000; // 15s

void setup() {
  Serial.begin(115200);
  delay(300);

  // CONEXIÓN WIFI
  Serial.printf("Conectando a %s\n", ssid);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("No se pudo conectar a WiFi en setup. Seguirá intentando en loop.");
  } else {
    Serial.println("WiFi conectado");
  }

  // CONFIG NTP (Quito/Guayaquil UTC-5)
  configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.google.com");
  Serial.println("Esperando sincronización NTP...");
  struct tm timeinfo;
  int tries = 0;
  while (!getLocalTime(&timeinfo) && tries < 15) {
    delay(500);
    tries++;
  }
  if (tries >= 15) Serial.println("Advertencia: no se sincronizó NTP aún");
  else Serial.println("Hora sincronizada");

  // Primera impresión (si hay hora)
  printLocalTime();
  printProfileData();
}

void loop() {
  // Intento de reconectar WiFi si se desconecta (no bloqueante)
  if (WiFi.status() != WL_CONNECTED) {
    unsigned long now = millis();
    if (now - lastWiFiAttempt > wifiRetryInterval) {
      Serial.println("WiFi desconectado. Intentando reconectar...");
      WiFi.disconnect();
      WiFi.begin(ssid, password);
      lastWiFiAttempt = now;
    }
  }

  // Actualiza la hora y muestra perfil cada 60s
  printLocalTime();
  printProfileData();

  delay(60000);
}

// Imprime la hora local (usa currentTime global)
void printLocalTime() {
  // Actualiza la hora local y verificar si logró hacerlo
  if (!ActualizarHoraLocal()) {
    Serial.println("No se obtuvo la hora NTP (aún).");
    return;
  }
  // Imprimir el tiempo actual
  char buf[64];
  // Formato: YYYY-MM-DD HH:MM:SS
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &currentTime);
  Serial.println(buf);
}

// Convierte tm_wday (0=Sunday..6=Saturday) a Monday=0..Sunday=6
int convierteFormatoLunDom(int tm_wday) {
  return (tm_wday == 0) ? 6 : (tm_wday - 1);
}

// Hace el get al server NTP y copia a la variable global
bool ActualizarHoraLocal(){
  struct tm ti;
  if (!getLocalTime(&ti)) {
    hasTime = false;
    return false;
  }
  // Copiamos la hora a la variable global
  currentTime = ti;
  currentDayIdx = convierteFormatoLunDom(ti.tm_wday); // Convierte (0=Sun..6=Sat) a (Mon=0..Sun=6)
  currentHour = ti.tm_hour;
  hasTime = true;
  return true;
}

// Consulta en la "DB" local que valores de consumo electrico hay en esa hora
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

// Imprime la consulta de la base de datos para "ahora"
void printProfileData(){
  if (!hasTime) {
    Serial.println("Hora no disponible, no se puede consultar profile.");
    return;
  }
  Profile p = getProfileAt(currentDayIdx, currentHour);
  if (!p.valid) {
    Serial.println("Índice fuera de rango al intentar leer el profile.");
    return;
  }
  Serial.printf("Profile (%s %02d): p_kw=%.3f kW, q_kw=%.3f kW, V=%.3f V, I=%.3f A\n",
                dias_es_lunes0[currentDayIdx], currentHour,
                p.p_kw, p.q_kw, p.voltage_v, p.current_a);
}
