// Copyright 2023, Erik Hedberg. All Rights Reserved.

#pragma once

#include "Templates/AndOrNot.h"
#include "Templates/UnrealTypeTraits.h"
#include "Engine/NetSerialization.h"


// Helper macro to define CV qualified versions of a traits class
#define TB_TRAITS_CV_QUALIFICATION(Trait) \
template <typename T> struct Trait<const			T> { enum { Value = Trait<T>::Value }; }; \
template <typename T> struct Trait<volatile			T> { enum { Value = Trait<T>::Value }; }; \
template <typename T> struct Trait<const volatile	T> { enum { Value = Trait<T>::Value }; }

#define TB_WITH_OPTIMIZED_SERIALIZER() using HasOptimizedSerializer = std::true_type

namespace TB::Traits
{
	template<typename ImpactStruct>
	struct TIsImpactStruct
	{
		enum { Value = false };
	};
	TB_TRAITS_CV_QUALIFICATION(TIsImpactStruct);

	template<typename TypeOfSimData>
	struct TIsSimData
	{
		enum { Value = false };
	};
	TB_TRAITS_CV_QUALIFICATION(TIsSimData);



	/* Traits class which tests if a type is a TSimTaskDelegates struct */
	template<typename ...Ts>
	struct TIsSimTaskDelegateStruct
	{
		enum
		{
			Value = false
		};
	};
	TB_TRAITS_CV_QUALIFICATION(TIsSimTaskDelegateStruct);



	/* Traits class which tests if a type is an FVector */
	template<typename T>
	struct TIsFVector
	{
		enum { Value = false };
	};

	template<> struct TIsFVector<FVector3d> { enum { Value = true }; };
	template<> struct TIsFVector<FVector3f> { enum { Value = true }; };
	template<> struct TIsFVector<FVector_NetQuantize> { enum { Value = true }; };
	template<> struct TIsFVector<FVector_NetQuantize10> { enum { Value = true }; };
	template<> struct TIsFVector<FVector_NetQuantize100> { enum { Value = true }; };
	template<> struct TIsFVector<FVector_NetQuantizeNormal> { enum { Value = true }; };
	template<typename T> struct TIsFVector<UE::Math::TVector<T>> { enum { Value = true }; };

	TB_TRAITS_CV_QUALIFICATION(TIsFVector);


	/* Traits class which tests if a type is an FVector2D */
	template<typename T>
	struct TIsFVector2D
	{
		enum { Value = false };
	};

	template<> struct TIsFVector2D<FVector2d> { enum { Value = true }; };
	template<> struct TIsFVector2D<FVector2f> { enum { Value = true }; };
	template<typename T> struct TIsFVector2D<UE::Math::TVector2<T>> { enum { Value = true }; };

	TB_TRAITS_CV_QUALIFICATION(TIsFVector2D);


	
	template<typename ...Ts>
	struct TAllAreArithmetic
	{
		static constexpr bool Value = (TIsArithmetic<Ts>::Value && ...);
	};



	// Templates used to determine the type with the highest precision.
	template<typename ...Ts>
	struct THighestPrecision;

	template<typename T>
	struct THighestPrecision<T>
	{
		using type = T;
	};

	template<typename T, typename ...Ts>
	struct THighestPrecision<T, Ts...>
	{
		using type = typename std::conditional<sizeof(T) >= sizeof(typename THighestPrecision<Ts...>::type),
			T,
			typename THighestPrecision<Ts...>::type>::type;
	};


	// specialization for empty parameter pack
	template<>
	struct THighestPrecision<>
	{
		using type = void;
	};

	template<typename ...Ts>
	using THighestPrecision_t = typename THighestPrecision<Ts...>::type;


	template<typename T>
	struct TTypeTraitsIfInvalid
	{
		enum
		{
			UseDefault = true, // If this type has been determined to be invalid, use its default value instead.
			MarkIfInvalid = false // If this type has been determined to be invalid, set "bIsInvalid" to true. This is mutually exclusive with "UseDefault" and takes precedence.
		};
	};

	template<typename T> struct TTypeTraitsIfInvalid<T&> : public TTypeTraitsIfInvalid<T> {};
	template<typename T> struct TTypeTraitsIfInvalid<const T> : public TTypeTraitsIfInvalid<T> {};
	template<typename T> struct TTypeTraitsIfInvalid<const T&> : public TTypeTraitsIfInvalid<T> {};
	template<typename T> struct TTypeTraitsIfInvalid<volatile T> : public TTypeTraitsIfInvalid<T> {};
	template<typename T> struct TTypeTraitsIfInvalid<volatile T&> : public TTypeTraitsIfInvalid<T> {};
	template<typename T> struct TTypeTraitsIfInvalid<const volatile T> : public TTypeTraitsIfInvalid<T> {};
	template<typename T> struct TTypeTraitsIfInvalid<const volatile T&> : public TTypeTraitsIfInvalid<T> {};


	template<typename Function>
	static constexpr bool THasFunction(Function)
	{
		return std::is_member_function_pointer_v<Function>;
	}


	template<typename Type>
	class THasCustomNetSerializer
	{
		template<typename T>
		static constexpr auto CheckForNetSerialize(void*) -> decltype(std::declval<T>().NetSerialize(std::declval<FArchive&>(), std::declval<UPackageMap*>(), std::declval<bool&>()), std::true_type());

		template<typename>
		static constexpr std::false_type CheckForNetSerialize();

		typedef decltype(CheckForNetSerialize<Type>(0)) hasNetSerialize;
	public:
		static constexpr bool value = hasNetSerialize::value;
	};
	template<typename T>
	constexpr bool THasCustomNetSerializer_v = THasCustomNetSerializer<T>::value;

	template<typename Type>
	class THasOptimizedNetSerializer
	{
		template<typename T>
		static constexpr auto Check(void*) -> decltype(std::is_same_v<typename T::HasOptimizedSerializer, std::true_type>, std::true_type());

		template<typename>
		static constexpr std::false_type Check(...);

		typedef decltype(Check<Type>(0)) type;
	public:
		static constexpr bool value = type::value;
	};
	template<typename T>
	constexpr bool THasOptimizedNetSerializer_v = THasOptimizedNetSerializer<T>::value;
};
