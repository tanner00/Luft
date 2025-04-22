#pragma once

#define FLAGS_ENUM(enum)																												\
	inline enum& operator|=(enum& lhs, enum rhs) { return lhs = (enum)((__underlying_type(enum))lhs | (__underlying_type(enum))rhs); }	\
	inline enum& operator&=(enum& lhs, enum rhs) { return lhs = (enum)((__underlying_type(enum))lhs & (__underlying_type(enum))rhs); }	\
	inline enum& operator^=(enum& lhs, enum rhs) { return lhs = (enum)((__underlying_type(enum))lhs ^ (__underlying_type(enum))rhs); }	\
	inline constexpr enum operator|(enum lhs, enum rhs) { return (enum)((__underlying_type(enum))lhs | (__underlying_type(enum))rhs); }	\
	inline constexpr enum operator&(enum lhs, enum rhs) { return (enum)((__underlying_type(enum))lhs & (__underlying_type(enum))rhs); }	\
	inline constexpr enum operator^(enum lhs, enum rhs) { return (enum)((__underlying_type(enum))lhs ^ (__underlying_type(enum))rhs); }	\
	inline constexpr enum operator~(enum e) { return (enum)~(__underlying_type(enum))e; }												\
	inline constexpr bool operator!(enum e) { return !(__underlying_type(enum))e; }														\

template<typename E>
constexpr bool HasFlags(E e, E flags)
{
	return (static_cast<__underlying_type(E)>(e) & static_cast<__underlying_type(E)>(flags)) != 0;
}
