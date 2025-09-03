#ifdef MAIN_APP_LOG_LEVEL 
#define CORE_DEBUG_LEVEL MAIN_APP_LOG_LEVEL
#else
#define CORE_DEBUG_LEVEL ARDUHAL_LOG_LEVEL_INFO
#endif

#include "handler_wifi.h"
#include "wifi_status.h"
#include "Arduino.h"
#include "WiFi.h"
#include "main_config.h"
#include "rgb_led.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

static const char* TAG = "WIFI";

// ---------- CONSTANTES (ajusta si lo necesitas) ----------
constexpr unsigned long INITIAL_CONNECT_TIMEOUT_MS      = 10000UL; // timeout inicial en init_thr_wifi
constexpr unsigned long RECONNECT_CYCLE_TIMEOUT_MS      = 30000UL; // máximo por ciclo de reconexión en thread_wifi
constexpr unsigned long BACKOFF_INITIAL_MS              = 200UL;   // backoff inicial entre intentos
constexpr unsigned long BACKOFF_MAX_MS                  = 5000UL;  // backoff tope
constexpr unsigned int  MAX_RETRIES_BEFORE_FORCE_BEGIN  = 6U;      // tras estos intentos forzamos WiFi.begin()
constexpr unsigned long PAUSE_AFTER_FAILURE_MS          = 10000UL; // espera entre ciclos si no se pudo conectar
constexpr unsigned long CONNECTED_POLL_MS               = 2000UL;  // polling cuando está conectado
constexpr unsigned long DISCONNECTED_SLICE_MS           = 100UL;   // slice para esperar dentro del backoff
constexpr unsigned long TASK_LOOP_DELAY_MS              = 50UL;    // delay mínimo por iteración

//> Macro to use static network

// #define USE_STATIC_NETWORK

#ifdef USE_STATIC_NETWORK
static IPAddress stc_ip(192, 168, 4, 1);
static IPAddress stc_gw(192, 168, 1, 1);
static IPAddress stc_sn(192, 168, 1, 1);

static IPAddress stc_dns0(192, 168, 4, 1);
static IPAddress stc_dns1(192, 168, 1, 1);

static void apply_static_network_config() {
  bool cfg_ok = WiFi.config(stc_ip, stc_gw, stc_gw, stc_dns0, stc_dns1);
  if (!cfg_ok) {
    ESP_LOGW(TAG, "Cant use IP Static");
  }
  ESP_LOGI(TAG, "Static net applied: IP=%s GW=%s SN=%s DNS0=%s DNS1=%s",
    ip.toString().c_str(),
    gw.toString().c_str(),
    sn.toString().c_str(),
    dns0.toString().c_str(),
    dns1.toString().c_str());
}
#endif // USE_STATIC_NETWORK

//* WI-FI Status */
static uint8_t current_wl_status;
uint8_t get_wifi_status() {
  return current_wl_status;
}

void init_thr_wifi() {
  //? main config pre-loaded 
  ESP_LOGI(TAG, "Init thread");

  //? Force mode STA (Client)
  WiFi.mode(WIFI_STA);

  //! Try first connection
  ESP_LOGI(TAG, "Try connect to: %s", cnfg.get_ssid());

  #ifdef USE_STATIC_NETWORK
    apply_static_network_config();
  #endif

  WiFi.begin(cnfg.get_ssid(), cnfg.get_pass());

  unsigned long cnt_timeout = millis();
  unsigned long tick_timeout = INITIAL_CONNECT_TIMEOUT_MS;

  uint8_t wlst = WL_DISCONNECTED;
  while (wlst != WL_CONNECTED) {
    wlst = WiFi.status();

    //> timeout
    if ((millis() - cnt_timeout >= tick_timeout)) break;
    delay(100);
  }

  const char* str_wls = (wlst == WL_CONNECTED) ? "Connected correctly" : "Could not connect";
  ESP_LOGI(TAG, "%s", str_wls);
  current_wl_status = wlst;

  if (wlst == WL_CONNECTED) {
    wrgb_1.switch_color(0, 255, 0);
  } else {
    wrgb_1.switch_color(255, 0, 0);
  }
  delay(200);
  wrgb_1.off();
}

/*
 * Tarea que mantiene la conexión WiFi
 * - Conserva la estructura while(true) que tenías
 * - Mejora: exponential backoff, forzar WiFi.begin() si reconnect falla repetidamente,
 *   evita busy-loop con vTaskDelay, logs más descriptivos y actualización del LED.
 */
void thread_wifi(void* parametres) {
  (void)parametres; // no usamos parámetros, conservando la firma original

  for (;;) {
    current_wl_status = WiFi.status();

    if (current_wl_status != WL_CONNECTED) {
      ESP_LOGE(TAG, "WiFi disconnected (status=%d). Starting reconnect procedure...", (int)current_wl_status);

      unsigned long cycle_start = millis();
      unsigned long backoff = BACKOFF_INITIAL_MS;
      unsigned int attempts = 0;
      bool connected = false;

      while ( (millis() - cycle_start) < RECONNECT_CYCLE_TIMEOUT_MS ) {
        ++attempts;
        ESP_LOGI(TAG, "Reconnect attempt %u (elapsed %lu ms)", attempts, (millis() - cycle_start));

        // Intento rápido de reconexión (si la pila lo permite)
        WiFi.reconnect();

        // Esperamos el periodo de backoff, pero en slices para revisar estado y timeout
        unsigned long waited = 0;
        while (waited < backoff && (millis() - cycle_start) < RECONNECT_CYCLE_TIMEOUT_MS) {
          current_wl_status = WiFi.status();
          if (current_wl_status == WL_CONNECTED) {
            connected = true;
            break;
          }
          vTaskDelay(pdMS_TO_TICKS(DISCONNECTED_SLICE_MS));
          waited += DISCONNECTED_SLICE_MS;
        }

        if (connected || WiFi.status() == WL_CONNECTED) {
          current_wl_status = WL_CONNECTED;
          ESP_LOGI(TAG, "WiFi reconnected after %u attempts (elapsed %lu ms)", attempts, (millis() - cycle_start));
          // Indicar con LED
          wrgb_1.switch_color(0, 255, 0);
          delay(100);
          wrgb_1.off();
          break;
        }

        // Si llevamos varios intentos, forzamos un begin (puede ayudar si la stack perdió estado)
        if (attempts % MAX_RETRIES_BEFORE_FORCE_BEGIN == 0) {
          ESP_LOGW(TAG, "Forcing WiFi.begin() after %u failed reconnect attempts", attempts);
          
          #ifdef USE_STATIC_NETWORK
          apply_static_network_config();
          #endif
          
          WiFi.begin(cnfg.get_ssid(), cnfg.get_pass());
        }

        // Exponential backoff con tope
        backoff = backoff * 2;
        if (backoff > BACKOFF_MAX_MS) backoff = BACKOFF_MAX_MS;
      } // while timeout

      // Evaluar resultado del ciclo
      current_wl_status = WiFi.status();
      if (current_wl_status == WL_CONNECTED) {
        ESP_LOGI(TAG, "Connected at end of reconnect cycle. IP: %s", WiFi.localIP().toString().c_str());
        wrgb_1.switch_color(0, 255, 0);
        delay(100);
        wrgb_1.off();
      } else {
        ESP_LOGE(TAG, "Could not connect within %lu ms. Will wait %lu ms before next cycle.", RECONNECT_CYCLE_TIMEOUT_MS, PAUSE_AFTER_FAILURE_MS);
        // Indicar fallo con LED
        wrgb_1.switch_color(255, 0, 0);
        delay(100);
        wrgb_1.off();

        // Espera más larga antes de volver a intentar para no saturar AP/CPU
        unsigned long slept = 0;
        while (slept < PAUSE_AFTER_FAILURE_MS) {
          vTaskDelay(pdMS_TO_TICKS(200));
          slept += 200;
        }
      }
    } else {
      // Estamos conectados: reportar una sola vez y descansar más tiempo
      static bool reported_connected = false;
      if (!reported_connected) {
        ESP_LOGI(TAG, "WiFi connected (SSID: %s, IP: %s)", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
        // Mostrar brevemente LED verde
        wrgb_1.switch_color(0, 255, 0);
        delay(80);
        wrgb_1.off();
        reported_connected = true;
      }

      // Dormir un tiempo razonable mientras estamos conectados
      vTaskDelay(pdMS_TO_TICKS(CONNECTED_POLL_MS));

      // Si se perdió la conexión, reiniciamos el flag para volver a reportar
      if (WiFi.status() != WL_CONNECTED) {
        reported_connected = false;
      }
    }

    // pequeña pausa general para evitar busy-loop intenso en estados intermedios
    vTaskDelay(pdMS_TO_TICKS(TASK_LOOP_DELAY_MS));
  }

  // nunca debería llegar aquí, pero por limpieza:
  vTaskDelete(NULL);
}