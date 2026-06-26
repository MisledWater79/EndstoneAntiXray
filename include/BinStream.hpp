#pragma once

#include <cstdint>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>

typedef uint32_t uint24_t;
typedef int32_t int24_t;

enum class Endian : uint8_t {
	Big,
	Little
};

class BinStream {
private:
	uint8_t* data;
	uint32_t size;
	uint32_t position;

public:

	BinStream();
	BinStream(const uint8_t* data, uint32_t length);
	BinStream(uint32_t length);
	BinStream(const char* data, uint32_t length);
	BinStream(std::string_view sv);

	BinStream(const BinStream& other);
	BinStream(BinStream&& other) noexcept;
	BinStream& operator=(const BinStream& other);
	BinStream& operator=(BinStream&& other) noexcept;

	~BinStream();

	uint8_t* getData();
	uint32_t getSize() const;
	uint32_t getPosition() const;

	void setData(const uint8_t* data, uint32_t length);
	void setSize(uint32_t length);
	void setPosition(uint32_t position);

	template <typename T>
	void write(T* value, Endian endian = Endian::Big) {
		value->write(this, endian);
	}
	void writeUVarInt(uint32_t value);
	void writeUVarLong(uint64_t value);
	void writeUint8(uint8_t value);
	void writeUint16(uint16_t value, Endian endian = Endian::Big);
	void writeUint24(uint24_t value, Endian endian = Endian::Big);
	void writeUint32(uint32_t value, Endian endian = Endian::Big);
	void writeUint64(uint64_t value, Endian endian = Endian::Big);

	void writeVarInt(int32_t value);
	void writeVarLong(int64_t value);
	void writeInt8(int8_t value);
	void writeInt16(int16_t value, Endian endian = Endian::Big);
	void writeInt24(int24_t value, Endian endian = Endian::Big);
	void writeInt32(int32_t value, Endian endian = Endian::Big);
	void writeInt64(int64_t value, Endian endian = Endian::Big);
	void writeFloat(float value, Endian endian = Endian::Big);
	void writeDouble(double value, Endian endian = Endian::Big);

	template <typename T>
	T* read(Endian endian = Endian::Big) {
		T* v = new T();
		v->read(this, endian);
		return v;
	}
	uint32_t readUVarInt();
	int32_t readVarInt();
	uint64_t readUVarLong();
	int64_t readVarLong();
	uint8_t readUint8();
	uint16_t readUint16(Endian endian = Endian::Big);
	uint24_t readUint24(Endian endian = Endian::Big);
	uint32_t readUint32(Endian endian = Endian::Big);
	uint64_t readUint64(Endian endian = Endian::Big);
	int8_t readInt8();
	int16_t readInt16(Endian endian = Endian::Big);
	int24_t readInt24(Endian endian = Endian::Big);
	int32_t readInt32(Endian endian = Endian::Big);
	int64_t readInt64(Endian endian = Endian::Big);
	float readFloat(Endian endian = Endian::Big);
	double readDouble(Endian endian = Endian::Big);

	void writeString(const std::string& value);
	void writeString16(const std::string& value, Endian endian = Endian::Big);
	std::string readString();

	void writeBytes(const uint8_t* value, uint32_t length);
	uint8_t* readBytes(uint32_t length);

	void writeBinStream(const BinStream& value, Endian endian = Endian::Big);
	BinStream readBinStream(Endian endian = Endian::Big);

	void reset();
	void clear();

	std::string print();
};
