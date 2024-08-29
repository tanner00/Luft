#pragma once

template<typename Type, Type V>
struct Constant
{
	static constexpr Type Value = V;
};

using FalseConstant = Constant<bool, false>;
using TrueConstant = Constant<bool, true>;

template<typename T, typename U>
struct IsSame : FalseConstant {};
template<typename T>
struct IsSame<T, T> : TrueConstant {};

template<typename T>
struct IsLValueReference : FalseConstant {};
template<typename T>
struct IsLValueReference<T&> : TrueConstant {};

template<typename T>
struct RemoveReference { using Type = T; };
template<typename T>
struct RemoveReference<T&> { using Type = T; };
template<typename T>
struct RemoveReference<T&&> { using Type = T; };
template<typename T>
using RemoveReferenceType = typename RemoveReference<T&&>::Type;

template<typename T>
constexpr RemoveReferenceType<T>&& Move(T&& toMove) noexcept
{
	using NoReference = RemoveReferenceType<T>;
	static_assert(!IsSame<NoReference&, const NoReference&>::Value, "Move called erroneously on a const value!");
	static_assert(IsLValueReference<T>::Value, "Move called erroneously on an r-value!");

	return static_cast<NoReference&&>(toMove);
}

template<typename T>
T&& Forward(RemoveReferenceType<T>& toForward) noexcept
{
	return static_cast<T&&>(toForward);
}

template<typename T>
T&& Forward(RemoveReferenceType<T>&& toForward) noexcept
{
	return static_cast<T&&>(toForward);
}

template<typename T>
struct IsTriviallyCopyable : Constant<bool, __is_trivially_copyable(T)> {};
