#include "BinStream.hpp"

BinStream::BinStream() {
	this->data = nullptr;
	this->size = 0;
	this->position = 0;
}

BinStream::BinStream(const uint8_t* data, uint32_t length) {
	this->data = new uint8_t[length];
	memcpy(this->data, data, length);
	this->size = length;
	this->position = 0;
}

BinStream::BinStream(uint32_t length) {
	this->data = new uint8_t[length]();
	this->size = length;
	this->position = 0;
}

BinStream::BinStream(const char* data, uint32_t length) {
	this->data = new uint8_t[length];
	memcpy(this->data, data, length);
	this->size = length;
	this->position = 0;
}

BinStream::BinStream(std::string_view sv) {
	this->size = sv.size();
	this->data = new uint8_t[this->size];
	memcpy(this->data, sv.data(), this->size);
	this->position = 0;
}

BinStream::BinStream(const BinStream& other) {
	this->size = other.size;
	this->position = other.position;
	if (other.data) {
		this->data = new uint8_t[other.size];
		memcpy(this->data, other.data, other.size);
	} else {
		this->data = nullptr;
	}
}

BinStream::BinStream(BinStream&& other) noexcept {
	this->data = other.data;
	this->size = other.size;
	this->position = other.position;
	other.data = nullptr;
	other.size = 0;
	other.position = 0;
}

BinStream& BinStream::operator=(const BinStream& other) {
	if (this != &other) {
		delete[] this->data;
		this->size = other.size;
		this->position = other.position;
		if (other.data) {
			this->data = new uint8_t[other.size];
			memcpy(this->data, other.data, other.size);
		} else {
			this->data = nullptr;
		}
	}
	return *this;
}

BinStream& BinStream::operator=(BinStream&& other) noexcept {
	if (this != &other) {
		delete[] this->data;
		this->data = other.data;
		this->size = other.size;
		this->position = other.position;
		other.data = nullptr;
		other.size = 0;
		other.position = 0;
	}
	return *this;
}

BinStream::~BinStream() {
	delete[] this->data;
}

uint8_t* BinStream::getData() {
	return this->data;
}

uint32_t BinStream::getSize() const {
	return this->size;
}

uint32_t BinStream::getPosition() const {
	return this->position;
}

void BinStream::setData(const uint8_t* data, uint32_t length) {
	delete[] this->data;
	this->data = new uint8_t[length];
	memcpy(this->data, data, length);
	this->size = length;
	this->position = 0;
}

void BinStream::setSize(uint32_t length) {
	this->size = length;
}

void BinStream::setPosition(uint32_t position) {
	this->position = position;
}

uint32_t BinStream::readUVarInt() {
	uint32_t value = 0;
	int shift = 0;
	while (true) {
		if (this->position >= this->size) return 0;
		uint8_t byte = this->data[this->position++];
		value |= (uint32_t)(byte & 0x7F) << shift;
		if ((byte & 0x80) == 0) break;
		shift += 7;
		if (shift > 35) return 0;
	}
	return value;
}

int32_t BinStream::readVarInt() {
	uint32_t raw = readUVarInt();
	return (int32_t)((raw >> 1) ^ (~(raw & 1) + 1));
}

uint64_t BinStream::readUVarLong() {
	uint64_t value = 0;
	int shift = 0;
	while (true) {
		if (this->position >= this->size) return 0;
		uint8_t byte = this->data[this->position++];
		value |= (uint64_t)(byte & 0x7F) << shift;
		if ((byte & 0x80) == 0) break;
		shift += 7;
		if (shift > 70) return 0;
	}
	return value;
}

int64_t BinStream::readVarLong() {
	uint64_t raw = readUVarLong();
	return (int64_t)((raw >> 1) ^ (~(raw & 1) + 1));
}

void BinStream::writeUVarInt(uint32_t value) {
	while (true) {
		uint8_t byte = value & 0x7F;
		value >>= 7;
		if (value != 0) byte |= 0x80;
		this->writeUint8(byte);
		if (value == 0) break;
	}
}

void BinStream::writeUVarLong(uint64_t value) {
	while (true) {
		uint8_t byte = value & 0x7F;
		value >>= 7;
		if (value != 0) byte |= 0x80;
		this->writeUint8(byte);
		if (value == 0) break;
	}
}

void BinStream::writeUint8(uint8_t value) {
	if (this->position + 1 > this->size) return;
	this->data[this->position] = value;
	this->position += 1;
}

void BinStream::writeUint16(uint16_t value, Endian endian) {
	if (this->position + 2 > this->size) return;

	if (endian == Endian::Big) {
		this->data[this->position] = (value >> 8) & 0xFF;
		this->data[this->position + 1] = value & 0xFF;
	} else {
		this->data[this->position] = value & 0xFF;
		this->data[this->position + 1] = (value >> 8) & 0xFF;
	}

	this->position += 2;
}

void BinStream::writeUint24(uint24_t value, Endian endian) {
	if (this->position + 3 > this->size) return;

	if (endian == Endian::Big) {
		this->data[this->position] = (value >> 16) & 0xFF;
		this->data[this->position + 1] = (value >> 8) & 0xFF;
		this->data[this->position + 2] = value & 0xFF;
	} else {
		this->data[this->position] = value & 0xFF;
		this->data[this->position + 1] = (value >> 8) & 0xFF;
		this->data[this->position + 2] = (value >> 16) & 0xFF;
	}

	this->position += 3;
}

void BinStream::writeUint32(uint32_t value, Endian endian) {
	if (this->position + 4 > this->size) return;

	if (endian == Endian::Big) {
		this->data[this->position] = (value >> 24) & 0xFF;
		this->data[this->position + 1] = (value >> 16) & 0xFF;
		this->data[this->position + 2] = (value >> 8) & 0xFF;
		this->data[this->position + 3] = value & 0xFF;
	} else {
		this->data[this->position] = value & 0xFF;
		this->data[this->position + 1] = (value >> 8) & 0xFF;
		this->data[this->position + 2] = (value >> 16) & 0xFF;
		this->data[this->position + 3] = (value >> 24) & 0xFF;
	}

	this->position += 4;
}

void BinStream::writeUint64(uint64_t value, Endian endian) {
	if (this->position + 8 > this->size) return;

	if (endian == Endian::Big) {
		this->data[this->position] = (value >> 56) & 0xFF;
		this->data[this->position + 1] = (value >> 48) & 0xFF;
		this->data[this->position + 2] = (value >> 40) & 0xFF;
		this->data[this->position + 3] = (value >> 32) & 0xFF;
		this->data[this->position + 4] = (value >> 24) & 0xFF;
		this->data[this->position + 5] = (value >> 16) & 0xFF;
		this->data[this->position + 6] = (value >> 8) & 0xFF;
		this->data[this->position + 7] = value & 0xFF;
	} else {
		this->data[this->position] = value & 0xFF;
		this->data[this->position + 1] = (value >> 8) & 0xFF;
		this->data[this->position + 2] = (value >> 16) & 0xFF;
		this->data[this->position + 3] = (value >> 24) & 0xFF;
		this->data[this->position + 4] = (value >> 32) & 0xFF;
		this->data[this->position + 5] = (value >> 40) & 0xFF;
		this->data[this->position + 6] = (value >> 48) & 0xFF;
		this->data[this->position + 7] = (value >> 56) & 0xFF;
	}

	this->position += 8;
}

void BinStream::writeVarInt(int32_t value) {
	uint32_t zigzag = (value << 1) ^ (value >> 31);
	writeUVarInt(zigzag);
}

void BinStream::writeVarLong(int64_t value) {
	uint64_t zigzag = (value << 1) ^ (value >> 63);
	writeUVarLong(zigzag);
}

void BinStream::writeInt8(int8_t value) {
	this->writeUint8((uint8_t)value);
}

void BinStream::writeInt16(int16_t value, Endian endian) {
	this->writeUint16((uint16_t)value, endian);
}

void BinStream::writeInt24(int24_t value, Endian endian) {
	this->writeUint24((uint32_t)value, endian);
}

void BinStream::writeInt32(int32_t value, Endian endian) {
	this->writeUint32((uint32_t)value, endian);
}

void BinStream::writeInt64(int64_t value, Endian endian) {
	this->writeUint64((uint64_t)value, endian);
}

void BinStream::writeFloat(float value, Endian endian) {
	uint32_t bits;
	memcpy(&bits, &value, sizeof(bits));
	this->writeUint32(bits, endian);
}

void BinStream::writeDouble(double value, Endian endian) {
	uint64_t bits;
	memcpy(&bits, &value, sizeof(bits));
	this->writeUint64(bits, endian);
}

uint8_t BinStream::readUint8() {
	if (this->position + 1 > this->size) return 0;
	uint8_t value = this->data[this->position];
	this->position += 1;
	return value;
}

uint16_t BinStream::readUint16(Endian endian) {
	if (this->position + 2 > this->size) return 0;

	uint16_t value;
	if (endian == Endian::Big) {
		value = (this->data[this->position] << 8) | this->data[this->position + 1];
	} else {
		value = (this->data[this->position + 1] << 8) | this->data[this->position];
	}

	this->position += 2;
	return value;
}

uint24_t BinStream::readUint24(Endian endian) {
	if (this->position + 3 > this->size) return 0;

	uint32_t value;
	if (endian == Endian::Big) {
		value = (this->data[this->position] << 16) | (this->data[this->position + 1] << 8) | this->data[this->position + 2];
	} else {
		value = this->data[this->position] | (this->data[this->position + 1] << 8) | (this->data[this->position + 2] << 16);
	}

	this->position += 3;
	return value;
}

uint32_t BinStream::readUint32(Endian endian) {
	if (this->position + 4 > this->size) return 0;

	uint32_t value;
	if (endian == Endian::Big) {
		value = (this->data[this->position] << 24) | (this->data[this->position + 1] << 16) | (this->data[this->position + 2] << 8) | this->data[this->position + 3];
	} else {
		value = this->data[this->position] | (this->data[this->position + 1] << 8) | (this->data[this->position + 2] << 16) | (this->data[this->position + 3] << 24);
	}

	this->position += 4;
	return value;
}

uint64_t BinStream::readUint64(Endian endian) {
	if (this->position + 8 > this->size) return 0;

	uint64_t value;
	if (endian == Endian::Big) {
		value = ((uint64_t)this->data[this->position] << 56) | ((uint64_t)this->data[this->position + 1] << 48) | ((uint64_t)this->data[this->position + 2] << 40) | ((uint64_t)this->data[this->position + 3] << 32) | ((uint64_t)this->data[this->position + 4] << 24) | ((uint64_t)this->data[this->position + 5] << 16) | ((uint64_t)this->data[this->position + 6] << 8) | (uint64_t)this->data[this->position + 7];
	} else {
		value = (uint64_t)this->data[this->position] | ((uint64_t)this->data[this->position + 1] << 8) | ((uint64_t)this->data[this->position + 2] << 16) | ((uint64_t)this->data[this->position + 3] << 24) | ((uint64_t)this->data[this->position + 4] << 32) | ((uint64_t)this->data[this->position + 5] << 40) | ((uint64_t)this->data[this->position + 6] << 48) | ((uint64_t)this->data[this->position + 7] << 56);
	}

	this->position += 8;
	return value;
}

int8_t BinStream::readInt8() {
	return (int8_t)this->readUint8();
}

int16_t BinStream::readInt16(Endian endian) {
	return (int16_t)this->readUint16(endian);
}

int24_t BinStream::readInt24(Endian endian) {
	return (int32_t)this->readUint24(endian);
}

int32_t BinStream::readInt32(Endian endian) {
	return (int32_t)this->readUint32(endian);
}

int64_t BinStream::readInt64(Endian endian) {
	return (int64_t)this->readUint64(endian);
}

float BinStream::readFloat(Endian endian) {
	uint32_t bits = this->readUint32(endian);
	float value;
	memcpy(&value, &bits, sizeof(value));
	return value;
}

double BinStream::readDouble(Endian endian) {
	uint64_t bits = this->readUint64(endian);
	double value;
	memcpy(&value, &bits, sizeof(value));
	return value;
}

void BinStream::writeString(const std::string& value) {
	this->writeUVarInt(value.length());
	this->writeBytes((const uint8_t*)(value.c_str()), value.length());
}

void BinStream::writeString16(const std::string& value, Endian endian) {
	this->writeUint16(value.length(), endian);
	this->writeBytes((const uint8_t*)(value.c_str()), value.length());
}

std::string BinStream::readString() {
	uint32_t length = this->readUVarInt();
	uint8_t* bytes = this->readBytes(length);
	if (!bytes) return {};
	std::string value((char*)bytes, length);
	delete[] bytes;
	return value;
}

void BinStream::writeBytes(const uint8_t* value, uint32_t length) {
	if (this->position + length > this->size) return;
	memcpy(this->data + this->position, value, length);
	this->position += length;
}

uint8_t* BinStream::readBytes(uint32_t length) {
	if (this->position + length > this->size) return nullptr;

	uint8_t* value = new uint8_t[length];
	memcpy(value, this->data + this->position, length);
	this->position += length;
	return value;
}

void BinStream::writeBinStream(const BinStream& value, Endian endian) {
	this->writeUint32(value.size, endian);
	this->writeBytes(value.data, value.size);
}

BinStream BinStream::readBinStream(Endian endian) {
	uint32_t length = this->readUint32(endian);
	uint8_t* bytes = this->readBytes(length);
	if (!bytes) return {};
	BinStream value(bytes, length);
	delete[] bytes;
	return value;
}

void BinStream::reset() {
	this->position = 0;
}

void BinStream::clear() {
	delete[] this->data;
	this->data = nullptr;
	this->size = 0;
	this->position = 0;
}

std::string BinStream::print() {
	std::stringstream ss;
	ss << "Bin " << this->size << ": ";
	for (uint32_t i = 0; i < this->size; i++) {
		ss << std::hex << std::setw(2) << std::setfill('0') << std::uppercase << (int)this->data[i] << " ";
	}
	return ss.str();
}
