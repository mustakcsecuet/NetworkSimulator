/*
 * DataIO.h
 *
 *  Created on: April 1, 2016
 *      Author: Yongjiang Liang
 */

#ifndef DATAIO_H_
#define DATAIO_H_

#include "head.h"

class DataIO {
public:
	DataIO();
	virtual ~DataIO();

	byte ReadByte();
	uint8 ReadUInt8();
	uint16 ReadUInt16();
	uint32 ReadUInt32();
	uint64 ReadUInt64();

	void WriteByte(byte byVal);
	void WriteUInt8(uint8 uVal);
	void WriteUInt16(uint16 uVal);
	void WriteUInt32(uint32 uVal);
	void WriteUInt64(uint64 uVal);

	virtual void ReadArray(void * lpResult, uint32 uByteCount) = 0;
	virtual void WriteArray(const void * lpVal, uint32 uByteCount) = 0;
	virtual unsigned GetAvailable() const = 0;
};

#endif /* DATAIO_H_ */
