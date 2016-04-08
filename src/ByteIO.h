/*
 * ByteIO.h
 *
 *  Created on: April 1, 2016
 *      Author: Yongjiang Liang
 */

#ifndef BYTEIO_H_
#define BYTEIO_H_

#include "head.h"
#include "DataIO.h"

class ByteIO : public DataIO {
public:
	ByteIO();
	virtual ~ByteIO();

	ByteIO(byte* pbyBuffer, uint32 uAvailable);
	ByteIO(const byte* pbyBuffer, uint32 uAvailable);

	void ReadArray(void* lpResult, uint32 uByteCount);
	void WriteArray(const void* lpVal, uint32 uByteCount);
	unsigned int GetAvailable() const;
	void Reset();
	byte* GetBuffer() const {
		return m_pbyBuffer;
	}
private:
	bool m_bReadOnly;
	byte* m_pbyBuffer;
	uint32 m_uAvailable;
	uint32 m_uUsed;
};

#endif /* BYTEIO_H_ */
