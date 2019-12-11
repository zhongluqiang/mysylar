#ifndef MYSYLAR_BYTEARRAY_H
#define MYSYLAR_BYTEARRAY_H

#include <memory>
#include <stdint.h>
#include <string>
#include <sys/uio.h>
#include <vector>

namespace sylar {

class ByteArray {
public:
    typedef std::shared_ptr<ByteArray> ptr;

    /* ByteArray的存储结点，链表结构，便于动态增长 */
    struct Node {
        Node(size_t s);
        Node();
        ~Node();

        char *ptr;
        Node *next;
        size_t size;
    };

    ByteArray(size_t base_size = 4096);
    ~ByteArray();

    //清空ByteArray
    void clear();

    //读写
    void write(const void *buf, size_t size);
    void read(void *buf, size_t size);
    void read(void *buf, size_t size, size_t position) const;

    //获取/设置ByteArray当前位置
    size_t getPosition() const { return m_position; }
    void setPosition(size_t v);

    //读写文件
    bool writeToFile(const std::string &name) const;
    bool readFromFile(const std::string &name);

    //返回内存块大小
    size_t getBaseSize() const { return m_baseSize; }

    //返回可读取数据大小
    size_t getReadSize() const { return m_size - m_position; }

    bool isLittleEndian() const;
    void setIsLittleEndian(bool val);

    std::string toString() const;
    std::string toHexString() const;

    //获取可读取的缓存，保存成iovec数组
    uint64_t getReadBuffers(std::vector<iovec> &buffers,
                            uint64_t len = ~0ull) const;
    uint64_t getReadBuffers(std::vector<iovec> &buffers, uint64_t len,
                            uint64_t position) const;
    uint64_t getWriteBuffers(std::vector<iovec> &buffers, uint64_t len);

    //返回数据长度
    size_t getSize() const { return m_size; }

    //写固定长度signed/unsigned数据
    void writeFint8(int8_t value);
    void writeFuint8(uint8_t value);
    void writeFint16(int16_t value);
    void writeFuint16(uint16_t value);
    void writeFint32(int32_t value);
    void writeFuint32(uint32_t value);
    void writeFint64(int64_t value);
    void writeFuint64(uint64_t value);

    //写signed/unsigned数据(可压缩，例如写255以下的数据其实只需要写1个字节)
    void writeInt32(int32_t value);
    void writeUint32(uint32_t value);
    void writeInt64(int64_t value);
    void writeUint64(uint64_t value);

    //写float/double
    void writeFloat(float value);
    void writeDouble(double value);

    //写string，要求string长度可以用uint16_t/uint32_t/uint64_t表示
    void writeStringF16(const std::string &value);
    void writeStringF32(const std::string &value);
    void writeStringF64(const std::string &value);

    //写string，长度可压缩
    void writeStringVint(const std::string &value);
    //写string，不带长度
    void writeStringWithoutLength(const std::string &value);

    //读
    int8_t readFint8();
    uint8_t readFuint8();
    int16_t readFint16();
    uint16_t readFuint16();
    int32_t readFint32();
    uint32_t readFuint32();
    int64_t readFint64();
    uint64_t readFuint64();

    int32_t readInt32();
    uint32_t readUint32();
    int64_t readInt64();
    uint64_t readUint64();

    float readFloat();
    double readDouble();

    std::string readStringF16();
    std::string readStringF32();
    std::string readStringF64();
    std::string readStringVint();

private:
    //扩容ByteArray，使其可以容纳size个数据（如果原本可以内容，则不扩容）
    void addCapacity(size_t size);
    //获取当前的可写入容量
    size_t getCapacity() const { return m_capacity - m_position; }

private:
    size_t m_baseSize;
    size_t m_position;
    size_t m_capacity;
    size_t m_size;
    int8_t m_endian;
    Node *m_root;
    Node *m_cur;
};

} // namespace sylar

#endif