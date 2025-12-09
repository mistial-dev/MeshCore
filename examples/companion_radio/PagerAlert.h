#pragma once

#if defined(PAGER_MODE) && defined(PIN_BUZZER)

#include <helpers/ui/buzzer.h>
#include "PagerAlertTones.h"

// Simple alert controller for pager tones. Caller must periodically invoke loop().
class PagerAlert {
public:
  PagerAlert() : _buzzer(nullptr), _active(false), _latched(false), _current(nullptr) {}

  void begin(genericBuzzer* buzzer) { _buzzer = buzzer; }

  void start(PagerAlertLevel level) {
    if (_buzzer == nullptr) return;
    const PagerAlertTone& tone = getPagerAlertTone(level);
    _current = &tone;
    _latched = tone.latched;
    _active = true;
    _buzzer->play(tone.melody);
  }

  void stop() {
    if (_buzzer == nullptr) return;
    _active = false;
    _latched = false;
    _current = nullptr;
    _buzzer->stop();
  }

  void loop() {
    if (_buzzer == nullptr) return;
    _buzzer->loop();
    if (_active && _latched && _current && !_buzzer->isPlaying()) {
      _buzzer->play(_current->melody);
    }
  }

  bool isActive() const { return _active; }
  bool isLatched() const { return _latched; }
  const PagerAlertTone* current() const { return _current; }

private:
  genericBuzzer* _buzzer;
  bool _active;
  bool _latched;
  const PagerAlertTone* _current;
};

#endif // PAGER_MODE && PIN_BUZZER
