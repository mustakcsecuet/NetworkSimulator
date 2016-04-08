/*
 * DataIO.cpp
 *
 *  Created on: April 1, 2016
 *      Author: Yongjiang Liang
 */

#include "DataIO.h"

DataIO::DataIO() {
	// TODO Auto-generated constructor stub

}

DataIO::~DataIO() {
	// TODO Auto-generated destructor stub
}

byte DataIO::ReadByte()
{
	byte byRetVal;
	ReadArray(&byRetVal, 1);
	return byRetVal;
}

uint8 DataIO::ReadUInt8()
{
	uint8 uRetVal;
	ReadArray(&uRetVal, sizeof(uint8));
	return uRetVal;
}

uint16 DataIO::ReadUInt16()
{
	uint16 uRetVal;
	ReadArray(&uRetVal, sizeof(uint16));
	return uRetVal;
}

uint32 DataIO::ReadUInt32()
{
	uint32 uRetVal;
	ReadArray(&uRetVal, sizeof(uint32));
	return uRetVal;
}

uint64 DataIO::ReadUInt64()
{
	uint64 uRetVal;
	ReadArray(&uRetVal, sizeof(uint64));
	return uRetVal;
}

void DataIO::WriteByte(byte byVal)
{
	WriteArray(&byVal, 1);
}

void DataIO::WriteUInt8(uint8 uVal)
{
	WriteArray(&uVal, sizeof(uint8));
}

void DataIO::WriteUInt16(uint16 uVal)
{
	WriteArray(&uVal, sizeof(uint16));
}

void DataIO::WriteUInt32(uint32 uVal)
{
	WriteArray(&uVal, sizeof(uint32));
}

void DataIO::WriteUInt64(uint64 uVal)
{
	WriteArray(&uVal, sizeof(uint64));
}

