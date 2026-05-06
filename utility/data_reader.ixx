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
    std::span<uint8_t> data;

public:
    template<n_t n_bytes>
    auto readInt(data_reader::Address address)
    {
        if constexpr (n_bytes == 1)
            return data[address];
        else if constexpr (n_bytes == 2)
            return uint16_t(data[address] | data[address + 1]);
        else if constexpr (n_bytes == 3)
            return uint32_t(data[address] | data[address + 1] | data[address + 2]);
        else if constexpr (n_bytes == 4)
            return uint32_t(data[address] | data[address + 1] | data[address + 2] | data[address + 3]);
        else
            static_assert(false, "Unsupported size");
    }
};