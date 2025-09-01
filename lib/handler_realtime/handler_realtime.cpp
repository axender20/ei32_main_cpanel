#ifdef MAIN_APP_LOG_LEVEL 
#define CORE_DEBUG_LEVEL MAIN_APP_LOG_LEVEL
#else
#define CORE_DEBUG_LEVEL ARDUHAL_LOG_LEVEL_INFO
#endif

#include "handler_realtime.h"
#include "Arduino.h"
#include "wifi_status.h"
#include "SupabaseRealtimeClient.h"
#include "realtime_status.h"
#include "utilities_realtime.h"
#include "alarms.h"
#include "trigger_update.h"
#include "esp_system.h"

static const char* TAG = "RLTM";

//> Realtime status
uint8_t realtime_status = RL_STATE_DISCONNECTED;
uint8_t get_realtime_status() {
  return realtime_status;
}

void init_thr_realtime() {
  ESP_LOGI(TAG, "Init thread");
}

static void task_realtime();
void thread_realtime(void* parametres) {
  //? Intervalo del thread (1 ticks)
  static TickType_t lw_thread_rtime, interval_thread_rtime = 1;

  BaseType_t delayed;
  lw_thread_rtime = xTaskGetTickCount();
  while (1) {
    task_realtime();
    delayed = xTaskDelayUntil(&lw_thread_rtime, interval_thread_rtime);
    if (delayed == pdFALSE) {
      lw_thread_rtime = xTaskGetTickCount();
    }
  }
  vTaskDelete(NULL);
}

//> --- Thread settings ---
static const unsigned long BACKOFF_BASE_MS = 500; // 500 ms
static const unsigned long BACKOFF_MAX_MS = 60000; // 60 s tope
static const int MAX_ATTEMPTS_BEFORE_RECREATE = 5; // tras 5 reintentos fallidos, recrear cliente
static const int MAX_ATTEMPTS_BEFORE_COOLDOWN = 10; // tras 10 fallos, entrar cooldown largo
static const unsigned long COOLDOWN_MIN_MS = 5UL * 60UL * 1000UL; // 5 min
static const unsigned long COOLDOWN_MAX_MS = 10UL * 60UL * 1000UL; // 10 min
static const int JITTER_PERCENT = 20; // +/- 20%
// -------------------------------------------

//> Supabase realtime
SupabaseRealtimeClient* supa = nullptr;
String supabaseURL = "uqmexwvdctoznkqhopry.supabase.co";
String apikey = "sb_publishable_nCoEPBqUzH2_skM_4NhCiw_y4nFUM94";

void onChange(const SupabaseRealtimeClient::Change& ch) {
  ESP_LOGI(TAG, "Cambio recibido:");
  ESP_LOGI(TAG, "  message: %s", ch.message.c_str());

  bool enable_alarm = true;
  bool enable_updates = false;

  if (ch.message.equals("EI32_UPDATE")) {
    ESP_LOGI(TAG, "Request update. Initializing...");
    try_update_controller();
  }
  else {
    //? Revisar si tiene el formato normal
    bool rcvdata[6];
    if (parseBinaryArray(ch.message, rcvdata)) {
      set_alarms(rcvdata);
    }
  }
}

static unsigned long apply_jitter(unsigned long base_ms) {
  if (base_ms == 0) return 0;
  unsigned long jitter_range = (base_ms * JITTER_PERCENT) / 100;
  // esp_random devuelve uint32_t
  uint32_t r = esp_random();
  // rango [-jitter_range, +jitter_range]
  long signed_jitter = (long)(r % (2 * jitter_range + 1)) - (long)jitter_range;
  long long result = (long long)base_ms + signed_jitter;
  if (result < 0) result = 0;
  return (unsigned long)result;
}

static void schedule_cooldown(unsigned long& cooldown_until) {
  uint32_t r = esp_random();
  unsigned long range = COOLDOWN_MAX_MS - COOLDOWN_MIN_MS;
  unsigned long add = (range == 0) ? 0 : (r % range);
  cooldown_until = millis() + COOLDOWN_MIN_MS + add;
}

static bool last_error_contains(const String& err, const char* pat) {
  if (err.length() == 0) return false;
  String e = err;
  e.toLowerCase();
  String p = String(pat);
  p.toLowerCase();
  return e.indexOf(p) >= 0;
}

static void try_refresh_credentials() {
  // TODO: implementa tu lógica de refresh aquí (si aplicable)
  // Por ejemplo: pedir token a tu servidor, actualizar 'apikey' y persistirla.
  // Actualmente esto es un stub para indicar dónde implementar.
}

static void create_and_start_client() {
  if (supa) return; // ya creado
  supa = new SupabaseRealtimeClient();
  if (!supa) return; // allocation failed
  
  supa->init(supabaseURL, apikey, true, 443);
  supa->setChangeCallback(onChange);
  supa->setLogLevel(SupabaseRealtimeClient::LogLevel::LOG_NONE);
  // DESHABILITAR reconexion automática del WebSocketsClient: gestionar manualmente
  supa->setReconnectDelay(0);
  supa->begin();
}

static void stop_and_destroy_client() {
  if (!supa) return;
  supa->close();
  delete supa;
  supa = nullptr;
}

//> Thread realtime
static void task_realtime() {
  static bool config = false;
  static int prv_status = SupabaseRealtimeClient::STATE_ERROR;

  static unsigned long backoff_ms = BACKOFF_BASE_MS;
  static int consecutive_failures = 0;
  static unsigned long next_attempt_at = 0;
  static unsigned long cooldown_until = 0;

  const TickType_t loop_delay_ticks = pdMS_TO_TICKS(50);

  if (get_wifi_status() == WL_CONNECTED) {
    unsigned long now = millis();

    // Si no estamos configurados o cliente destruido, intentar crear si llegó el momento
    if (!config) {
      if (now < cooldown_until) {
        // en cooldown: no intentamos nada
      }
      else if (now >= next_attempt_at) {
        // Intentar crear e iniciar cliente
        ESP_LOGI(TAG, "Connected. Initializing realtime client...");
        create_and_start_client();
        if (supa) {
          config = true;
          backoff_ms = BACKOFF_BASE_MS;
        }
        else {
          unsigned long jittered = apply_jitter(backoff_ms);
          next_attempt_at = now + jittered;
          backoff_ms = min(backoff_ms * 2UL, BACKOFF_MAX_MS);
        }
      }
    }

    //? Typical
    if (config && supa) {
      supa->loop();

      int c_status = supa->getState();
      realtime_status = c_status;

      if (prv_status != c_status) {
        prv_status = c_status;
        ESP_LOGI(TAG, "Status: %s", supa->getStateString().c_str());
      }

      if (supa->isConnected() || supa->isJoined()) {
        // Conexión exitosa: resetear contadores y backoff
        consecutive_failures = 0;
        backoff_ms = BACKOFF_BASE_MS;
        next_attempt_at = 0;
      }
      else if (c_status == SupabaseRealtimeClient::STATE_DISCONNECTED || c_status == SupabaseRealtimeClient::STATE_ERROR) {
        String lastErr = supa->getLastError();
        enum ErrorClass { NO_INTERNET, AUTH_FAILED, TLS_ERROR, PROTOCOL_ERROR } eclass = PROTOCOL_ERROR;

        if (last_error_contains(lastErr, "401") || last_error_contains(lastErr, "403") || last_error_contains(lastErr, "auth") || last_error_contains(lastErr, "apikey") || last_error_contains(lastErr, "unauthorized")) {
          eclass = AUTH_FAILED;
        }
        else if (last_error_contains(lastErr, "ssl") || last_error_contains(lastErr, "tls") || last_error_contains(lastErr, "certificate")) {
          eclass = TLS_ERROR;
        }
        else if (last_error_contains(lastErr, "timeout") || last_error_contains(lastErr, "connect") || last_error_contains(lastErr, "no route") || last_error_contains(lastErr, "refused")) {
          eclass = NO_INTERNET;
        }
        else {
          eclass = PROTOCOL_ERROR;
        }

        ESP_LOGI(TAG, "Realtime client error class=%d reason=%s", (int)eclass, lastErr.c_str());

        switch (eclass) {
        case NO_INTERNET:
          stop_and_destroy_client();
          config = false;
          {
            unsigned long jittered = apply_jitter(backoff_ms);
            next_attempt_at = millis() + jittered;
            backoff_ms = min(backoff_ms * 2UL, BACKOFF_MAX_MS);
          }
          break;

        case AUTH_FAILED:
          try_refresh_credentials();
          stop_and_destroy_client();
          config = false;
          {
            unsigned long jittered = apply_jitter(backoff_ms);
            next_attempt_at = millis() + jittered;
            backoff_ms = min(backoff_ms * 2UL, BACKOFF_MAX_MS);
          }
          break;

        case TLS_ERROR:
          stop_and_destroy_client();
          config = false;
          {
            unsigned long jittered = apply_jitter(backoff_ms);
            next_attempt_at = millis() + jittered;
            backoff_ms = min(backoff_ms * 2UL, BACKOFF_MAX_MS);
          }
          break;

        case PROTOCOL_ERROR:
        default:
          stop_and_destroy_client();
          config = false;
          {
            unsigned long jittered = apply_jitter(backoff_ms);
            next_attempt_at = millis() + jittered;
            backoff_ms = min(backoff_ms * 2UL, BACKOFF_MAX_MS);
          }
          break;
        }

        consecutive_failures++;

        if (consecutive_failures >= MAX_ATTEMPTS_BEFORE_RECREATE) {
          ESP_LOGI(TAG, "Consecutive failures >= %d. Will recreate client on next attempt.", MAX_ATTEMPTS_BEFORE_RECREATE);
          backoff_ms = BACKOFF_BASE_MS; // Reinicia backoff para la nueva creación
        }

        if (consecutive_failures >= MAX_ATTEMPTS_BEFORE_COOLDOWN) {
          ESP_LOGI(TAG, "Reached max failures (%d). Entering long cooldown.", MAX_ATTEMPTS_BEFORE_COOLDOWN);
          schedule_cooldown(cooldown_until);
          consecutive_failures = 0;
          backoff_ms = BACKOFF_BASE_MS;
          next_attempt_at = cooldown_until;
        }
      }
    }

  }
  else {
    if (config) {
      ESP_LOGI(TAG, "WiFi lost. Closing realtime client...");
      stop_and_destroy_client();
      config = false;
    }
  }

  vTaskDelay(loop_delay_ticks);
}
