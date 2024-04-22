// Copyright 2022, Small Indie Company LLC, All rights reserved

#pragma once

#include "CoreMinimal.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Serialization/MemoryArchive.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectWriter.h"
#include "Serialization/ObjectReader.h"
#include "TerminalBallisticsTraits.h"

template<typename Type>
class THasNone
{
	template<typename T>
	static constexpr auto CheckForNone(void*) -> decltype(T::None, std::true_type());

	template<typename>
	static constexpr std::false_type CheckForNone(...);

	typedef decltype(CheckForNone<Type>(0)) type;
public:
	static constexpr bool value = type::value;
};

template<typename Type>
class THasEqualityOperators
{
	template<typename T>
	static constexpr auto CheckForEqualityOperator(void*) -> decltype(std::declval<T>().operator==(std::declval<T>()), std::true_type());

	template<typename T>
	static constexpr auto CheckForInequalityOperator(void*) -> decltype(std::declval<T>().operator!=(std::declval<T>()), std::true_type());

	template<typename>
	static constexpr std::false_type CheckForEqualityOperator(...);

	template<typename>
	static constexpr std::false_type CheckForInequalityOperator(...);

	typedef decltype(CheckForEqualityOperator<Type>(0)) hasEquality;
	typedef decltype(CheckForInequalityOperator<Type>(0)) hasInequality;
public:
	static constexpr bool value = hasEquality::value && hasInequality::value;
};

struct FTestWriter : public FMemoryWriter
{
	FTestWriter(TArray<uint8>& InBytes, bool bIsPersistent = false, bool bSetOffset = false, const FName InArchiveName = NAME_None)
		: FMemoryWriter(InBytes, bIsPersistent, bSetOffset, InArchiveName)
	{}
	
	FArchive& operator<<(UObject*& Obj)
	{
		ByteOrderSerialize(&Obj, sizeof(Obj));
		return *this;
	}

	FArchive& operator<<(FObjectPtr& Value)
	{
		ByteOrderSerialize(&Value, sizeof(Value));
		return *this;
	}

	FArchive& operator<<(FLazyObjectPtr& Value)
	{
		FUniqueObjectGuid ID = Value.GetUniqueID();
		return *this << ID;
	}

	FArchive& operator<<(FSoftObjectPtr& Value)
	{
		return *this << Value.GetUniqueID();
	}

	FArchive& operator<<(FSoftObjectPath& Value)
	{
		Value.SerializePath(*this);
		return *this;
	}

	FArchive& operator<<(FWeakObjectPtr& Value)
	{
		return FArchiveUObject::SerializeWeakObjectPtr(*this, Value);
	}
};

struct FTestReader : public FMemoryReader
{
	FTestReader(TArray<uint8>& InBytes, bool bIsPersistent = false)
		: FMemoryReader(InBytes, bIsPersistent)
	{}

	FArchive& operator<<(UObject*& Res)
	{
		ByteOrderSerialize(&Res, sizeof(Res));
		return *this;
	}

	FArchive& operator<<(FObjectPtr& Value)
	{
		ByteOrderSerialize(&Value, sizeof(Value));
		return *this;
	}

	FArchive& operator<<(FLazyObjectPtr& Value)
	{
		FArchive& Ar = *this;
		FUniqueObjectGuid ID;
		Ar << ID;

		Value = ID;
		return Ar;
	}

	FArchive& operator<<(FSoftObjectPtr& Value)
	{
		Value.ResetWeakPtr();
		return *this << Value.GetUniqueID();
	}

	FArchive& operator<<(FSoftObjectPath& Value)
	{
		Value.SerializePath(*this);
		return *this;
	}

	FArchive& operator<<(FWeakObjectPtr& Value)
	{
		return FArchiveUObject::SerializeWeakObjectPtr(*this, Value);
	}
};

template<typename StructType>
std::tuple<bool, FString> StructCompare(const StructType& lhs, const StructType& rhs)
{
	bool Equal = true;
	FString Info = FString::Printf(TEXT("%s Comparision:"), *(lhs.StaticStruct()->GetStructCPPName()));
	for (TFieldIterator<FProperty> it(lhs.StaticStruct()); it; ++it)
	{
		FProperty* prop = *it;
		const bool localEqual = prop->Identical_InContainer(&lhs, &rhs);
		Info = Info.Append(FString::Printf(TEXT("\n\t%s:\t%s"), *(prop->GetNameCPP()), localEqual ? L"true" : L"false"));
		Equal &= localEqual;
	}
	return { Equal, Info };
}

template<typename StructType>
class TTestWrapper
{
public:
	static StructType TestSerialize(const StructType& Struct, TArray<uint8>* Data = nullptr)
	{
		StructType TestStruct = Struct;

		TArray<uint8> SaveData;
		
		FTestWriter OutAr(SaveData);
		bool unused = false;
		TestStruct.NetSerialize(OutAr, nullptr, unused);

		//Deserialize into new struct
		FTestReader InAr(SaveData);
		StructType DeserializedStruct = StructType();
		DeserializedStruct.NetSerialize(InAr, nullptr, unused);
		if (Data)
		{
			*Data = SaveData;
		}

		return DeserializedStruct;
	}

	static bool StructTest(const StructType& Struct)
	{
		FGCScopeGuard GCGuard = FGCScopeGuard();
		StructType TestedStruct = TestSerialize(Struct);
		return TestedStruct == Struct;
	}

	static bool StructTest(FAutomationTestBase* Test, const StructType& Struct, const FString& TestDescription)
	{
		FGCScopeGuard GCGuard = FGCScopeGuard();
		StructType TestedStruct = TestSerialize(Struct);

		const bool TestEqual = Test->TestEqual(TestDescription, TestedStruct, Struct);

		if(!TestEqual)
		{
			auto [Equal, Info] = StructCompare(Struct, TestedStruct);
			Test->AddInfo(Info);
		}
		return TestEqual;
	}
	
	static int32 GetArchivedSize(const StructType& Struct)
	{
		TArray<uint8> Data = {};
		TestSerialize(Struct, &Data);
		return Data.Num();
	}

	static int32 GetArchiveSizeNative(const StructType& Struct)
	{
		TArray<uint8> Data;
		FTestWriter Ar(Data);
		UScriptStruct* ScriptStruct = Struct.StaticStruct();
		ScriptStruct->SerializeBin(Ar, (void*)&Struct);
		return Data.Num();
	}
};

template<typename StructType>
static FORCENOINLINE bool TestStructSerialization(FAutomationTestBase* Test, const StructType& TestStruct, const TCHAR* StructName)
{
	using TestWrapper = TTestWrapper<StructType>;

	bool bPassed = true;

	const StructType DefaultStruct = StructType();

	/* Test serialization/deserialization equality */
	{
		if constexpr (THasNone<StructType>::value)
		{
			const StructType StructNone = StructType::None;
			bPassed &= TestWrapper::StructTest(Test, StructNone, FString::Printf(TEXT("Serializing/Deserializing %s::None"), StructName));
		}
		bPassed &= TestWrapper::StructTest(Test, DefaultStruct, FString::Printf(TEXT("Serializing/Deserializing default %s"), StructName));
		bPassed &= TestWrapper::StructTest(Test, TestStruct, FString::Printf(TEXT("Serializing/Deserializing %s"), StructName));
	}

	/* Test serialization optimization */
	if constexpr (TB::Traits::THasOptimizedNetSerializer<StructType>::value)
	{
		const int32 ArchiveSizeDefault = TestWrapper::GetArchivedSize(DefaultStruct);
		const int32 ArchiveSizeNonDefault = TestWrapper::GetArchivedSize(TestStruct);
		const int32 ArchiveSizeNative = TestWrapper::GetArchiveSizeNative(TestStruct);

		const bool bDefaultIsSmallerThanNonDefault = ArchiveSizeDefault <= ArchiveSizeNonDefault;
		const bool bNonNativeIsLargerThanNative = ArchiveSizeNonDefault > ArchiveSizeNative;

		bPassed &= Test->TestFalse("User defined struct serialization is larger than native serialization", bNonNativeIsLargerThanNative);
		if constexpr (THasNone<StructType>::value)
		{
			const StructType StructNone = StructType::None;
			const int32 ArchiveSizeNone = TestWrapper::GetArchivedSize(StructNone);
			const bool bNoneIsSmallest = bDefaultIsSmallerThanNonDefault && ArchiveSizeNone <= ArchiveSizeDefault;
			bPassed &= Test->TestTrue("Default-like struct serialization/deserialization optimization", bNoneIsSmallest);
			Test->AddInfo(FString::Printf(TEXT("%s Archive Sizes:\n\tNative: %d\n\tDefault: %d\n\tNon-Default: %d\n\tNone: %d"), StructName, ArchiveSizeNative, ArchiveSizeDefault, ArchiveSizeNonDefault, ArchiveSizeNone));
		}
		else
		{
			bPassed &= Test->TestTrue("Default-like struct serialization/deserialization optimization", bDefaultIsSmallerThanNonDefault);
			Test->AddInfo(FString::Printf(TEXT("%s Archive Sizes:\n\tNative: %d\n\tDefault: %d\n\tNon-Default: %d"), StructName, ArchiveSizeNative, ArchiveSizeDefault, ArchiveSizeNonDefault));
		}
	}

	return bPassed;
}

template<typename StructType>
struct TStructSpecFixtureBase
{
	StructType TestStruct;

	TStructSpecFixtureBase() = default;

	TStructSpecFixtureBase(const StructType& TestStruct)
		: TestStruct(TestStruct)
	{}
};

template<typename Type>
class THasSpecFixtureBase
{
	template<typename T>
	static constexpr auto CheckForFixture(void*) -> decltype(std::declval<T>().TestFixture, std::true_type());

	template<typename>
	static constexpr std::false_type CheckForFixture(...);

	typedef decltype(CheckForFixture<Type>(0)) type_hasFixture;
public:
	static constexpr bool value = type_hasFixture::value;
};

template<
	typename StructType,
	typename SpecType,
	TEMPLATE_REQUIRES(THasSpecFixtureBase<SpecType>::value)
>
static void TestStructSerialization(SpecType* Spec, const StructType& TestStruct, const TCHAR* StructName)
{
	using TestWrapper = TTestWrapper<StructType>;

	const StructType DefaultStruct = StructType();

	Spec->Describe("Serialization", [&]()
		{
			Spec->It("Should serialize and deserialize correctly", [&]()
				{
					if constexpr (THasNone<StructType>::value)
					{
						const StructType StructNone = StructType::None;
						TestWrapper::StructTest(TestStruct);
					}
					TestWrapper::StructTest(DefaultStruct);
					TestWrapper::StructTest(TestStruct);
				}
			);

			if constexpr (TB::Traits::THasOptimizedNetSerializer_v<StructType>)
			{
				Spec->It("Should attempt to optimize serialization", [&]()
					{
						const int32 ArchiveSizeDefault = TestWrapper::GetArchivedSize(DefaultStruct);
						const int32 ArchiveSizeNonDefault = TestWrapper::GetArchivedSize(TestStruct);

						const bool bArchiveSizeIsLargerThanStructSize = FMath::Max(ArchiveSizeDefault, ArchiveSizeNonDefault) > sizeof(StructType);
						const bool bDefaultIsSmallerThanNonDefault = ArchiveSizeDefault < ArchiveSizeNonDefault;

						Spec->TestFalse(FString::Printf(TEXT("Archived size of %s is not larger than the size of %s"), StructName, StructName), bArchiveSizeIsLargerThanStructSize);
						Spec->TestTrue("Default-like struct has a smaller archive size than a non-default struct", bDefaultIsSmallerThanNonDefault);
						Spec->AddInfo(FString::Printf(TEXT("\t%s size: %d"), StructName, sizeof(StructType)));
						if constexpr (THasNone<StructType>::value)
						{
							const StructType StructNone = StructType::None;
							const int32 ArchiveSizeNone = TestWrapper::GetArchivedSize(StructNone);
							const bool bNoneIsSmallest = bDefaultIsSmallerThanNonDefault && ArchiveSizeNone <= ArchiveSizeDefault;

							Spec->TestTrue(FString::Printf(TEXT("Serializing %s::None has the smallest archive size"), StructName), bNoneIsSmallest);
							Spec->AddInfo(FString::Printf(TEXT("\t%s Archive Sizes:\n\t\tDefault: %d\n\t\tNon-Default: %d\n\t\tNone: %d"), StructName, ArchiveSizeDefault, ArchiveSizeNonDefault, ArchiveSizeNone));
						}
						else
						{
							Spec->AddInfo(FString::Printf(TEXT("\t%s Archive Sizes:\n\t\tDefault: %d\n\t\tNon-Default: %d"), StructName, ArchiveSizeDefault, ArchiveSizeNonDefault));
						}
					}
				);
			}
		}
	);
}

#define DESCRIBE_STRUCT_SERIALIZATION_SPEC_INNER(StructType) \
using TestWrapper = TTestWrapper<StructType>; \
It("Should serialize and deserialize correctly", [&]() \
	{ \
		if constexpr (THasNone<StructType>::value) \
		{ \
			TestWrapper::StructTest(StructType::None); \
		} \
		TestWrapper::StructTest(StructType()); \
		TestWrapper::StructTest(TestFixture->TestStruct); \
	} \
); \
\
if constexpr (TB::Traits::THasOptimizedNetSerializer_v<StructType>) \
{ \
	It("Should attempt to optimize serialization", [&]() \
		{ \
			const int32 ArchiveSizeDefault = TestWrapper::GetArchivedSize(StructType()); \
			const int32 ArchiveSizeNonDefault = TestWrapper::GetArchivedSize(TestFixture->TestStruct); \
\
			const bool bArchiveSizeIsLargerThanStructSize = FMath::Max(ArchiveSizeDefault, ArchiveSizeNonDefault) > sizeof(StructType); \
			const bool bDefaultIsSmallerThanNonDefault = ArchiveSizeDefault <= ArchiveSizeNonDefault; \
\
			TestFalse(FString::Printf(TEXT("Archived size of %s is not larger than the size of %s"), TEXT(#StructType), TEXT(#StructType)), bArchiveSizeIsLargerThanStructSize); \
			TestTrue("Default-like struct has a smaller archive size than a non-default struct", bDefaultIsSmallerThanNonDefault); \
			AddInfo(FString::Printf(TEXT("\t%s size: %d"), TEXT(#StructType), sizeof(StructType))); \
			if constexpr (THasNone<StructType>::value) \
			{ \
				const int32 ArchiveSizeNone = TestWrapper::GetArchivedSize(StructType::None); \
				const bool bNoneIsSmallest = bDefaultIsSmallerThanNonDefault && ArchiveSizeNone <= ArchiveSizeDefault; \
\
				TestTrue(FString::Printf(TEXT("Serializing %s::None has the smallest archive size"), TEXT(#StructType)), bNoneIsSmallest); \
				AddInfo(FString::Printf(TEXT("\t%s Archive Sizes:\n\t\tDefault: %d\n\t\tNon-Default: %d\n\t\tNone: %d"), TEXT(#StructType), ArchiveSizeDefault, ArchiveSizeNonDefault, ArchiveSizeNone)); \
			} \
			else \
			{ \
				AddInfo(FString::Printf(TEXT("\t%s Archive Sizes:\n\t\tDefault: %d\n\t\tNon-Default: %d"), TEXT(#StructType), ArchiveSizeDefault, ArchiveSizeNonDefault)); \
			} \
		} \
	); \
}


enum
{
	Functionality,
	Serialization
};

#define CREATE_TEST_NAME(StructType, TestType) PREPROCESSOR_JOIN(StructType##TestType, Test)
#define CREATE_SERIALIZATION_TEST_NAME(StructType) CREATE_TEST_NAME(StructType, Serialization)

#define CREATE_PRETTY_TEST_NAME(TestCategory, TClass, TestType) PREPROCESSOR_TO_STRING(TestCategory.TClass.TestType)

#define SERIALIZATION_ASSERTIONS(StructType) \
static_assert(THasEqualityOperators<StructType>::value, "Equality operators must be defined."); \
static_assert(TB::Traits::THasCustomNetSerializer_v<StructType>, "Struct must implement \"NetSerialize\"."); \
static_assert(TIsConstructible<StructType>::Value, "Struct must have a default constructor.");

#define IMPLEMENT_STRUCT_SERIALIZATION_TEST_INNER(StructTypeTestName, StructTypeTestNamePretty, Flags) \
IMPLEMENT_SIMPLE_AUTOMATION_TEST(StructTypeTestName, StructTypeTestNamePretty, Flags)

#define IMPLEMENT_STRUCT_SERIALIZATION_TEST_NEW(TestCategory, StructType, StructInstance, Flags) \
SERIALIZATION_ASSERTIONS(StructType) \
static const auto PREPROCESSOR_JOIN(StructType, TestNamePretty) = CREATE_PRETTY_TEST_NAME(TestCategory, StructType, Serialization); \
IMPLEMENT_STRUCT_SERIALIZATION_TEST_INNER(CREATE_SERIALIZATION_TEST_NAME(StructType), PREPROCESSOR_JOIN(StructType, TestNamePretty), Flags); \
bool CREATE_SERIALIZATION_TEST_NAME(StructType)::RunTest(const FString& Parameters) \
{ \
	return TestStructSerialization(this, StructInstance, TEXT(#StructType)); \
}

#define IMPLEMENT_STRUCT_SERIALIZATION_TEST(TestCategory, StructType, StructInstance, Flags) \
SERIALIZATION_ASSERTIONS(StructType) \
IMPLEMENT_SIMPLE_AUTOMATION_TEST_PRIVATE(CREATE_SERIALIZATION_TEST_NAME(StructType), FAutomationTestBase, CREATE_PRETTY_TEST_NAME(TestCategory, StructType, Serialization), Flags, __FILE__, __LINE__) \
namespace \
{ \
	CREATE_SERIALIZATION_TEST_NAME(StructType) PREPROCESSOR_JOIN(StructType, Serialization)AutomationTestInstance( PREPROCESSOR_TO_STRING( CREATE_SERIALIZATION_TEST_NAME(StructType) ) ); \
}; \
bool CREATE_SERIALIZATION_TEST_NAME(StructType)::RunTest(const FString& Parameters) \
{ \
	return TestStructSerialization(this, StructInstance, TEXT(#StructType)); \
}


/*
Helper macro to create a basic serialization smoke test for a struct.
Example usage: IMPLEMENT_STRUCT_SERIALIZATION_SMOKE_TEST(test.vector, FVector, FVector(1, 2, 3));
*/
#define IMPLEMENT_STRUCT_SERIALIZATION_SMOKE_TEST(TestCategory, StructType, StructInstance) \
IMPLEMENT_STRUCT_SERIALIZATION_TEST_NEW(TestCategory, StructType, StructInstance, EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::SmokeFilter)


#define BEGIN_SIMPLE_TEST(TestCategory, TClass, TestType, Flags) \
IMPLEMENT_SIMPLE_AUTOMATION_TEST_PRIVATE(CREATE_TEST_NAME(TClass, TestType), FAutomationTestBase, CREATE_PRETTY_TEST_NAME(TestCategory, TClass, TestType), Flags, __FILE__, __LINE__) \
namespace \
{ \
	CREATE_TEST_NAME(TClass, TestType) TClass##TestType##AutomationTestInstance( PREPROCESSOR_TO_STRING( CREATE_TEST_NAME(TClass, TestType) ) ); \
}; \
bool CREATE_TEST_NAME(TClass, TestType)::RunTest(const FString& Parameters) \
{

#define END_SIMPLE_TEST }


#define BEGIN_STRUCT_SERIALIZATION_TEST(TestCategory, StructType, Flags) \
SERIALIZATION_ASSERTIONS(StructType) \
IMPLEMENT_SIMPLE_AUTOMATION_TEST_PRIVATE(CREATE_SERIALIZATION_TEST_NAME(StructType), FAutomationTestBase, CREATE_PRETTY_TEST_NAME(TestCategory, StructType, Serialization), Flags, __FILE__, __LINE__) \
namespace \
{ \
	CREATE_SERIALIZATION_TEST_NAME(StructType) PREPROCESSOR_JOIN(StructType, Serialization)##AutomationTestInstance( PREPROCESSOR_JOIN( L, PREPROCESSOR_TO_STRING( CREATE_SERIALIZATION_TEST_NAME(StructType) ) ) ); \
}; \
bool PREPROCESSOR_JOIN(StructType, SerializationTest)::RunTest(const FString& Parameters) \
{ \
	bool bPassed = true;

#define ADD_STRUCT_INSTANCE_TO_SERIALIZATION_TEST(StructType, StructInstance) \
	bPassed &= TestStructSerialization(this, StructInstance, TEXT(#StructType))

#define END_STRUCT_SERIALIZATION_TEST() \
	return bPassed; \
}

#define BEGIN_STRUCT_SERIALIZATION_SMOKE_TEST(TestCategory, StructType) \
BEGIN_STRUCT_SERIALIZATION_TEST(TestCategory, StructType, EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

#endif
