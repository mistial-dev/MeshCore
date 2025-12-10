#include <Arduino.h>   // needed for PlatformIO
#include <Mesh.h>

#include "MyMesh.h"
#if defined(PIN_BUZZER)
#include "helpers/ui/buzzer.h"
static genericBuzzer dispatch_buzzer;
static bool gps_fix_tone_done = false;
#endif

#ifdef DISPLAY_CLASS
  #include "UITask.h"
  static UITask ui_task(display);
#endif

StdRNG fast_rng;
SimpleMeshTables tables;
MyMesh the_mesh(board, radio_driver, *new ArduinoMillis(), fast_rng, rtc_clock, tables);

void halt() {
  while (1) ;
}

static char command[MAX_POST_TEXT_LEN+1];
static uint32_t boot_reset_reason = 0;
static bool reset_reason_printed = false;
static bool wdt_initialized = false;

// Feed the watchdog; safe to call even if not yet initialized.
void kickWatchdog() {
#if defined(NRF52_PLATFORM)
  if (wdt_initialized) {
    NRF_WDT->RR[0] = WDT_RR_RR_Reload;
  }
#endif
}

void setup() {
  Serial.begin(115200);
  boot_reset_reason = NRF_POWER->RESETREAS;
  NRF_POWER->RESETREAS = boot_reset_reason;  // clear for next time
  delay(1000);

  board.begin();
#if defined(PIN_BUZZER)
  dispatch_buzzer.begin();
  dispatch_buzzer.quiet(false);
  dispatch_buzzer.startup();
#endif

#ifdef DISPLAY_CLASS
  if (display.begin()) {
    display.startFrame();
    display.setCursor(0, 0);
    display.print("Please wait...");
    display.endFrame();
  }
#endif

  if (!radio_init()) { halt(); }

  fast_rng.begin(radio_get_rng_seed());

  FILESYSTEM* fs;
#if defined(NRF52_PLATFORM)
  InternalFS.begin();
  fs = &InternalFS;
  IdentityStore store(InternalFS, "");
#elif defined(RP2040_PLATFORM)
  LittleFS.begin();
  fs = &LittleFS;
  IdentityStore store(LittleFS, "/identity");
  store.begin();
#elif defined(ESP32)
  SPIFFS.begin(true);
  fs = &SPIFFS;
  IdentityStore store(SPIFFS, "/identity");
#else
  #error "need to define filesystem"
#endif
  if (!store.load("_main", the_mesh.self_id)) {
    the_mesh.self_id = radio_new_identity();   // create new random identity
    int count = 0;
    while (count < 10 && (the_mesh.self_id.pub_key[0] == 0x00 || the_mesh.self_id.pub_key[0] == 0xFF)) {  // reserved id hashes
      the_mesh.self_id = radio_new_identity(); count++;
    }
    store.save("_main", the_mesh.self_id);
  }

  Serial.print("Room ID: ");
  mesh::Utils::printHex(Serial, the_mesh.self_id.pub_key, PUB_KEY_SIZE); Serial.println();

  command[0] = 0;

  sensors.begin(); // dispatch should try to provide location if hardware permits

  the_mesh.begin(fs);

#ifdef DISPLAY_CLASS
  ui_task.begin(the_mesh.getNodePrefs(), FIRMWARE_BUILD_DATE, FIRMWARE_VERSION);
#endif

  // Enable watchdog to recover from hangs. 8-second timeout, fed in loop().
  if (!wdt_initialized) {
    // Use the SoftDevice watchdog API if present; otherwise use nrfx.
    NRF_WDT->CONFIG = (WDT_CONFIG_SLEEP_Run << WDT_CONFIG_SLEEP_Pos) |
                      (WDT_CONFIG_HALT_Pause << WDT_CONFIG_HALT_Pos);
    NRF_WDT->CRV = 8 * 32768; // ~8 seconds
    NRF_WDT->RREN |= WDT_RREN_RR0_Msk;
    NRF_WDT->TASKS_START = 1;
    wdt_initialized = true;
  }

  // send out initial Advertisement to the mesh
  the_mesh.sendSelfAdvertisement(16000);
#if defined(PIN_BUZZER)
  // Light chirp after initial advert to signal readiness.
  dispatch_buzzer.play("Ready:d=4,o=5,b=180:8a6");
#endif
}

void loop() {
  if (!reset_reason_printed && Serial) {
    Serial.printf("ResetReason=0x%08lx\n", (unsigned long)boot_reset_reason);
    reset_reason_printed = true;
  }

  int len = strlen(command);
  while (Serial.available() && len < sizeof(command)-1) {
    char c = Serial.read();
    if (c == '\b' || c == 0x7f) {  // backspace/delete
      if (len > 0) {
        len--;
        command[len] = 0;
        Serial.print("\b \b"); // erase the last character on terminal
      }
      continue;
    }
    if (c != '\n') {
      command[len++] = c;
      command[len] = 0;
      Serial.print(c);
      continue;
    }
  }
  if (len == sizeof(command)-1) {  // command buffer full
    command[sizeof(command)-1] = '\r';
  }

  if (len > 0 && command[len - 1] == '\r') {  // received complete line
    command[len - 1] = 0;  // replace newline with C string null terminator
    char reply[160];
    the_mesh.handleCommand(0, command, reply);  // NOTE: there is no sender_timestamp via serial!
    if (reply[0]) {
      const size_t chunk = 120; // avoid huge lines; safe for most terminals
      const char* p = reply;
      while (*p) {
        Serial.print("  -> ");
        size_t n = 0;
        while (p[n] && n < chunk) n++;
        Serial.write((const uint8_t*)p, n);
        Serial.print("\r\n");
        p += n;
      }
    }

    command[0] = 0;  // reset command buffer
  }

  the_mesh.loop();
  sensors.loop();
#if defined(PIN_BUZZER)
  if (!gps_fix_tone_done) {
    LocationProvider* loc = sensors.getLocationProvider();
    if (loc != NULL && loc->isValid()) {
      // Distinct GPS lock tone: short rising triad to disambiguate from startup chirp.
      dispatch_buzzer.play("GPS:d=16,o=6,b=180:16c,16e,8g,16p,8g");
      gps_fix_tone_done = true;
    }
  }
  dispatch_buzzer.loop();
#endif
#ifdef DISPLAY_CLASS
  ui_task.loop();
#endif
  rtc_clock.tick();

  // Kick the watchdog after main work; if we hang in loop(), we will reset.
  kickWatchdog();
}
