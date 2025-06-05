#include <windows.h>
#include <mmsystem.h>
#include "waveout.h"

// Specifying a link library
#pragma comment( lib, "Winmm.lib" )

WaveOutClass::WaveOutClass( void )
:MmioHandle( NULL ), Handle( NULL ), ThreadHandle( NULL ), Status( 0 ),
 LoopStart( 0 ), LoopEnd( -1 ), Volume( 1.0f ), NowVolume( 1.0f ), VolumeRate( 0.0f ),
 SampleCount( 0 )
{
}

WaveOutClass::~WaveOutClass( void )
{
}

int WaveOutClass::Init( int buffer_size, int count )
{
	int i;
	// Clear
	MmioHandle = NULL;
	Handle = NULL;
	ThreadHandle = NULL;
	Status = 0;
	LoopStart = 0;
	LoopEnd = -1;
	Volume = 1.0f;
	NowVolume = 1.0f;
	VolumeRate = 0.0f;
	SampleCount = buffer_size / 2;
	// Create a waveform data storage buffer
	WaveData = (WAVEHDR*)malloc( sizeof( WAVEHDR ) * count );
	WaveDataCount = count;
	ZeroMemory( WaveData, sizeof( WAVEHDR ) * count );
	for( i = 0; i < count; i ++ )
	{
		WaveData[ i ].lpData = (LPSTR)malloc( buffer_size );
		ZeroMemory( WaveData[ i ].lpData, buffer_size );
		WaveData[ i ].dwBufferLength = buffer_size;
	}
	// Formatting
	ZeroMemory( &Format, sizeof( WAVEFORMATEX ) );
	Format.wFormatTag = WAVE_FORMAT_PCM;
	Format.nChannels = 2;
	Format.nSamplesPerSec = 44100;
	Format.wBitsPerSample = 16;
	Format.nBlockAlign = Format.nChannels * Format.wBitsPerSample / 8;
	Format.nAvgBytesPerSec = Format.nSamplesPerSec * Format.nBlockAlign;
	// Start the callback thread
	ThreadEnd = false;
	ThreadLock = CreateMutex( NULL, FALSE, NULL );
	ThreadHandle = CreateThread( NULL, 0, ThreadProc, this, 0, &ThreadID );
	if ( NULL == ThreadHandle )
	{
		Release();
		return 1;
	}
	SetThreadPriority( ThreadHandle, THREAD_PRIORITY_ABOVE_NORMAL);
	// Playback
	if( waveOutOpen( &Handle, WAVE_MAPPER, &Format, (DWORD_PTR)ThreadID, (DWORD_PTR)this, CALLBACK_THREAD ) != MMSYSERR_NOERROR )
	{
		Release();
		return 2;
	}
	return 0;
}

void WaveOutClass::Release( void )
{
	int i;
	// Stop playing
	Stop();
	// WaveOut End
	if( Handle )
	{
		while( MMSYSERR_HANDLEBUSY == waveOutReset( Handle ) )
		{
			Sleep( 1 );
		}
		for( i = 0; i < WaveDataCount; i ++ )
		{
			if( WaveData[ i ].dwFlags & WHDR_PREPARED )
			{
				waveOutUnprepareHeader( Handle, &WaveData[ i ], sizeof( WAVEHDR ) );
			}
		}
		while( MMSYSERR_HANDLEBUSY == waveOutClose( Handle ) )
		{
			Sleep( 1 );
		}
		Handle = NULL;
	}
	for( i = 0; i < WaveDataCount; i ++ )
	{
		if( WaveData[ i ].lpData )
		{
			free( WaveData[ i ].lpData );
			WaveData[ i ].lpData = NULL;
		}
	}
	// Thread Termination
	if ( ThreadHandle )
	{
		int retry = 2000; // If it doesn't finish within 2 seconds, it will be terminated.
		DWORD exit_code = -1;
		ThreadEnd = true;
		while( ThreadEnd )
		{
			Sleep( 1 );
			if( GetExitCodeThread( ThreadHandle, &exit_code ) != 0 )
			{
				if( exit_code != STILL_ACTIVE )
				{
					ThreadEnd = false;
				}
			}
			retry --;
			if( retry < 0 )
			{
				// Forced Termination
				TerminateThread( ThreadHandle, 0 );
				Sleep( 100 );
				ThreadEnd = false;
			}
		}
		CloseHandle( ThreadLock );
		CloseHandle( ThreadHandle );
		ThreadHandle = NULL;
	}
}

int WaveOutClass::Play( char *path, unsigned int mode )
{
	Stop();
	WaitForSingleObject( ThreadLock, INFINITE );
	PlayMode = mode;
	PlayBlock = 0;
	if ( OpenWave( path ) != 0 )
	{
		ReleaseMutex( ThreadLock );
		return 1;
	}
	int i;
	for( i = 0; i < WaveDataCount; i ++ )
	{
		ReadWave( (char*)WaveData[ i ].lpData, WaveData[ i ].dwBufferLength, mode );
	}
	for( i = 0; i < WaveDataCount; i ++ )
	{
		if( waveOutPrepareHeader( Handle, &WaveData[ i ], sizeof( WAVEHDR ) ) )
		{
			ReleaseMutex( ThreadLock );
			return 2;
		}
		if( waveOutWrite( Handle, &WaveData[ i ], sizeof( WAVEHDR ) ) )
		{
			waveOutUnprepareHeader( Handle, &WaveData[ i ], sizeof( WAVEHDR ) );
			ReleaseMutex( ThreadLock );
			return 3;
		}
	}
	Status = 1;
	ReleaseMutex( ThreadLock );
	return 0;
}

// In  : pause = (0 = unpause, 1 = pause)
void WaveOutClass::Pause( int pause )
{
	if( 1 == pause )
	{
		waveOutPause( Handle );
	}
	else
	{
		waveOutRestart( Handle );
	}
}

void WaveOutClass::Stop( void )
{
	WaitForSingleObject( ThreadLock, INFINITE );
	while( MMSYSERR_HANDLEBUSY == waveOutReset( Handle ) )
	{
		Sleep( 1 );
	}
	if( MmioHandle )
	{
		CloseWave();
	}
	Status = 0;
	ReleaseMutex( ThreadLock );
}

// In  : start = The start of the loop (in samples)
//     : end = not implemented
void WaveOutClass::SetLoop( int start, int end )
{
	LoopStart = (LONG)( start * 4 );
	LoopEnd = (LONG)( end * 4 );
}

// Out : (0 = not playing, 1 = playing)
int WaveOutClass::GetStatus( void )
{
	return Status;
}

int WaveOutClass::GetPosition( void )
{
	MMTIME mmtime;
	ZeroMemory( &mmtime, sizeof( mmtime ) );
	mmtime.wType = TIME_SAMPLES;
	if ( waveOutGetPosition( Handle, &mmtime, sizeof( mmtime ) ) != MMSYSERR_NOERROR )
	{
		return -1;
	}
	return (int)mmtime.u.sample;
}

// In  : volume = 0.0f ~ 1.0f
//     : time = number of samples (44100 = 1 second)
void WaveOutClass::SetVolume( float volume, int time )
{
	if( 0 == time )
	{
		VolumeRate = 0.0f;
		Volume = volume;
		NowVolume = volume;
	}
	else
	{
		VolumeRate = ( volume - Volume ) / (float)time;
		Volume = volume;
		if( 0.0f == VolumeRate )
		{
			Volume = volume;
			NowVolume = volume;
		}
	}
}

// Out : volume = 0.0f ~ 1.0f
float WaveOutClass::GetVolume( void )
{
	return Volume;
}

void WaveOutClass::AddPlayData( void )
{
	if( 0 == Status )
	{
		return;
	}
	if( waveOutUnprepareHeader( Handle, &WaveData[ PlayBlock ], sizeof( WAVEHDR ) ) )
	{
		return;
	}
	// Data Loading
	ReadWave( (char*)WaveData[ PlayBlock ].lpData, WaveData[ PlayBlock ].dwBufferLength, PlayMode );
	// Volume adjustment
	short *wave_data;
	int i;
    wave_data = (short*)WaveData[ PlayBlock ].lpData;
	if( 0 == VolumeRate )
	{
		if( Volume != 1.0f )
		{
			for( i = 0; i < SampleCount; i ++ )
			{
				wave_data[ i ] = (short)( (float)wave_data[ i ] * Volume );
			}
		}
	}
	else
	{
		for( i = 0; i < SampleCount; i ++ )
		{
			wave_data[ i ] = (short)( (float)wave_data[ i ] * NowVolume );
			if( i & 1 )
			{
				NowVolume += VolumeRate;
				if( VolumeRate < 0 )
				{
					if( NowVolume < 0 )
					{
						NowVolume = 0.0f;
						VolumeRate = 0.0f;
					}
				}
				else
				{
					if( NowVolume > 1.0f )
					{
						NowVolume = 1.0f;
						VolumeRate = 0.0f;
					}
				}
			}
		}
	}
	// Playback
	if( waveOutPrepareHeader( Handle, &WaveData[ PlayBlock ], sizeof( WAVEHDR ) ) )
	{
		Status = 0;
		return;
	}
	if( waveOutWrite( Handle, &WaveData[ PlayBlock ], sizeof( WAVEHDR ) ) )
	{
		waveOutUnprepareHeader( Handle, &WaveData[ PlayBlock ], sizeof( WAVEHDR ) );
		Status = 0;
		return;
	}
	// Playback block update
	PlayBlock ++;
	PlayBlock %= WaveDataCount;
}

int WaveOutClass::OpenWave( char *path )
{
	MMCKINFO mmck_infomain;
	MMCKINFO mmck_infosub;
	MMRESULT result;
	BYTE *pbyBuffer = NULL;
	MmioHandle = mmioOpen( (char*)path, NULL, MMIO_READ );
	if( NULL == MmioHandle )
	{
		return 1;
	}
	mmck_infomain.fccType = mmioFOURCC( 'W', 'A', 'V', 'E' );
	result = mmioDescend( MmioHandle, &mmck_infomain, NULL, MMIO_FINDRIFF );
	if( MMIOERR_CHUNKNOTFOUND == result )
	{
		// 'WAVE' chunk not found
		mmioClose( MmioHandle, 0 );
		return 2;
	}
	mmck_infosub.ckid = mmioFOURCC( 'f', 'm', 't', ' ' );
	result = mmioDescend( MmioHandle, &mmck_infosub, &mmck_infomain, MMIO_FINDCHUNK );
	if( MMIOERR_CHUNKNOTFOUND == result )
	{
		// 'fmt ' chunk not found
		mmioClose( MmioHandle, 0 );
		return 3;
	}
	DWORD chunk_size;
	WAVEFORMATEX format;
	chunk_size = mmioRead( MmioHandle, (HPSTR)&format, mmck_infosub.cksize );
	if( chunk_size != mmck_infosub.cksize )
	{
		// Unable to read 'fmt ' chunk data
		mmioClose( MmioHandle, 0 );
		return 4;
	}
	if( format.wFormatTag != Format.wFormatTag )
	{
		// Not in the specified format
		mmioClose( MmioHandle, 0 );
		return 5;
	}
	if( format.nChannels != Format.nChannels )
	{
		// Not in the specified format
		mmioClose( MmioHandle, 0 );
		return 5;
	}
	if( format.nSamplesPerSec != Format.nSamplesPerSec )
	{
		// Not in the specified format
		mmioClose( MmioHandle, 0 );
		return 5;
	}
	if( format.wBitsPerSample != Format.wBitsPerSample )
	{
		// Not in the specified format
		mmioClose( MmioHandle, 0 );
		return 5;
	}
	// Go back one chunk
	mmioAscend( MmioHandle, &mmck_infosub, 0 );
	// 'data' chunk scanning
	mmck_infosub.ckid = mmioFOURCC( 'd', 'a', 't', 'a' );
	result = mmioDescend( MmioHandle, &mmck_infosub, &mmck_infomain, MMIO_FINDCHUNK );
	if( MMIOERR_CHUNKNOTFOUND == result )
	{
		// 'data' chunk not found
		mmioClose( MmioHandle, 0 );
		return 6;
	}
	// Save current location
	WaveStart = mmioSeek( MmioHandle, 0L, SEEK_CUR );
	// Save the end of the file
	WaveEnd = mmioSeek( MmioHandle, 0L, SEEK_END );
	// Calculate the data size from the above two
	WaveSize = WaveEnd - WaveStart;
	WaveSampleSize = WaveSize / 4;
	// Return the seek position to the original location
	result = mmioSeek( MmioHandle, (LONG)WaveStart, SEEK_SET );
	return 0;
}

int WaveOutClass::ReadWave( char *buffer, int size, unsigned int mode )
{
	if( NULL == MmioHandle )
	{
		return 0;
	}
	int rest;
	int result;
	rest = size;
	while( rest > 0 )
	{
		ZeroMemory( buffer, rest );
		result = (int)mmioRead( MmioHandle, (HPSTR)buffer, rest );
		if ( rest != result )
		{
			if( PlayMode & MODE_LOOP )
			{
				mmioSeek( MmioHandle, WaveStart + LoopStart, SEEK_SET );
				buffer += result;
			}
			else
			{
				break;
			}
		}
		rest -= result;
	}
	return size - rest;
}

void WaveOutClass::CloseWave( void )
{
	if( NULL == MmioHandle )
	{
		return;
	}
	mmioClose( MmioHandle, 0 );
	MmioHandle = NULL;
}

DWORD WINAPI WaveOutClass::ThreadProc( LPVOID Thread )
{
	WaveOutClass *waveout_class;
	waveout_class = (WaveOutClass*)Thread;
	MSG msg;
	while( 1 )
	{
		Sleep( 1 );
		WaitForSingleObject( waveout_class->ThreadLock, INFINITE );
		if( waveout_class->ThreadEnd )
		{
			ReleaseMutex( waveout_class->ThreadLock );
			break;
		}
		if( !( waveout_class->PlayMode & MODE_LOOP ) )
		{
			if ( waveout_class->GetPosition() > waveout_class->WaveSampleSize )
			{
				waveout_class->Status = 0;
			}
		}
		if( waveout_class->Status != 0 )
		{
			if( PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) )
			{
				if( FALSE == GetMessage( &msg, NULL, 0, 0 ) )
				{
					ReleaseMutex( waveout_class->ThreadLock );
					break;
				}
				switch( msg.message )
				{
					case MM_WOM_DONE:
					{
						waveout_class->AddPlayData();
						break;
					}
				}
			}
		}
		ReleaseMutex( waveout_class->ThreadLock );
	}
	ExitThread( 0 );
}
