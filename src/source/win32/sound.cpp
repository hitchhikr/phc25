#include <windows.h>
#include "sound.h"

// Specifying a link library
#pragma comment( lib, "dxguid.lib" )
#pragma comment( lib, "dsound.lib" )

SoundClass::SoundClass( void )
: InitFlag( false ), DirectSound( NULL ), PrimaryBuffer( NULL ), SecondryBuffer( NULL ), WritePosition( 1 )
{
}

SoundClass::~SoundClass( void )
{
	Release();
}

bool SoundClass::Init( HWND window_handle, int rate, int sample_count )
{
	InitFlag = false;
	DirectSound = NULL;
	PrimaryBuffer = NULL;
	SecondryBuffer = NULL;
	WritePosition = 1;
	SampleSize = sample_count * 2;
	BufferSize = sample_count * 2 * sizeof( short );
	WriteSize = sample_count * sizeof( short );
	if( FAILED( DirectSoundCreate( NULL, &DirectSound, NULL ) ) )
	{
		return false;
	}
	if( FAILED( DirectSound->SetCooperativeLevel( window_handle, DSSCL_PRIORITY ) ) )
	{
		Release();
		return false;
	}
	// Primary Buffer
	DSBUFFERDESC dsbd;
	ZeroMemory( &dsbd, sizeof( dsbd ) );
	dsbd.dwSize = sizeof( dsbd );
	dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
	if( FAILED( DirectSound->CreateSoundBuffer( &dsbd, &PrimaryBuffer, NULL ) ) )
	{
		Release();
		return false;
	}
	WAVEFORMATEX wfex;
	ZeroMemory( &wfex, sizeof( wfex ) );
	wfex.wFormatTag = WAVE_FORMAT_PCM;
	wfex.nChannels = 1;
	wfex.nSamplesPerSec = rate;
	wfex.nBlockAlign = 2;
	wfex.nAvgBytesPerSec = wfex.nSamplesPerSec * wfex.nBlockAlign;
	wfex.wBitsPerSample = 16;
	if( FAILED( PrimaryBuffer->SetFormat( &wfex ) ) )
	{
		Release();
		return false;
	}
	// Secondary Buffer
	PCMWAVEFORMAT pcmwf;
	ZeroMemory( &pcmwf, sizeof( pcmwf ) );
	pcmwf.wf.wFormatTag = WAVE_FORMAT_PCM;
	pcmwf.wf.nChannels = 1;
	pcmwf.wf.nSamplesPerSec = rate;
	pcmwf.wf.nBlockAlign = 2;
	pcmwf.wf.nAvgBytesPerSec = pcmwf.wf.nSamplesPerSec * pcmwf.wf.nBlockAlign;
	pcmwf.wBitsPerSample = 16;
	ZeroMemory( &dsbd, sizeof( dsbd ) );
	dsbd.dwSize = sizeof( dsbd );
	dsbd.dwFlags = DSBCAPS_STICKYFOCUS | DSBCAPS_GETCURRENTPOSITION2;
	dsbd.dwBufferBytes = BufferSize;
	dsbd.lpwfxFormat = (LPWAVEFORMATEX)&pcmwf;
	if( FAILED( DirectSound->CreateSoundBuffer( &dsbd, &SecondryBuffer, NULL ) ) )
	{
		Release();
		return false;
	}
	InitFlag = true;
	return true;
}

void SoundClass::Release( void )
{
	Stop();
	if( SecondryBuffer )
	{
		SecondryBuffer->Release();
		SecondryBuffer = NULL;
	}
	if( PrimaryBuffer )
	{
		PrimaryBuffer->Release();
		PrimaryBuffer = NULL;
	}
	if( DirectSound )
	{
		DirectSound->Release();
		DirectSound = NULL;
	}
	InitFlag = false;
}

bool SoundClass::Play( void )
{
	if( !InitFlag )
	{
		return false;
	}
	WORD *ptr1;
	WORD *ptr2;
	DWORD size1;
	DWORD size2;
	if( DSERR_BUFFERLOST == SecondryBuffer->Lock( 0, BufferSize, (void **)&ptr1, &size1, (void**)&ptr2, &size2, 0 ) )
	{
		SecondryBuffer->Restore();
	}
	if( ptr1 )
	{
		ZeroMemory( ptr1, size1 );
	}
	if( ptr2 )
	{
		ZeroMemory( ptr2, size2 );
	}
	SecondryBuffer->Unlock( ptr1, size1, ptr2, size2 );
	SecondryBuffer->Play( 0, 0, DSBPLAY_LOOPING );
	return true;
}

bool SoundClass::Stop( void )
{
	if( !InitFlag )
	{
		return false;
	}
	SecondryBuffer->Stop();
	return true;
}

void SoundClass::SetVolume( float volume )
{
}

bool SoundClass::GetStatus( void )
{
	DWORD play_cursor;
	if( FAILED( SecondryBuffer->GetCurrentPosition( &play_cursor, NULL ) ) )
	{
		return false;
	}
	if( WritePosition )
	{
		if( play_cursor < WriteSize )
		{
			return true;
		}
	}
	else
	{
		if( play_cursor >= WriteSize )
		{
			return true;
		}
	}
	return false;
}

int SoundClass::AddData( short *data )
{
	if( !InitFlag )
	{
		return -1;
	}
	WORD *ptr1;
	WORD *ptr2;
	DWORD size1;
	DWORD size2;
	DWORD data_size;
	data_size = WriteSize;
	if( SecondryBuffer->Lock( WriteSize * WritePosition, WriteSize, (void **)&ptr1, &size1, (void**)&ptr2, &size2, 0 ) == DSERR_BUFFERLOST )
	{
		SecondryBuffer->Restore();
	}
	if( ptr1 )
	{
		if( data_size > size1 )
		{
			memcpy( ptr1, data, size1 );
			data_size -= size1;
		}
		else if( data_size != 0 )
		{
			memcpy( ptr1, data, data_size );
			data_size = 0;
		}
	}
	if( ptr2 )
	{
		if( data_size > size2 )
		{
			memcpy( ptr2, data, size2 );
			data_size -= size2;
		}
		else if( data_size != 0 )
		{
			memcpy( ptr2, data, data_size );
			data_size = 0;
		}
	}
	SecondryBuffer->Unlock( ptr1, size1, ptr2, size2 );
	WritePosition ++;
	WritePosition &= 1;
	return data_size;
}
