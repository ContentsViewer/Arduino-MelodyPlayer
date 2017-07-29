/*
 *  このプログラムはArduinoで簡単にメロディ再生を行うものです.
 *  Melody.h 内で定義されたメロディを再生することができます.
 *  
 *  使用されるリソース:
 *   TIMER1
 *   TIMER2
 *   
 *   以上から, このライブラリpin3,9,10,11のpwm出力を妨げます.
 *   
 *   今後, 使用するTIMERを一つにする予定です.
 *  
 *  対応条件:
 *   Arduino Uno, Mega
 *   
 *  再生できる周波数範囲:
 *   0Hz, 31Hz ~ 65535Hz
 *   
 *   範囲外の周波数を鳴らすことはできません.
 *   周波数0Hzは無音と解釈されます.
 *   
 *  Melody.hの書式:
 *   Melody.hには, 鳴らしたいメロディの定義を記述します.
 *   
 *   書式は以下のとおりです.
static const uint16_t "メロディー名"[] PROGMEM =
{
  //ここにメロディーの内容を書く.
  //周波数, 時間(ミリ秒),
  //周波数, 時間(ミリ秒)
  //....
  //と記述していく
};

 *   
 */

#ifndef _INCLUDE_MELODY_PLAYER_H_
#define _INCLUDE_MELODY_PLAYER_H_



//#define __PROG_TYPES_COMPAT__  //prog_uint8_t, prog_uint32_tを宣言するのに必要


#include <avr/pgmspace.h>

#include "Pitches.h"
#include "Melody.h"

#include "Arduino.h"


class MelodyPlayer
{
  private:
    
  public:
    enum PLAY_MODE
    {
      NORMAL,
      LOOP,
      WAIT
    };

    
    class ParamInternal
    {
      public:
        unsigned char speakerPin;
        unsigned long noteStartTime;
        int noteIndex;
        int noteCount;
        const uint16_t *playingMelody;
        PLAY_MODE playMode;
    };

    //
    // ライブラリ内部で使用するパラメータ群
    // アクセスレベルがパブリックですが, この変数をアプリケーション側で変更しないでください.
    //
    static ParamInternal paramInternal;

    static void Begin(unsigned char speakerPin)
    {
      //パラメータ設定
      paramInternal.speakerPin = speakerPin;

      //TIMER1設定
      //8bit高速PWM, 波形出力なし
      TCCR1A = _BV(WGM10);
      TCCR1B = _BV(WGM12) | _BV(CS10);

    }

    static void End()
    {
      Stop();
    }

    static bool IsPlaying()
    {
      return TIMSK1 & _BV(TOIE1);
    }
    
    template<int SIZE>
    static void Play(const uint16_t(&melody)[SIZE], PLAY_MODE mode)
    {
      paramInternal.noteCount = SIZE / 2;
      paramInternal.noteIndex = 0;
      paramInternal.playingMelody = &melody[0];
      paramInternal.noteStartTime = millis();
      paramInternal.playMode = mode;
      
      ToneInternal(pgm_read_word_near(&(melody[paramInternal.noteIndex * 2])));
      //タイマー割り込み開始
      TIMSK1 |= _BV(TOIE1);

      if (mode == PLAY_MODE::WAIT)
      {
        while (TIMSK1 & _BV(TOIE1));
      }
    }
    
    static void Stop()
    {
      noTone(paramInternal.speakerPin);
      paramInternal.noteCount = 0;
      paramInternal.noteIndex = 0;
      paramInternal.playingMelody = 0x00;
      paramInternal.playMode = PLAY_MODE::NORMAL;

      //タイマー割り込み終了
      TIMSK1 &= ~_BV(TOIE1);
    }

    //
    // ライブラリ内部で使用するTone関数
    // Arduinoデフォルトで用意されているtone関数と異なり, この関数はfreqが0のとき音を鳴らさない.
    // tone関数では, 周波数0Hz時の動作が保証されていない.
    //
    static void ToneInternal(unsigned int freq)
    {
      if (freq == 0)
      {
        noTone(paramInternal.speakerPin);
      }
      else
      {
        tone(paramInternal.speakerPin, freq);
      }
    }
};


MelodyPlayer::ParamInternal MelodyPlayer::paramInternal;

ISR(TIMER1_OVF_vect)
{
  unsigned long time = millis();
  //各ノートの再生時間を超えたとき
  if (time - MelodyPlayer::paramInternal.noteStartTime
      > pgm_read_word_near(&(MelodyPlayer::paramInternal.playingMelody[MelodyPlayer::paramInternal.noteIndex * 2 + 1])))
  {
    //次のノードへ
    MelodyPlayer::paramInternal.noteIndex++;

    //すべてのノードが再生されたとき
    if (MelodyPlayer::paramInternal.noteIndex >= MelodyPlayer::paramInternal.noteCount)
    {
      switch (MelodyPlayer::paramInternal.playMode)
      {
        case MelodyPlayer::PLAY_MODE::NORMAL:
        case MelodyPlayer::PLAY_MODE::WAIT:
          MelodyPlayer::Stop();
          return;

        case MelodyPlayer::PLAY_MODE::LOOP:
          MelodyPlayer::paramInternal.noteIndex = 0;
          break;
      }

    }

    MelodyPlayer::paramInternal.noteStartTime = time;
    MelodyPlayer::ToneInternal(pgm_read_word_near(&(MelodyPlayer::paramInternal.playingMelody[MelodyPlayer::paramInternal.noteIndex * 2])));
  }
}

#endif
