/*
 * ByteIO.cpp
 *
 *  Created on: April 1, 2016
 *      Author: Yongjiang Liang
 */

#include "ByteIO.h"

ByteIO::ByteIO() {
	// TODO Auto-generated constructor stub

}

ByteIO::~ByteIO() {
	// TODO Auto-generated destructor stub
}

ByteIO::ByteIO(byte* pbyBuffer, uint32 uAvailable)
{
	m_bReadOnly = false;
	m_pbyBuffer = pbyBuffer;
	m_uAvailable = uAvailable;
	m_uUsed = 0;
}

ByteIO::ByteIO(const byte* pbyBuffer, uint32 uAvailable)
{
	m_bReadOnly = true;
	m_pbyBuffer = const_cast<byte*>(pbyBuffer);
	m_uAvailable = uAvailable;
	m_uUsed = 0;
}

unsigned int ByteIO::GetAvailable() const
{
	return m_uAvailable;
}

void ByteIO::ReadArray(void * lpResult, uint32 uByteCount)
{
	if (m_uAvailable < uByteCount)
	{
		fprintf(stdout, "ByteIO::ReadArray() error!\n");
		return;
	}

	memcpy(lpResult, m_pbyBuffer, uByteCount);
	m_pbyBuffer += uByteCount;
	m_uUsed += uByteCount;
	m_uAvailable -= uByteCount;
}

void ByteIO::WriteArray(const void * lpVal, uint32 uByteCount)
{
	if (m_bReadOnly)
	{
		fprintf(stdout, "ByteIO::WriteArray() error!\n");
		return;
	}

	if (m_uAvailable < uByteCount)
	{
		fprintf(stdout, "ByteIO::WriteArray(), Buffer_too_small error!\n");
		return;
	}

	memcpy(m_pbyBuffer, lpVal, uByteCount);

	m_pbyBuffer += uByteCount;
	m_uUsed += uByteCount;
	m_uAvailable -= uByteCount;
}

void ByteIO::Reset()
{
	m_uAvailable += m_uUsed;
	m_pbyBuffer -= m_uUsed;
	m_uUsed = 0;
}
