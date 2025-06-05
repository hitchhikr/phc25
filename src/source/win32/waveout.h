//============================================================================
//  waveout.h "Stream playback control using waveOut API"
//													Programmed by Keiichi Tago
//----------------------------------------------------------------------------
// 2004.02.10 Ver.1.00 Something that works has been completed.
// 2004.02.09 Start creating.
//============================================================================
#ifndef WAVEOUT_INCLUDE
#define WAVEOUT_INCLUDE

class WaveOutClass
{
	public:
		enum
		{
			MODE_LOOP = 0x00000001,
		};
		WaveOutClass( void );
		~WaveOutClass( void );
		int Init( int buffer_size, int count );
		void Release( void );
		int Play( char *path, unsigned int mode = 0 );
		void Pause( int pause );
		void Stop( void );
		void SetLoop( int start, int end = -1 );
		int GetStatus( void );
		int GetPosition( void );
		void SetVolume( float volume, int time = 0 );
		float GetVolume( void );
	private:
		HWAVEOUT Handle;
		WAVEHDR *WaveData;
		int WaveDataCount;
		WAVEFORMATEX Format;
		HANDLE ThreadHandle;
		DWORD ThreadID;
		bool ThreadEnd;
		HANDLE ThreadLock;
		unsigned int PlayMode;
		int PlayBlock;
		LONG LoopStart;
		LONG LoopEnd;
		int WaveStart;
		int WaveEnd;
		int WaveSize;
		int WaveSampleSize;
		int Status;
		float Volume;			// 0.0f ~ 1.0f
		float NowVolume;		// 0.0f ~ 1.0f
		float VolumeRate;
		int SampleCount;
		HMMIO MmioHandle;
		WaveOutClass( const WaveOutClass& waveout );
		WaveOutClass& operator = ( const WaveOutClass &waveout );
		void AddPlayData( void );
		int OpenWave( char *path );
		int ReadWave( char *buffer, int size, unsigned int mode );
		void CloseWave( void );
		static DWORD WINAPI ThreadProc( LPVOID Thread );
};

#endif
