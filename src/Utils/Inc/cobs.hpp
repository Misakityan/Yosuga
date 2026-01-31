//
// Created by misaki on 2026/1/26.
//

/**
 * cobs.hpp
 * 所谓COBS,即Consistent Overhead Byte Stuffing(持续开销字节填充)
 * 是一种将字节包编码成不包含值为零的字节（0x00）形式的方法。
 * 输入的字节包可以包含从 0x00 到 0xFF 的全部范围内的字节。
 * COBS 编码的数据包保证生成字节范围 0x01 到 0xFF 的数据包。
 * 因此，在通信协议中，数据包边界可以用 0x00 字节可靠地界定。
 *
 * 在Yosuga项目当中，COBS编码被用于解决Yosuga与嵌入式设备使用串口收发数据时出现的粘包问题。
 * 之所以使用COBS编码而不是常用的字符填充法，这是因为字符填充法会使得数据包的大小无法确定，并且往往会使得数据包变得更大。
 *
 * 本模块为COBS的C++实现，而在Yosuga_embedded当中，则使用了cobs的C实现。
 *
 * C++20
 */
#pragma once
#include <cstdint>
#include <cstddef>
#include <span>
#include <vector>
#include <string>

namespace cobs {

// 状态码
enum class [[nodiscard]] Status : uint8_t {
    OK = 0x00,
    NULL_POINTER = 0x01,
    OUT_BUFFER_OVERFLOW = 0x02,
    ZERO_BYTE_IN_INPUT = 0x04,      // 仅 decode
    INPUT_TOO_SHORT = 0x08          // 仅 decode
};

// 结果结构体
struct [[nodiscard]] EncodeResult {
    size_t out_len = 0;
    Status status = Status::OK;
};

struct [[nodiscard]] DecodeResult {
    size_t out_len = 0;
    Status status = Status::OK;
};

// 缓冲区大小计算
constexpr size_t encode_dst_len_max(const size_t src_len) noexcept {
    return (src_len == 0) ? 1 : (src_len + (src_len + 253) / 254);
}

constexpr size_t decode_dst_len_max(const size_t src_len) noexcept {
    return (src_len == 0) ? 0 : (src_len - 1);
}

constexpr size_t encode_src_offset(const size_t src_len) noexcept {
    return (src_len + 253) / 254;
}

// 底层核心实现
inline EncodeResult encode_core(std::span<uint8_t> dst, const std::span<const uint8_t> src) noexcept {
    EncodeResult result;
    if (dst.empty() || src.empty()) {
        result.status = Status::NULL_POINTER;
        return result;
    }

    const uint8_t* src_read_ptr = src.data();
    const uint8_t* src_end_ptr = src_read_ptr + src.size();
    uint8_t* dst_start_ptr = dst.data();
    const uint8_t* dst_end_ptr = dst_start_ptr + dst.size();
    uint8_t* dst_code_write_ptr = dst_start_ptr;
    uint8_t* dst_write_ptr = dst_code_write_ptr + 1;
    uint8_t search_len = 1;

    if (src.empty()) {
        *dst_code_write_ptr = search_len;
        result.out_len = 1;
        return result;
    }

    for (;;) {
        if (dst_write_ptr >= dst_end_ptr) {
            result.status = Status::OUT_BUFFER_OVERFLOW;
            break;
        }

        const uint8_t src_byte = *src_read_ptr++;
        if (src_byte == 0) {
            *dst_code_write_ptr = search_len;
            dst_code_write_ptr = dst_write_ptr++;
            search_len = 1;
            if (src_read_ptr >= src_end_ptr) break;
        } else {
            *dst_write_ptr++ = src_byte;
            search_len++;
            if (src_read_ptr >= src_end_ptr) break;
            if (search_len == 0xFF) {
                *dst_code_write_ptr = search_len;
                dst_code_write_ptr = dst_write_ptr++;
                search_len = 1;
            }
        }
    }

    if (dst_code_write_ptr >= dst_end_ptr) {
        result.status = Status::OUT_BUFFER_OVERFLOW;
    } else {
        *dst_code_write_ptr = search_len;
    }

    result.out_len = static_cast<size_t>(dst_write_ptr - dst_start_ptr);
    return result;
}

inline DecodeResult decode_core(std::span<uint8_t> dst, const std::span<const uint8_t> src) noexcept {
    DecodeResult result;
    if (dst.empty() || src.empty()) {
        result.status = Status::NULL_POINTER;
        return result;
    }

    const uint8_t* src_read_ptr = src.data();
    const uint8_t* src_end_ptr = src_read_ptr + src.size();
    uint8_t* dst_start_ptr = dst.data();
    uint8_t* dst_end_ptr = dst_start_ptr + dst.size();
    uint8_t* dst_write_ptr = dst_start_ptr;

    if (src.empty()) {
        return result; // out_len = 0, status = OK
    }

    for (;;) {
        uint8_t len_code = *src_read_ptr++;
        if (len_code == 0) {
            result.status = Status::ZERO_BYTE_IN_INPUT;
            break;
        }
        len_code--;

        auto remaining = static_cast<size_t>(src_end_ptr - src_read_ptr);
        if (len_code > remaining) {
            result.status = Status::INPUT_TOO_SHORT;
            len_code = static_cast<uint8_t>(remaining);
        }

        remaining = static_cast<size_t>(dst_end_ptr - dst_write_ptr);
        if (len_code > remaining) {
            result.status = Status::OUT_BUFFER_OVERFLOW;
            len_code = static_cast<uint8_t>(remaining);
        }

        for (uint8_t i = len_code; i != 0; i--) {
            const uint8_t src_byte = *src_read_ptr++;
            if (src_byte == 0) {
                result.status = Status::ZERO_BYTE_IN_INPUT;
            }
            *dst_write_ptr++ = src_byte;
        }

        if (src_read_ptr >= src_end_ptr) break;
        if (len_code != 0xFE) {
            if (dst_write_ptr >= dst_end_ptr) {
                result.status = Status::OUT_BUFFER_OVERFLOW;
                break;
            }
            *dst_write_ptr++ = 0;
        }
    }

    result.out_len = static_cast<size_t>(dst_write_ptr - dst_start_ptr);
    return result;
}

// 便捷接口（std::vector）
inline EncodeResult encode(std::vector<uint8_t>& dst, const std::span<const uint8_t> src) noexcept {
    dst.resize(encode_dst_len_max(src.size()));
    const auto result = encode_core(dst, src);
    dst.resize(result.out_len);
    return result;
}

inline DecodeResult decode(std::vector<uint8_t>& dst, const std::span<const uint8_t> src) noexcept {
    dst.resize(decode_dst_len_max(src.size()));
    const auto result = decode_core(dst, src);
    dst.resize(result.out_len);
    return result;
}

// 便捷接口（std::string）
inline EncodeResult encode(std::string& dst, const std::string_view src) noexcept {
    dst.resize(encode_dst_len_max(src.size()));
    const auto result = encode_core(
        std::span<uint8_t>(reinterpret_cast<uint8_t*>(dst.data()), dst.size()),
        std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(src.data()), src.size())
    );
    dst.resize(result.out_len);
    return result;
}

inline DecodeResult decode(std::string& dst, const std::span<const uint8_t> src) noexcept {
    std::vector<uint8_t> temp;
    const auto result = decode(temp, src);
    if (result.status == Status::OK) {
        dst.assign(reinterpret_cast<const char*>(temp.data()), temp.size());
    }
    return {result.out_len, result.status};
}

// 类型安全辅助函数
template <typename T>
requires std::is_trivially_copyable_v<T>
inline EncodeResult encode(std::vector<uint8_t>& dst, const T& obj) noexcept {
    return encode(dst, std::span<const uint8_t>(
        reinterpret_cast<const uint8_t*>(&obj), sizeof(T)
    ));
}

template <typename T>
requires std::is_trivially_copyable_v<T>
inline DecodeResult decode(T& obj, const std::span<const uint8_t> src) noexcept {
    return decode_core(std::span<uint8_t>(
        reinterpret_cast<uint8_t*>(&obj), sizeof(T)
    ), src);
}

} // namespace cobs