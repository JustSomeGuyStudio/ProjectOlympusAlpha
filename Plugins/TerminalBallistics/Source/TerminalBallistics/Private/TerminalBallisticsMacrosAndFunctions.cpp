// Copyright 2023, Erik Hedberg. All Rights Reserved.

#include "TerminalBallisticsMacrosAndFunctions.h"
#include "Engine/HitResult.h"

namespace TB
{
	bool HitResultsAreEqual(const FHitResult& A, const FHitResult& B)
	{
		return A.TraceStart.Equals(B.TraceStart)
			&& A.TraceEnd.Equals(B.TraceEnd)
			&& A.ImpactPoint.Equals(B.ImpactPoint)
			&& A.HitObjectHandle == B.HitObjectHandle
			&& A.Component == B.Component;
	}

	bool HitResultsAreEqualStrict(const FHitResult& A, const FHitResult& B, bool bCheckStartAndEndAreSame, bool bCheckComponentsAreSame, bool bCheckLocationsAreSame, bool bCheckNormalsAreSame, bool bCheckFaceIndiciesAreSame, double Tolerance)
	{
		const bool AHasValidHitObjectHandle = A.HasValidHitObjectHandle();
		const bool BHasValidHitObjectHandle = B.HasValidHitObjectHandle();

		bool bHandlesAreEqual = false;

		if (AHasValidHitObjectHandle != BHasValidHitObjectHandle) // If one handle is invalid but the other isn't, we know they are different.
		{
			return false;
		}
		else
		{
			if (IsInGameThread() || IsInParallelGameThread()) // If we're in the game thread, we can safely compare the object handles
			{
				bHandlesAreEqual = A.HitObjectHandle == B.HitObjectHandle;
			}
			else // Otherwise, we need to get creative
			{
				if (A.GetComponent() && B.GetComponent())
				{
					bHandlesAreEqual = A.GetComponent()->GetOwner() == B.GetComponent()->GetOwner();
				}
				else if (!A.GetComponent() && !B.GetComponent())
				{
					bHandlesAreEqual = true;
				}
			}
		}

		if (!bHandlesAreEqual)
		{
			return false;
		}

		bool bStartAndEndAreSame = true;
		bool bComponentsAreSame = true;
		bool bLocationsAreSame = true;
		bool bNormalsAreSame = true;
		bool bFaceIndiciesAreSame = true;

		if (bCheckStartAndEndAreSame)
		{
			bStartAndEndAreSame =
				A.TraceStart.Equals(B.TraceStart, Tolerance)
				&& A.TraceEnd.Equals(B.TraceEnd, Tolerance);
		}
		if (bCheckComponentsAreSame)
		{
			bComponentsAreSame = A.Component == B.Component;
		}
		if (bCheckLocationsAreSame)
		{
			bLocationsAreSame = A.ImpactPoint.Equals(B.ImpactPoint, Tolerance);
		}
		if (bCheckNormalsAreSame)
		{
			bNormalsAreSame = A.ImpactNormal.Equals(B.ImpactNormal, Tolerance);
		}
		if (bCheckFaceIndiciesAreSame)
		{
			bFaceIndiciesAreSame = A.FaceIndex == B.FaceIndex;
		}

		return bStartAndEndAreSame && bComponentsAreSame && bLocationsAreSame && bNormalsAreSame && bFaceIndiciesAreSame;
	}
}
