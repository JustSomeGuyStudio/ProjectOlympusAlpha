// Copyright 2023, Erik Hedberg. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "TerminalBallisticsMacrosAndFunctions.h"

static bool GetRandBool()
{
	int r = std::rand();
	return r % 2 == 0;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPackArchiveTest, "Terminal Ballistics.Utility.Pack Archive", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FPackArchiveTest::RunTest(const FString& Parameters)
{
	bool bPassed = true;

	static const int NUM_BOOLS = 36;

	TArray<bool> bools;
	bools.SetNum(NUM_BOOLS);
	for (int i = 0; i < NUM_BOOLS; i++)
	{
		bools[i] = GetRandBool();
	}

	using namespace TB::BitPackHelpers;
	TArray<uint8> Data = {};
	FMemoryWriter WriteAr = FMemoryWriter(Data, false);
	PackArchive<1>(
		WriteAr,
		&bools[0]
	);
	PackArchive<2>(
		WriteAr,
		&bools[1],
		&bools[2]
	);
	PackArchive<3>(
		WriteAr,
		&bools[3],
		&bools[4],
		&bools[5]
	);
	PackArchive<4>(
		WriteAr,
		&bools[6],
		&bools[7],
		&bools[8],
		&bools[9]
	);
	PackArchive<5>(
		WriteAr,
		&bools[10],
		&bools[11],
		&bools[12],
		&bools[13],
		&bools[14]
	);
	PackArchive<6>(
		WriteAr,
		&bools[15],
		&bools[16],
		&bools[17],
		&bools[18],
		&bools[19],
		&bools[20]
	);
	PackArchive<7>(
		WriteAr,
		&bools[21],
		&bools[22],
		&bools[23],
		&bools[24],
		&bools[25],
		&bools[26],
		&bools[27]
	);
	PackArchive<8>(
		WriteAr,
		&bools[28],
		&bools[29],
		&bools[30],
		&bools[31],
		&bools[32],
		&bools[33],
		&bools[34],
		&bools[35]
	);

	FMemoryReader ReadAr = FMemoryReader(Data, false);
	TArray<bool> boolsLoaded;
	boolsLoaded.SetNum(NUM_BOOLS);
	PackArchive<1>(
		ReadAr,
		&boolsLoaded[0]
	);
	PackArchive<2>(
		ReadAr,
		&boolsLoaded[1],
		&boolsLoaded[2]
	);
	PackArchive<3>(
		ReadAr,
		&boolsLoaded[3],
		&boolsLoaded[4],
		&boolsLoaded[5]
	);
	PackArchive<4>(
		ReadAr,
		&boolsLoaded[6],
		&boolsLoaded[7],
		&boolsLoaded[8],
		&boolsLoaded[9]
	);
	PackArchive<5>(
		ReadAr,
		&boolsLoaded[10],
		&boolsLoaded[11],
		&boolsLoaded[12],
		&boolsLoaded[13],
		&boolsLoaded[14]
	);
	PackArchive<6>(
		ReadAr,
		&boolsLoaded[15],
		&boolsLoaded[16],
		&boolsLoaded[17],
		&boolsLoaded[18],
		&boolsLoaded[19],
		&boolsLoaded[20]
	);
	PackArchive<7>(
		ReadAr,
		&boolsLoaded[21],
		&boolsLoaded[22],
		&boolsLoaded[23],
		&boolsLoaded[24],
		&boolsLoaded[25],
		&boolsLoaded[26],
		&boolsLoaded[27]
	);
	PackArchive<8>(
		ReadAr,
		&boolsLoaded[28],
		&boolsLoaded[29],
		&boolsLoaded[30],
		&boolsLoaded[31],
		&boolsLoaded[32],
		&boolsLoaded[33],
		&boolsLoaded[34],
		&boolsLoaded[35]
	);

	bool bArraysAreEqual = bools == boolsLoaded;
	const int32 PackedSize = Data.Num();

	Data.Empty();
	WriteAr.Seek(0);
	WriteAr << bools;
	const int32 UnpackedSize = Data.Num();


	TestTrue("PackArchive correctly serializes/deserializes up to 8 values", bArraysAreEqual);
	AddInfo(FString::Printf(TEXT("Archive Sizes:\n\tUnacked: %d\n\tPacked: %d"), UnpackedSize, PackedSize));

	ReadAr.Seek(0);
	WriteAr.Seek(0);
	Data.Empty();


	bool bTemp1 = GetRandBool();
	bool bTemp2 = GetRandBool();
	bool bTemp3 = GetRandBool();
	bool bTemp4 = GetRandBool();
	bool bTemp5 = GetRandBool();
	bool bTemp6 = GetRandBool();
	bool bTemp7 = GetRandBool();
	bool bTemp8 = GetRandBool();
	const bool b1 = bTemp1;
	const bool b2 = bTemp2;
	const bool b3 = bTemp3;
	const bool b4 = bTemp4;
	const bool b5 = bTemp5;
	const bool b6 = bTemp6;
	const bool b7 = bTemp7;
	const bool b8 = bTemp8;

	bool bDeserializedCorrectly = true;

	auto CheckIdentical = [&]()
	{
		bDeserializedCorrectly &= bTemp1 == b1
			&& bTemp2 == b2
			&& bTemp3 == b3
			&& bTemp4 == b4
			&& bTemp5 == b5
			&& bTemp6 == b6
			&& bTemp7 == b7
			&& bTemp8 == b8;
	};

	TB_PACK_ARCHIVE_WITH_BITFIELDS_ONE(WriteAr, bTemp1);
	TB_PACK_ARCHIVE_WITH_BITFIELDS_ONE(ReadAr, bTemp1);
	CheckIdentical();
	TB_PACK_ARCHIVE_WITH_BITFIELDS_TWO(WriteAr, bTemp1, bTemp2);
	TB_PACK_ARCHIVE_WITH_BITFIELDS_TWO(ReadAr, bTemp1, bTemp2);
	CheckIdentical();
	TB_PACK_ARCHIVE_WITH_BITFIELDS_THREE(WriteAr, bTemp1, bTemp2, bTemp3);
	TB_PACK_ARCHIVE_WITH_BITFIELDS_THREE(ReadAr, bTemp1, bTemp2, bTemp3);
	CheckIdentical();
	TB_PACK_ARCHIVE_WITH_BITFIELDS_FOUR(WriteAr, bTemp1, bTemp2, bTemp3, bTemp4);
	TB_PACK_ARCHIVE_WITH_BITFIELDS_FOUR(ReadAr, bTemp1, bTemp2, bTemp3, bTemp4);
	CheckIdentical();
	TB_PACK_ARCHIVE_WITH_BITFIELDS_FIVE(WriteAr, bTemp1, bTemp2, bTemp3, bTemp4, bTemp5);
	TB_PACK_ARCHIVE_WITH_BITFIELDS_FIVE(ReadAr, bTemp1, bTemp2, bTemp3, bTemp4, bTemp5);
	CheckIdentical();
	TB_PACK_ARCHIVE_WITH_BITFIELDS_SIX(WriteAr, bTemp1, bTemp2, bTemp3, bTemp4, bTemp5, bTemp6);
	TB_PACK_ARCHIVE_WITH_BITFIELDS_SIX(ReadAr, bTemp1, bTemp2, bTemp3, bTemp4, bTemp5, bTemp6);
	CheckIdentical();
	TB_PACK_ARCHIVE_WITH_BITFIELDS_SEVEN(WriteAr, bTemp1, bTemp2, bTemp3, bTemp4, bTemp5, bTemp6, bTemp7);
	TB_PACK_ARCHIVE_WITH_BITFIELDS_SEVEN(ReadAr, bTemp1, bTemp2, bTemp3, bTemp4, bTemp5, bTemp6, bTemp7);
	CheckIdentical();
	TB_PACK_ARCHIVE_WITH_BITFIELDS_EIGHT(WriteAr, bTemp1, bTemp2, bTemp3, bTemp4, bTemp5, bTemp6, bTemp7, bTemp8);
	TB_PACK_ARCHIVE_WITH_BITFIELDS_EIGHT(ReadAr, bTemp1, bTemp2, bTemp3, bTemp4, bTemp5, bTemp6, bTemp7, bTemp8);
	CheckIdentical();

	TestTrue("PACK_ARCHIVE_WITH_BITFIELDS macro works as expected.", bDeserializedCorrectly);

	return bPassed;
}

#endif
