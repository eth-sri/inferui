#!/usr/bin/python

import io
import six
import zlib

class ZlibReader:
    def __init__(self, filename):
        self.zip = zlib.decompressobj(16 + zlib.MAX_WBITS)
        self.f = io.open(filename, 'rb')
        self.data = ""
        self.blocksize = 16384

    def _fill(self, size):
        if self.zip:
            while len(self.data) < size:
                data = self.f.read(self.blocksize)
                if not data:
                    self.data += self.zip.flush()
                    self.zip = None # no more data
                    break
                self.data += self.zip.decompress(data)

    def read(self, size):
        assert size > 0
        self._fill(size)
        result = self.data[:size]
        self.data = self.data[size:]
        return result

    def close(self):
        self.f.close()
        self.zip = None


class ZlibWriter:
    def __init__(self, filename):
        self.zip = zlib.compressobj(zlib.Z_DEFAULT_COMPRESSION, zlib.DEFLATED, 16 + zlib.MAX_WBITS)
        self.f = io.open(filename, 'wb')

    def write(self, data):
        compressed_data = self.zip.compress(data)
        # zlib uses internal buffer
        if compressed_data:
            self.f.write(compressed_data)

    def close(self):
        data = self.zip.flush()
        self.f.write(data)
        self.f.close()
        self.zip = None


def DecodeVarintFromStream(stream):
    mask = (1 << 32) - 1
    result = 0
    shift = 0
    while 1:
        buffer = stream.read(1)
        b = six.indexbytes(buffer, 0)
        result |= ((b & 0x7f) << shift)
        if not (b & 0x80):
            if result > 0x7fffffff:
                result -= (1 << 32)
                result |= ~mask
            else:
                result &= mask
            return result
        shift += 7
        if shift >= 32:
            raise _DecodeError('Too many bytes when decoding varint.') 

# Taken from: https://github.com/google/protobuf/blob/master/python/google/protobuf/internal/encoder.py#L391
def WriteVarintToStream(stream, value):
    if value < 0:
        value += (1 << 32)
    bits = value & 0x7f
    value >>= 7
    while value:
        stream.write(six.int2byte(0x80|bits))
        bits = value & 0x7f
        value >>= 7
    return stream.write(six.int2byte(bits))

class RecordReader:
    def __init__(self, filename, compressed = False):
        if compressed:
            self.f = ZlibReader(filename)
        else:
            self.f = io.open(filename, 'rb')
    
    def read(self, proto):
        size = DecodeVarintFromStream(self.f)
        if size < 0:
            return False
        buf = self.f.read(size)
        while len(buf) < size:
            buf += self.f.read(size - len(buf))
        proto.ParseFromString(buf)
        return True
    
    def close(self):
        self.f.close()
        

class RecordWriter:
    def __init__(self, filename, compressed = False):
        if compressed:
            self.f = ZlibWriter(filename)
        else:
            self.f = io.open(filename, 'wb')

    def write(self, proto):
        msg = proto.SerializeToString()
        WriteVarintToStream(self.f, len(msg))
        self.f.write(msg)

    def close(self):
        WriteVarintToStream(self.f, -1)
        self.f.close()

