#include "audio/BluetoothPlayer.h"

#include <SD_MMC.h>
#include "Audio.h"
#include "board/BoardConfig.h"

static BluetoothPlayer* s_instance = nullptr;

void audio_eof_mp3(const char* /*info*/) {
  if (s_instance) s_instance->onTrackEnded();
}

void audio_info(const char* info) {
  Serial.printf("[music] %s\n", info);
}

BluetoothPlayer::~BluetoothPlayer() {
  stop();
}

bool BluetoothPlayer::begin() {
  s_instance = this;
  scanTracks();
  audio_ = new Audio();
  audio_->setPinout(BoardConfig::PIN_AUDIO_BCLK,
                    BoardConfig::PIN_AUDIO_WS,
                    BoardConfig::PIN_AUDIO_DOUT,
                    I2S_PIN_NO_CHANGE,
                    BoardConfig::PIN_AUDIO_MCLK);
  audio_->setVolume(volume_);
  if (!tracks_.empty()) {
    startTrack(0);
  }
  return true;
}

void BluetoothPlayer::stop() {
  if (audio_) {
    audio_->stopSong();
    delete audio_;
    audio_ = nullptr;
  }
  playing_ = false;
  if (s_instance == this) s_instance = nullptr;
}

void BluetoothPlayer::loop() {
  if (audio_) audio_->loop();
}

void BluetoothPlayer::togglePlayPause() {
  if (!audio_) return;
  audio_->pauseResume();
  playing_ = !playing_;
}

void BluetoothPlayer::next() {
  if (tracks_.empty()) return;
  startTrack((trackIndex_ + 1) % (int)tracks_.size());
}

void BluetoothPlayer::prev() {
  if (tracks_.empty()) return;
  startTrack((trackIndex_ - 1 + (int)tracks_.size()) % (int)tracks_.size());
}

String BluetoothPlayer::trackDisplayName() const {
  if (trackIndex_ < 0 || trackIndex_ >= (int)names_.size()) return "";
  return names_[trackIndex_];
}

void BluetoothPlayer::volumeUp() {
  if (volume_ < kVolumeMax) volume_++;
  if (audio_) audio_->setVolume(volume_);
}

void BluetoothPlayer::volumeDown() {
  if (volume_ > 0) volume_--;
  if (audio_) audio_->setVolume(volume_);
}

String BluetoothPlayer::trackDisplayNameAt(int index) const {
  if (index < 0 || index >= (int)names_.size()) return "";
  return names_[index];
}

void BluetoothPlayer::playTrack(int index) {
  startTrack(index);
}

void BluetoothPlayer::onTrackEnded() {
  next();
}

void BluetoothPlayer::startTrack(int index) {
  if (!audio_ || tracks_.empty()) return;
  trackIndex_ = index;
  playing_ = true;
  audio_->connecttoFS(SD_MMC, tracks_[index].c_str());
}

void BluetoothPlayer::scanTracks() {
  tracks_.clear();
  names_.clear();
  File dir = SD_MMC.open("/music");
  if (!dir || !dir.isDirectory()) {
    Serial.println("[music] /music not found on SD card");
    return;
  }
  File f = dir.openNextFile();
  while (f) {
    if (!f.isDirectory()) {
      String fname = f.name();
      if (fname.endsWith(".mp3") || fname.endsWith(".MP3")) {
        tracks_.push_back(String("/music/") + fname);
        String name = fname;
        if (name.length() > 4) name = name.substring(0, name.length() - 4);
        names_.push_back(name);
      }
    }
    f = dir.openNextFile();
  }
  Serial.printf("[music] found %d tracks\n", (int)tracks_.size());
}
