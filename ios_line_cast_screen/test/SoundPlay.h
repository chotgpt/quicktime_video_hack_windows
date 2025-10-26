#pragma once
#include <SDL.h>

#include <iostream>

class SoundPlay
{
public:
	SoundPlay(){ }
	~SoundPlay(){ }

	bool Init();
	void UnInit();
	void PauseAudioDevice(int nPauseOn);	// 0²¥·Å£¬·Ç0ÔÝÍ£
	bool SetFormat(int nChannels, int nSamplerate);
	void PushData(void* pBuffer, int nBuffLeng);
private:
	SDL_AudioSpec		m_AudioSpec;
	SDL_AudioDeviceID	m_AudioDeviceID;
};

