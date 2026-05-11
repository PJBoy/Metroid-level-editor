module;

#include "../global.h"

export module data_reader;

export namespace data_reader
{
    // A type can provide a conversion to this for address mapping
    struct Address
    {
        index_t address;

        operator index_t() const
        {
            return address;
        }
    };
}

export class DataReader
{
    std::span<const uint8_t> data;
    index_t address{};

public:
    DataReader() = default;
    explicit DataReader(std::span<const uint8_t> data);

    // Returns a fixed-width integer depending on `n_bytes`
    template<n_t n_bytes>
    constexpr auto peekInt(data_reader::Address address) const;
    
    // Returns an std::array of `n_ints` fixed-width integers
    template<n_t n_bytes, n_t n_ints>
    constexpr auto peekInts(data_reader::Address address) const;

    // Writes fixed-width integer depending on `n_bytes` to `vars...`
    template<n_t n_bytes>
    constexpr void peekIntsTo(data_reader::Address address, auto&... vars) const;
    
    // Return a subspan of data
    std::span<const uint8_t> peekBytes(data_reader::Address address, n_t n_bytes) const;

    // Read functions do peak operations using and advancing the working address
    template<n_t n_bytes>
    constexpr auto readInt();

    template<n_t n_bytes, n_t n_ints>
    constexpr auto readInts();

    template<n_t n_bytes>
    constexpr void readIntsTo(auto&... vars);
    
    std::span<const uint8_t> readBytes(n_t n_bytes);

    void seek(data_reader::Address address);
    data_reader::Address tell() const;
};

template<n_t n_bytes>
constexpr auto DataReader::peekInt(data_reader::Address address) const
{
    if constexpr (n_bytes == 1)
        return data[address];
    else if constexpr (n_bytes == 2)
        return uint16_t(data[address] | data[address + 1] << 8);
    else if constexpr (n_bytes == 3)
        return uint32_t(data[address] | data[address + 1] << 8 | data[address + 2] << 0x10);
    else if constexpr (n_bytes == 4)
        return uint32_t(data[address] | data[address + 1] << 8 | data[address + 2] << 0x10 | data[address + 3] << 0x18);
    else
        static_assert(false, "Unsupported size");
}

template<n_t n_bytes, n_t n_ints>
constexpr auto DataReader::peekInts(data_reader::Address address) const
{
    DataReader reader(*this);
    reader.seek({address});

    return reader.readInts<n_bytes, n_ints>();
}

template<n_t n_bytes>
constexpr void DataReader::peekIntsTo(data_reader::Address address, auto&... vars) const
{
    std::tie(vars...) = peekInts<n_bytes, sizeof...(vars)>(address);
}

template<n_t n_bytes>
constexpr auto DataReader::readInt()
{
    auto ret = peekInt<n_bytes>({address});
    address += n_bytes;

    return ret;
}

template<n_t n_bytes, n_t n_ints>
constexpr auto DataReader::readInts()
{
    using int_t = decltype(readInt<n_bytes>());
    std::array<int_t, n_ints> ret;
    for (int_t& datum : ret)
        datum = readInt<n_bytes>();

    return ret;
}

template<n_t n_bytes>
constexpr void DataReader::readIntsTo(auto&... vars)
{
    std::tie(vars...) = readInts<n_bytes, sizeof...(vars)>();
}
