#pragma once

#include <Arduino.h>
#include <vector>

class Audio;

class BluetoothPlayer {
 public:
  ~BluetoothPlayer();
  bool begin();
  void stop();
  void loop();
  void togglePlayPause();
  void next();
  void prev();
  bool isPlaying() const { return playing_; }
  bool hasNoTracks() const { return tracks_.empty(); }
  int trackCount() const { return (int)tracks_.size(); }
  int trackIndex() const { return trackIndex_; }
  String trackDisplayName() const;
  void onTrackEnded();

 private:
  void scanTracks();
  void startTrack(int index);

  Audio* audio_ = nullptr;
  std::vector<String> tracks_;
  std::vector<String> names_;
  int trackIndex_ = 0;
  bool playing_ = false;
};
