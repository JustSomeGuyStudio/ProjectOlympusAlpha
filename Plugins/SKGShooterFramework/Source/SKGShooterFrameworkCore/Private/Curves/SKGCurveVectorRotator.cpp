// Copyright 2023, Dakota Dawe, All rights reserved


#include "Curves/SKGCurveVectorRotator.h"


FSKGVectorRotator USKGCurveVectorRotator::GetVectorRotatorValue( float InTime ) const
{
	FSKGVectorRotator Result;
	Result.Vector.X = FloatCurves[0].Eval(InTime);
	Result.Vector.Y = FloatCurves[1].Eval(InTime);
	Result.Vector.Z = FloatCurves[2].Eval(InTime);
	Result.Rotator.Pitch = FloatCurves[3].Eval(InTime);
	Result.Rotator.Yaw = FloatCurves[4].Eval(InTime);
	Result.Rotator.Roll = FloatCurves[5].Eval(InTime);
	return Result;
}

FVector USKGCurveVectorRotator::GetVectorValue(float InTime) const
{
	FVector Result;
	Result.X = FloatCurves[0].Eval(InTime);
	Result.Y = FloatCurves[1].Eval(InTime);
	Result.Z = FloatCurves[2].Eval(InTime);
	return Result;
}

FRotator USKGCurveVectorRotator::GetRotatorValue(float InTime) const
{
	FRotator Result;
	Result.Pitch = FloatCurves[3].Eval(InTime);
	Result.Yaw = FloatCurves[4].Eval(InTime);
	Result.Roll = FloatCurves[5].Eval(InTime);
	return Result;
}

static const FName XCurveName(TEXT("X"));
static const FName YCurveName(TEXT("Y"));
static const FName ZCurveName(TEXT("Z"));
static const FName PitchCurveName(TEXT("Pitch"));
static const FName YawCurveName(TEXT("Yaw"));
static const FName RollCurveName(TEXT("Roll"));

TArray<FRichCurveEditInfoConst> USKGCurveVectorRotator::GetCurves() const 
{
	TArray<FRichCurveEditInfoConst> Curves;
	Curves.Add(FRichCurveEditInfoConst(&FloatCurves[0], XCurveName));
	Curves.Add(FRichCurveEditInfoConst(&FloatCurves[1], YCurveName));
	Curves.Add(FRichCurveEditInfoConst(&FloatCurves[2], ZCurveName));
	Curves.Add(FRichCurveEditInfoConst(&FloatCurves[3], PitchCurveName));
	Curves.Add(FRichCurveEditInfoConst(&FloatCurves[4], YawCurveName));
	Curves.Add(FRichCurveEditInfoConst(&FloatCurves[5], RollCurveName));
	return Curves;
}

TArray<FRichCurveEditInfo> USKGCurveVectorRotator::GetCurves() 
{
	TArray<FRichCurveEditInfo> Curves;
	Curves.Add(FRichCurveEditInfo(&FloatCurves[0], XCurveName));
	Curves.Add(FRichCurveEditInfo(&FloatCurves[1], YCurveName));
	Curves.Add(FRichCurveEditInfo(&FloatCurves[2], ZCurveName));
	Curves.Add(FRichCurveEditInfo(&FloatCurves[3], PitchCurveName));
	Curves.Add(FRichCurveEditInfo(&FloatCurves[4], YawCurveName));
	Curves.Add(FRichCurveEditInfo(&FloatCurves[5], RollCurveName));
	return Curves;
}

bool USKGCurveVectorRotator::operator==( const USKGCurveVectorRotator& Curve ) const
{
	return (FloatCurves[0] == Curve.FloatCurves[0]) && (FloatCurves[1] == Curve.FloatCurves[1]) && (FloatCurves[2] == Curve.FloatCurves[2]) && (FloatCurves[3] == Curve.FloatCurves[3]) && (FloatCurves[4] == Curve.FloatCurves[4]) && (FloatCurves[5] == Curve.FloatCurves[5]);
}

bool USKGCurveVectorRotator::IsValidCurve( FRichCurveEditInfo CurveInfo )
{
	return CurveInfo.CurveToEdit == &FloatCurves[0] ||
		CurveInfo.CurveToEdit == &FloatCurves[1] ||
		CurveInfo.CurveToEdit == &FloatCurves[2] ||
		CurveInfo.CurveToEdit == &FloatCurves[3] ||
		CurveInfo.CurveToEdit == &FloatCurves[4] ||
		CurveInfo.CurveToEdit == &FloatCurves[5];
}