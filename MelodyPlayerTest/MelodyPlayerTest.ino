#include "MelodyPlayer.h"

MelodyPlayer melodyPlayer;

void setup() {
  // put your setup code here, to run once:
  melodyPlayer.Begin(10);
  

}

void loop() {
  // put your main code here, to run repeatedly:
  melodyPlayer.Play(melody, MelodyPlayer::PLAY_MODE::WAIT);
  delay(3000);
}
