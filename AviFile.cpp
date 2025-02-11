// $Id: AviFile.cpp,v 1.1 2008/03/29 03:13:22 samn Exp $ 
// AviFile.cpp: implementation of the CAviFile class.
// darrennix@msn.com
// http://homepages.msn.com/RedmondAve/darrennix
// See also: www.egerter.com www.gamedev.net
//////////////////////////////////////////////////////////////////////

// The usual disclaimers apply to all source code you receive
// from me. You can read my copyright/disclaimer at my website.

#include "stdafx.h"
#include <Windows.h>
#include "CAviFile.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CAviFile::CAviFile()
{
	AVIFileInit();
	Init();
}

CAviFile::~CAviFile()
{
	AVIFileExit();
	Release();
}

void CAviFile::Init()
{
	m_pAviFile = NULL;
	m_nNumAudioStreams = 0;
	m_nNumVideoStreams = 0;
	m_bIsOpen = false;
	m_iCurrentStream = 0;

	int n;
	
	for(n = 0; n < MAX_AUDIO_STREAMS; n++)
	{
		m_pAudioStreams[n] = NULL;
		m_pAudioFormats[n] = NULL;
		m_pAudioData[n] = NULL;
	}

	for(n = 0; n < MAX_VIDEO_STREAMS; n++)
	{
		m_pVideoStreams[n] = NULL;
		m_pVideoFormats[n] = NULL;
		m_pVideoPGF[n] = NULL;
		m_lVideoEndTime[n] = 0;
		m_iCurrentFrame[n] = 0;
		m_iFirstFrame[n] = 0;
		m_iLastFrame[n] = 0;
	}
}

//
// The Release function must return the object's state to
// the way it was when it was initially allocated. The
// object must be ready to be reused.
//
void CAviFile::Release()
{
	if(m_pAviFile)
		AVIFileRelease(m_pAviFile);
	
	int n;
	for(n = 0; n < m_nNumAudioStreams; n++)
	{
		if(m_pAudioStreams[n])
			AVIStreamRelease(m_pAudioStreams[n]);

		if(m_pAudioFormats[n])
			delete [] ((LPBYTE)m_pAudioFormats[n]);

		if(m_pAudioData[n])
			delete [] m_pAudioData[n];
	}

	for(n = 0; n < m_nNumVideoStreams; n++)
	{
		if(m_pVideoStreams[n])
			AVIStreamRelease(m_pVideoStreams[n]);

		if(m_pVideoFormats[n])
			delete [] ((LPBYTE)m_pVideoFormats[n]);
	}

	Init();
}

bool CAviFile::Open(LPCSTR lpszFileName)
{
	m_bIsOpen = false;
	
	if(AVIFileOpen(&m_pAviFile, lpszFileName, OF_READ, NULL))
		return false;

	FindStreams();
	
	if(!DetermineAudioFormats() ||
		!DetermineVideoFormats())
	{
		Release();
		return false;
	}

	// Okay, we know what's in the file.

	// Video and audio will be loaded as it's needed,
	// so there isn't a huge delay or impact on memory
	// when loading a large video file.

	m_bIsOpen = true;

	// Yippee!
	return true;
}

void CAviFile::FindStreams()
{
	do
	{
		if(AVIFileGetStream(m_pAviFile, &m_pAudioStreams[m_nNumAudioStreams],
			streamtypeAUDIO, m_nNumAudioStreams))
			break;
	}
	while(++m_nNumAudioStreams < MAX_AUDIO_STREAMS);
	
	do
	{
		if(AVIFileGetStream(m_pAviFile, &m_pVideoStreams[m_nNumVideoStreams],
			streamtypeVIDEO, m_nNumVideoStreams))
			break;
	}
	while(++m_nNumVideoStreams < MAX_VIDEO_STREAMS);
}

bool CAviFile::DetermineAudioFormats()
{
	for(int n = 0; n < m_nNumAudioStreams; n++)
	{
		PAVISTREAM pStream = m_pAudioStreams[n];

		LONG lSize;
		if(AVIStreamReadFormat(pStream, AVIStreamStart(pStream), NULL, &lSize))
			return false;

		LPBYTE pChunk = new BYTE[lSize];
		if(!pChunk)
			return false;

		if(AVIStreamReadFormat(pStream, AVIStreamStart(pStream), pChunk, &lSize))
			return false;

		m_pAudioFormats[n] = (LPWAVEFORMAT)pChunk;
	}

	// Yay!
	return true;
}

bool CAviFile::DetermineVideoFormats()
{
	for(int n = 0; n < m_nNumVideoStreams; n++)
	{
		PAVISTREAM pStream = m_pVideoStreams[n];

		LONG lSize;
		if(AVIStreamReadFormat(pStream, AVIStreamStart(pStream), NULL, &lSize))
			return false;

		LPBYTE pChunk = new BYTE[lSize];
		if(!pChunk)
			return false;

		if(AVIStreamReadFormat(pStream, AVIStreamStart(pStream), pChunk, &lSize))
			return false;

		m_pVideoFormats[n] = (LPBITMAPINFO)pChunk;
	}

	// Bravo!
	return true;
}

int CAviFile::GetAudioStreamCount()
{
	return m_nNumAudioStreams;
}

int CAviFile::GetVideoStreamCount()
{
	return m_nNumVideoStreams;
}

LPBYTE CAviFile::ExtractAudioStream(UINT nStreamNum)
{
	if(m_pAudioData[nStreamNum])
		return m_pAudioData[nStreamNum];
	
	if(nStreamNum >= m_nNumAudioStreams)
		return NULL;

	PAVISTREAM pStream = m_pAudioStreams[nStreamNum];

	LONG lSize;
	if(AVIStreamRead(pStream, 0, AVISTREAMREAD_CONVENIENT, NULL, 0, &lSize, NULL))
		return NULL;

	LPBYTE pBuffer = new BYTE[lSize];
	if(!pBuffer)
		return NULL;

	if(AVIStreamRead(pStream, 0, AVISTREAMREAD_CONVENIENT, pBuffer, lSize, NULL, NULL))
		return NULL;

	m_pAudioData[nStreamNum] = pBuffer;

	return pBuffer;
}

bool CAviFile::StartVideoRetrieve(UINT nStreamNum)
{
	if(nStreamNum >= m_nNumVideoStreams)
		return false;

	PAVISTREAM pStream = m_pVideoStreams[nStreamNum];

	PGETFRAME &pgf = m_pVideoPGF[nStreamNum];
	//LPBITMAPINFOHEADER lpDesiredFormat = &m_pVideoFormats[nStreamNum]->bmiHeader;
	
	pgf = AVIStreamGetFrameOpen(pStream, NULL/*lpDesiredFormat*/);

	if(!pgf)
		return false;

	m_lVideoEndTime[nStreamNum] = AVIStreamEndTime(pStream);

	m_iCurrentStream = nStreamNum;
	m_iLastFrame[nStreamNum]=AVIStreamLength(pStream);
	m_iCurrentFrame[nStreamNum]=m_iFirstFrame[nStreamNum]=AVIStreamStart(pStream);

	// Yahoo!
	return true;
}

bool CAviFile::EndVideoRetrieve(UINT nStreamNum)
{
	PGETFRAME &pgf = m_pVideoPGF[nStreamNum];

	if(AVIStreamGetFrameClose(pgf))
		return false;

	pgf = NULL;

	m_iCurrentStream = 0;

	// Hurray!
	return true;
}

void CAviFile::GetVideoFrameAtTime(UINT nStreamNum, LONG lTimeInMilliSec, LPBITMAPINFOHEADER *ppbi)
{
	if(nStreamNum >= m_nNumVideoStreams)
	{
		*ppbi = NULL;
		return;
	}

	PAVISTREAM pStream = m_pVideoStreams[nStreamNum];
	PGETFRAME &pgf = m_pVideoPGF[nStreamNum];

	LONG lFrame;
	if(lTimeInMilliSec <= m_lVideoEndTime[nStreamNum])
		lFrame = AVIStreamTimeToSample(pStream, lTimeInMilliSec);
	else
	{
		*ppbi = NULL; // video is done, no more frames
		return;
	}

	*ppbi = (LPBITMAPINFOHEADER)AVIStreamGetFrame(pgf, lFrame);
}

void CAviFile::GetVideoFrame(UINT nStreamNum, LONG lFrame, LPBITMAPINFOHEADER *ppbi)
{
	PGETFRAME &pgf = m_pVideoPGF[nStreamNum];
	*ppbi = (LPBITMAPINFOHEADER)AVIStreamGetFrame(pgf, lFrame);
	// NULL on error or no more frames
}

LPWAVEFORMAT CAviFile::GetAudioFormat(UINT nStreamNum)
{
	if(nStreamNum >= m_nNumAudioStreams)
		return NULL;

	return m_pAudioFormats[nStreamNum];
}

LPBITMAPINFO CAviFile::GetVideoFormat(UINT nStreamNum)
{
	if(nStreamNum >= m_nNumVideoStreams)
		return NULL;

	return m_pVideoFormats[nStreamNum];
}
