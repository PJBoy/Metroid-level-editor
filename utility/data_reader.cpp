#include "../global.h"

import data_reader;

using namespace data_reader;

DataReader::DataReader(std::span<const uint8_t> data)
    : data(std::move(data))
{}

std::span<const uint8_t> DataReader::peekBytes(data_reader::Address address, n_t n_bytes) const
{
    return data.subspan(address, n_bytes);
}

std::span<const uint8_t> DataReader::peekBytes(n_t n_bytes) const
{
    return peekBytes({address}, n_bytes);
}

std::span<const uint8_t> DataReader::readBytes(n_t n_bytes)
{
    std::span<const uint8_t> ret = peekBytes(Address(address), n_bytes);
    address += n_bytes;

    return ret;
}

void DataReader::seek(data_reader::Address address_in)
{
    address = std::move(address_in);
}

data_reader::Address DataReader::tell() const
{
    return {address};
}
