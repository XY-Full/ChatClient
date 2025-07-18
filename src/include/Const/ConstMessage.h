#pragma once

#include <string>
#include <type_traits>

template<typename EnumType>
struct EnumToString;

#define ENUM_TO_STRING(ENUM_TYPE, ...) \
    template<> \
    struct EnumToString<ENUM_TYPE> { \
        static constexpr const char* names[] = { __VA_ARGS__ }; \
        static std::string toString(ENUM_TYPE value) { \
            if (static_cast<size_t>(value) < sizeof(names) / sizeof(names[0])) { \
                return #ENUM_TYPE "::" + std::string(names[static_cast<size_t>(value)]); \
            } \
            return #ENUM_TYPE "::UNKNOWN"; \
        } \
    };

template<typename EnumType>
std::string enumToString(EnumType value) {
    return EnumToString<EnumType>::toString(value);
}


enum class MessageActionType {
	SendMessage,
	ReceiveMessage,
    OnPrintMessage
};

ENUM_TO_STRING(MessageActionType, "SendMessage", "ReceiveMessage", "OnPrintMessage")
