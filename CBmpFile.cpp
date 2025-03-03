// $Id: CBmpFile.cpp,v 1.1 2008/03/29 03:13:32 samn Exp $ 
#include "stdafx.h"

#include "CBmpFile.h"

CBmpFile::CBmpFile()
{
	Init();
}

CBmpFile::CBmpFile(LPSTR szFilename)
{
	m_nIsValid = Load(szFilename) ? 1 : 0;
}

CBmpFile::~CBmpFile()
{
	Destroy();
}

/*bool CBmpFile::Load(BITMAPINFOHEADER* p)
{
	Destroy();

	m_bmInfoHeader = p;
    m_bmFileHeader.bfType=19778;    //BM header
    m_bmFileHeader.bfSize=55 + m_bmInfoHeader->biSizeImage;
    m_bmFileHeader.bfReserved1=0;
    m_bmFileHeader.bfReserved2=0;
    m_bmFileHeader.bfOffBits=sizeof(BITMAPINFOHEADER) + sizeof(BITMAPFILEHEADER); //54
}*/

bool CBmpFile::Load(LPSTR szFilename)
{
	HANDLE hFile = CreateFile(szFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL);
	if(!hFile) return false;
	Load(hFile);
	CloseHandle(hFile);
	return IsValid();
}

bool CBmpFile::Load(HANDLE hFile)
{
	Destroy();

	DWORD dwBytesRead;
	int bSuccess;

	DWORD &biSizeImage = m_bmInfoHeader.biSizeImage;

	// File header
	bSuccess = ReadFile(hFile, (LPVOID)&m_bmFileHeader, sizeof(BITMAPFILEHEADER), &dwBytesRead, NULL);
	if(!(bSuccess && m_bmFileHeader.bfType==MAKEWORD('B','M')))
	{ ASSERT(false); return false; }

	// Bitmap Info Header
	bSuccess = ReadFile(hFile, (LPVOID)&m_bmInfoHeader, sizeof(BITMAPINFOHEADER), &dwBytesRead, NULL);
	if(!(bSuccess && m_bmInfoHeader.biBitCount==24 && m_bmInfoHeader.biCompression==BI_RGB))
	{ ASSERT(false); return false; }

	// This member may be zero since biCompression == BI_RGB
	if(!m_bmInfoHeader.biSizeImage)
		m_bmInfoHeader.biSizeImage = m_bmInfoHeader.biHeight * m_bmInfoHeader.biWidth * m_bmInfoHeader.biBitCount/8;

	// Image Data
	m_pRGB = (LPRGB)new BYTE[biSizeImage];
	if(!m_pRGB) { ASSERT(false); return false; }
	bSuccess = ReadFile(hFile, (LPVOID)m_pRGB, biSizeImage, &dwBytesRead, NULL);
	if(!bSuccess) { ASSERT(false); return false; }

	m_nNumPixels = biSizeImage / 3;

	m_nIsValid = 1;
	return IsValid();
}

bool CBmpFile::Save(LPSTR szFilename)
{
	HANDLE hFile = CreateFile(szFilename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE, NULL);
	if(!hFile) { ASSERT(FALSE); return false; }
	Save(hFile);
	CloseHandle(hFile);
	return IsValid();
}

bool CBmpFile::Save(HANDLE hFile)
{
	DWORD dwBytesWrit;
	int bSuccess;

	DWORD &biSizeImage = m_bmInfoHeader.biSizeImage;

	// File header
	bSuccess = WriteFile(hFile, (LPVOID)&m_bmFileHeader, sizeof(BITMAPFILEHEADER), &dwBytesWrit, NULL);
	if(!(bSuccess && m_bmFileHeader.bfType==MAKEWORD('B','M')))
	{ ASSERT(false); return false; }

	// This member may be zero since biCompression == BI_RGB
	if(!m_bmInfoHeader.biSizeImage)
		m_bmInfoHeader.biSizeImage = m_bmInfoHeader.biHeight * m_bmInfoHeader.biWidth * m_bmInfoHeader.biBitCount/8;

	// Bitmap Info Header
	bSuccess = WriteFile(hFile, (LPVOID)&m_bmInfoHeader, sizeof(BITMAPINFOHEADER), &dwBytesWrit, NULL);
	if(!(bSuccess && m_bmInfoHeader.biBitCount==24 && m_bmInfoHeader.biCompression==BI_RGB))
	{ ASSERT(false); return false; }

	// Image Data
	if(!m_pRGB) { ASSERT(false); return false; }
	bSuccess = WriteFile(hFile, (LPVOID)m_pRGB, biSizeImage, &dwBytesWrit, NULL);
	if(!bSuccess) { ASSERT(false); return false; }

	m_nIsValid = 1;
	return IsValid();
}
	
void CBmpFile::Init()
{
	m_nIsValid = 0;
	memset(&m_bmFileHeader, 0, sizeof(BITMAPFILEHEADER));
	memset(&m_bmInfoHeader, 0, sizeof(BITMAPINFOHEADER));
	m_pRGB = NULL;
	m_nNumPixels = 0;
}

void CBmpFile::Destroy()
{
	if(m_pRGB) delete [] m_pRGB;
	Init();
}

/* Set pixels with components < rgbThreshold to rgbSetTo */
BOOL CBmpFile::BlueScreen(PIXEL rgbSetTo, PIXEL rgbThreshold)
{
	if(!IsValid()) return FALSE;

	for(int n = 0; n < m_nNumPixels; n++)
	{
		PIXEL &rgb = m_pRGB[n];
		if(rgb.red < rgbThreshold.red &&
			rgb.green < rgbThreshold.green &&
			rgb.blue < rgbThreshold.blue)
		{
			rgb.red = rgbSetTo.red;
			rgb.green = rgbSetTo.green;
			rgb.blue = rgbSetTo.blue;
		}
	}
	
	return TRUE;
}

/* Set pixels at rgbBkgColor to 255,255,255 and set
   all others to 0,0,0 */
BOOL CBmpFile::Maskify(PIXEL rgbBkgColor)
{
	if(!IsValid()) return FALSE;

	for(int n = 0; n < m_nNumPixels; n++)
	{
		PIXEL &rgb = m_pRGB[n];
		if(rgb.red == rgbBkgColor.red &&
			rgb.green == rgbBkgColor.green &&
			rgb.blue == rgbBkgColor.blue)
		{
			rgb.red = 255;
			rgb.green = 255;
			rgb.blue = 255;
		}
		else
		{
			rgb.red = 0;
			rgb.green = 0;
			rgb.blue = 0;
		}
	}

	return TRUE;
}

HBITMAP CBmpFile::CreateHandle(HDC hDC)
{
	LPBYTE pBits;
	HBITMAP hBitmap =
		CreateDIBSection(hDC, (LPBITMAPINFO)&m_bmInfoHeader,
		DIB_RGB_COLORS, (LPVOID*)&pBits, NULL, 0);

	if(!hBitmap) return NULL;

	memcpy(pBits, m_pRGB, m_bmInfoHeader.biSizeImage);

	return hBitmap;
}


int CBmpFile::Width()
{
	return m_bmInfoHeader.biWidth;
}

int CBmpFile::Height()
{
	return m_bmInfoHeader.biHeight;
}

int CBmpFile::GetBpp()
{
	return m_bmInfoHeader.biBitCount;
}

int CBmpFile::GetFileSize()
{
	// this is just a guess

	return m_bmFileHeader.bfSize + m_bmInfoHeader.biSize +
		m_nNumPixels * m_bmInfoHeader.biBitCount/8 +
		((m_bmInfoHeader.biBitCount==8)?(256*3):(0));
}

LPRGB CBmpFile::GetRGB()
{
	return m_pRGB;
}

BITMAPINFO* CBmpFile::GetBitmapInfo()
{
	return (BITMAPINFO*)&m_bmInfoHeader;
}
