//============================================================================
//  sound.h "Playback Control Using DirectSound"
//													Programmed by Keiichi Tago
//----------------------------------------------------------------------------
// Creation started on June 2, 2004.
//============================================================================
#ifndef SOUND_H
#define SOUND_H

#define DIRECTSOUND_VERSION 0x300
#include <dsound.h>
#include <windows.h>

class SoundClass
{
	public:
		SoundClass( void );
		~SoundClass( void );
		bool Init( HWND window_handle, int rate, int sample_count );
		void Release( void );
		bool Play( void );
		bool Stop( void );
		void SetVolume( float volume );
		bool GetStatus( void );
		int AddData( short *data );
	private:
		bool InitFlag;
		LPDIRECTSOUND DirectSound;
		LPDIRECTSOUNDBUFFER PrimaryBuffer;
		LPDIRECTSOUNDBUFFER SecondryBuffer;
		DWORD SampleSize;
		DWORD BufferSize;
		DWORD WriteSize;
		DWORD WritePosition;
		SoundClass( const SoundClass& sound );
		SoundClass& operator = ( const SoundClass &sound );
};

#endif
