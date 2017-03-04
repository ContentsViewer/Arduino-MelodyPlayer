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

    //3,11pinは使えない
    class ParamInternal
    {
    public:
        unsigned char speakerPin;
        unsigned long noteStartTime;
        int noteIndex;
        int noteCount;
        uint16_t *playingMelody;
        PLAY_MODE playMode;
    };

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

    template<int SIZE>
    static void Play(const uint16_t(&melody)[SIZE], PLAY_MODE mode)
    {
        paramInternal.noteCount = SIZE / 2;
        paramInternal.noteIndex = 0;
        paramInternal.playingMelody = &melody[0];
        paramInternal.noteStartTime = millis();
        paramInternal.playMode = mode;

        tone(paramInternal.speakerPin, pgm_read_word_near(&(melody[paramInternal.noteIndex * 2])));
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
        tone(MelodyPlayer::paramInternal.speakerPin, pgm_read_word_near(&(MelodyPlayer::paramInternal.playingMelody[MelodyPlayer::paramInternal.noteIndex * 2])));
    }
}

#endif