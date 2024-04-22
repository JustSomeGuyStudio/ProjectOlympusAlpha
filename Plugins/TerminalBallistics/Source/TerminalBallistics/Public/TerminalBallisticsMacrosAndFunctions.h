// Copyright 2023, Erik Hedberg. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/UnrealTypeTraits.h"
#include "TerminalBallistics.h"
#include "TerminalBallisticsLogChannels.h"

#define TB_VALID(x) (x != nullptr && x->IsValidLowLevel())
#define TB_VALID_OBJECT(obj) (IsValid(obj) && !obj->IsUnreachable())

#define TB_RET_COND_VALUE(Condition, ToReturn) \
if(Condition) { \
	return ToReturn; \
}
#define TB_RET_COND(Condition) \
if(Condition) { \
	return; \
}

#define TB_CHECK_RICOCHET_RET(ToReturn) TB_RET_COND_VALUE(!TB::Configuration::bEnableRicochets, ToReturn)
#define TB_CHECK_RICOCHET_BULLET_RET(ToReturn) TB_RET_COND_VALUE(!TB::Configuration::bEnableRicochets || !TB::Configuration::bEnableBulletRicochets, ToReturn)
#define TB_CHECK_RICOCHET_PROJECTILE_RET(ToReturn) TB_RET_COND_VALUE(!TB::Configuration::bEnableRicochets || !TB::Configuration::bEnableProjectileRicochets, ToReturn)

/* Logging Macros */
#define TB_LOG(Verbosity, Format, ...) UE_LOG(LogTerminalBallistics, Verbosity, Format, ##__VA_ARGS__)
#define TB_LOG_STR(Verbosity, Str) UE_LOG(LogTerminalBallistics, Verbosity, TEXT("%s"), *Str)

#define TB_SPACER_TEXT TEXT("----------------------------------------------------------------")
#define TB_LOG_SPACER() TB_LOG(Display, TB_SPACER_TEXT)
#define TB_LOG_WRAPPED(Verbosity, Format, ...) \
{ \
	TB_LOG_SPACER(); \
	TB_LOG(Verbosity, Format, ##__VA_ARGS__); \
	TB_LOG_SPACER(); \
}


// Helper to generate the necessary functions to allow a struct that wraps an array to be iterable.
#define TB_ARRAY_WRAPPED_STRUCT_ITERATOR_FUNCS(ArrayVariable) \
	FORCEINLINE auto begin()		{ return ArrayVariable.begin(); } \
	FORCEINLINE auto begin() const	{ return ArrayVariable.begin(); } \
	FORCEINLINE auto end()			{ return ArrayVariable.end(); } \
	FORCEINLINE auto end() const	{ return ArrayVariable.end(); }

struct FHitResult;
namespace TB
{
	extern TERMINALBALLISTICS_API bool HitResultsAreEqual(const FHitResult& A, const FHitResult& B);

	/**
	* Checks to see if two HitResult structures are equal.
	* @param A							The first FHitResult
	* @param B							The second FHitResult
	* @param bCheckStartAndEndAreSame	Should we compare the TraceStart and TraceEnd of A to the TraceStart and TraceEnd of B?
	* @param bCheckComponentsAreSame	Should we compare the components of A and B?
	* @param bCheckLocationsAreSame		Should we compare the ImpactLocations of A and B?
	* @param bCheckNormalsAreSame		Should we compare the ImpactNormals of A and B?
	* @param bCheckFaceIndicies			Should we compare the FaceIndex of A and B?
	* @param Tolerance					Location and Normal error tolerance
	*/
	extern TERMINALBALLISTICS_API bool HitResultsAreEqualStrict(const FHitResult& A, const FHitResult& B, bool bCheckStartAndEndAreSame = true, bool bCheckComponentsAreSame = true, bool bCheckLocationsAreSame = true, bool bCheckNormalsAreSame = true, bool bCheckFaceIndiciesAreSame = true, double Tolerance = 1e-5);

	template<typename ValueType, typename PredicateType>
	bool SerializeValueConditional(const bool bIsSaving, FArchive& Ar, ValueType& Value, const ValueType& DefaultValue, PredicateType&& Predicate)
	{
		bool bShouldSerialize = (bIsSaving && Predicate(Value));
		Ar.SerializeBits(&bShouldSerialize, 1);
		if (bShouldSerialize)
		{
			Ar << Value;
		}
		else if (!bIsSaving)
		{
			// Loading, and should use default
			Value = DefaultValue;
		}

		return bShouldSerialize;
	}
	template<typename ValueType>
	bool SerializeValueConditional(const bool bIsSaving, FArchive& Ar, ValueType& Value, const ValueType& DefaultValue, bool Condition)
	{
		return SerializeValueConditional(bIsSaving, Ar, Value, DefaultValue, [&](ValueType) { return Condition; });
	}

	namespace BitPackHelpers
	{
		template<typename T>
		struct TIsUnsignedInt
		{
			enum
			{
				Value =
				std::is_same<T, uint8>::value ||
				std::is_same<T, uint16>::value ||
				std::is_same<T, uint32>::value ||
				std::is_same<T, uint64>::value
			};
		};

		template<typename T, TEMPLATE_REQUIRES(TIsUnsignedInt<T>::Value)>
		constexpr int GetMax() { return 8; }
		template<> constexpr int GetMax<uint16>() { return 16; }
		template<> constexpr int GetMax<uint32>() { return 32; }
		template<> constexpr int GetMax<uint64>() { return 64; }

		namespace Private
		{
			template<typename FlagType = uint8, typename bool_like, TEMPLATE_REQUIRES(TIsUnsignedInt<FlagType>::Value)>
			void PackBool(FlagType* flags, size_t index, bool_like Bool)
			{
				*flags |= Bool << index;
			}

			template<typename FlagType = uint8, TEMPLATE_REQUIRES(TIsUnsignedInt<FlagType>::Value)>
			void UnpackBool(FlagType flags, size_t index, bool* Bool)
			{
				*Bool = flags & (1 << index);
			}


			template<typename FlagType, typename ...Args>
			void PackBits_helper(FlagType* flags, size_t i, Args... args)
			{
				((void)PackBool(flags, i++, args), ...);
			}

			template<typename FlagType, typename ...Args>
			void UnpackBits_helper(FlagType flags, size_t i, Args*... args)
			{
				((void)UnpackBool(flags, i++, args), ...);
			}
		};

		/**
		* Helper function to pack a variable number of bool values into an unisgned integer.
		* ex: PackBits<3>(true, false, true); Would return 0000 0101
		* @return	An unsigned integer (uint8/uint16/uint32/uint64) containing the given bool values.
		*/
		template<int NumBools, typename FlagType = uint8, TEMPLATE_REQUIRES(TIsUnsignedInt<FlagType>::Value), typename ...Args>
		FORCEINLINE static FlagType PackBits(Args ...Bools)
		{
			static_assert(NumBools > 0, "At least one value must be provided.");
			static_assert(NumBools <= GetMax<FlagType>(), "Too many values provided.");

			FlagType flags = 0U;
			Private::PackBits_helper(&flags, 0, Bools...);
			return flags;
		}

		/**
		* Helper function to unpack a variable number of bools from an unsigned integer.
		*/
		template<int NumBools, typename FlagType = uint8, TEMPLATE_REQUIRES(TIsUnsignedInt<FlagType>::Value), typename ...Args>
		FORCEINLINE void UnpackBits(FlagType flags, Args* ...Bools)
		{
			static_assert(NumBools > 0, "At least one value must be provided.");
			static_assert(NumBools <= GetMax<FlagType>(), "Too many values provided.");

			Private::UnpackBits_helper(flags, 0, Bools...);
		}

		/**
		* Helper function to serialize a variable number of bools into an archive.
		*/
		template<int NumBools, typename FlagType = uint8, TEMPLATE_REQUIRES(TIsUnsignedInt<FlagType>::Value), typename ...Args>
		FORCEINLINE void PackArchive(FArchive& Ar, Args ...Bools)
		{
			FlagType flags = 0U;
			if (Ar.IsSaving())
			{
				flags = PackBits<NumBools>(*Bools...);
			}
			Ar.SerializeBits(&flags, NumBools);
			if(Ar.IsLoading())
			{
				UnpackBits<NumBools>(flags, Bools...);
			}
		}

		template<> FORCEINLINE void PackArchive<1>(FArchive& Ar, bool* Bool)
		{
			Ar.SerializeBits(Bool, 1);
		}


		template<typename FlagType = uint8, typename ...Args>
		FORCEINLINE void PackArchive(FArchive& Ar, Args ...Bools)
		{
			size_t NumBools = sizeof...(Bools) / sizeof(bool);

			FlagType flags = 0U;
			if (Ar.IsSaving())
			{
				Private::PackBits_helper(&flags, 0, *Bools...);
			}
			Ar.SerializeBits(&flags, NumBools);
			if (Ar.IsLoading())
			{
				Private::UnpackBits_helper(flags, 0, Bools...);
			}
		}
	};
};


#define TB_PACK_ARCHIVE_WITH_BITFIELDS_ONE(Ar, BitOne) \
{ \
	bool Temp1 = BitOne; \
	Ar.SerializeBits(&Temp1, 1); \
	if (Ar.IsLoading()) \
	{ \
		BitOne = Temp1; \
	} \
}

#define TB_PACK_ARCHIVE_WITH_BITFIELDS_TWO(Ar, BitOne, BitTwo) \
{ \
	bool Temp1 = BitOne; \
	bool Temp2 = BitTwo; \
	TB::BitPackHelpers::PackArchive(Ar, &Temp1, &Temp2); \
	if (Ar.IsLoading()) \
	{ \
		BitOne = Temp1; \
		BitTwo = Temp2; \
	} \
};

#define TB_PACK_ARCHIVE_WITH_BITFIELDS_THREE(Ar, BitOne, BitTwo, BitThree) \
{ \
    bool Temp1 = BitOne; \
    bool Temp2 = BitTwo; \
    bool Temp3 = BitThree; \
    TB::BitPackHelpers::PackArchive(Ar, &Temp1, &Temp2, &Temp3); \
	if (Ar.IsLoading()) \
	{ \
		BitOne = Temp1; \
		BitTwo = Temp2; \
		BitThree = Temp3; \
	} \
};

#define TB_PACK_ARCHIVE_WITH_BITFIELDS_FOUR(Ar, BitOne, BitTwo, BitThree, BitFour) \
{ \
    bool Temp1 = BitOne; \
    bool Temp2 = BitTwo; \
    bool Temp3 = BitThree; \
    bool Temp4 = BitFour; \
    TB::BitPackHelpers::PackArchive(Ar, &Temp1, &Temp2, &Temp3, &Temp4); \
	if (Ar.IsLoading()) \
	{ \
		BitOne = Temp1; \
		BitTwo = Temp2; \
		BitThree = Temp3; \
		BitFour = Temp4; \
	} \
};

#define TB_PACK_ARCHIVE_WITH_BITFIELDS_FIVE(Ar, BitOne, BitTwo, BitThree, BitFour, BitFive) \
{ \
    bool Temp1 = BitOne; \
    bool Temp2 = BitTwo; \
    bool Temp3 = BitThree; \
    bool Temp4 = BitFour; \
    bool Temp5 = BitFive; \
    TB::BitPackHelpers::PackArchive(Ar, &Temp1, &Temp2, &Temp3, &Temp4, &Temp5); \
	if (Ar.IsLoading()) \
	{ \
		BitOne = Temp1; \
		BitTwo = Temp2; \
		BitThree = Temp3; \
		BitFour = Temp4; \
		BitFive = Temp5; \
	} \
};

#define TB_PACK_ARCHIVE_WITH_BITFIELDS_SIX(Ar, BitOne, BitTwo, BitThree, BitFour, BitFive, BitSix) \
{ \
    bool Temp1 = BitOne; \
    bool Temp2 = BitTwo; \
    bool Temp3 = BitThree; \
    bool Temp4 = BitFour; \
    bool Temp5 = BitFive; \
    bool Temp6 = BitSix; \
    TB::BitPackHelpers::PackArchive(Ar, &Temp1, &Temp2, &Temp3, &Temp4, &Temp5, &Temp6); \
	if (Ar.IsLoading()) \
	{ \
		BitOne = Temp1; \
		BitTwo = Temp2; \
		BitThree = Temp3; \
		BitFour = Temp4; \
		BitFive = Temp5; \
		BitSix = Temp6; \
	} \
};

#define TB_PACK_ARCHIVE_WITH_BITFIELDS_SEVEN(Ar, BitOne, BitTwo, BitThree, BitFour, BitFive, BitSix, BitSeven) \
{ \
    bool Temp1 = BitOne; \
    bool Temp2 = BitTwo; \
    bool Temp3 = BitThree; \
    bool Temp4 = BitFour; \
    bool Temp5 = BitFive; \
    bool Temp6 = BitSix; \
    bool Temp7 = BitSeven; \
    TB::BitPackHelpers::PackArchive(Ar, &Temp1, &Temp2, &Temp3, &Temp4, &Temp5, &Temp6, &Temp7); \
	if (Ar.IsLoading()) \
	{ \
		BitOne = Temp1; \
		BitTwo = Temp2; \
		BitThree = Temp3; \
		BitFour = Temp4; \
		BitFive = Temp5; \
		BitSix = Temp6; \
		BitSeven = Temp7; \
	} \
};

#define TB_PACK_ARCHIVE_WITH_BITFIELDS_EIGHT(Ar, BitOne, BitTwo, BitThree, BitFour, BitFive, BitSix, BitSeven, BitEight) \
{ \
    bool Temp1 = BitOne; \
    bool Temp2 = BitTwo; \
    bool Temp3 = BitThree; \
    bool Temp4 = BitFour; \
    bool Temp5 = BitFive; \
    bool Temp6 = BitSix; \
    bool Temp7 = BitSeven; \
    bool Temp8 = BitEight; \
    TB::BitPackHelpers::PackArchive(Ar, &Temp1, &Temp2, &Temp3, &Temp4, &Temp5, &Temp6, &Temp7, &Temp8); \
	if (Ar.IsLoading()) \
	{ \
		BitOne = Temp1; \
		BitTwo = Temp2; \
		BitThree = Temp3; \
		BitFour = Temp4; \
		BitFive = Temp5; \
		BitSix = Temp6; \
		BitSeven = Temp7; \
		BitEight = Temp8; \
	} \
};

