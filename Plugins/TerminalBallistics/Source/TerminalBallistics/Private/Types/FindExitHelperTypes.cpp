#include "Types/FindExitHelperTypes.h"

namespace TB
{
	bool FExitResult::HasValidPhysicalMaterial() const
	{
		const bool IsNull = HitResult.PhysMaterial.IsExplicitlyNull() || ExitHitResult.PhysMaterial.IsExplicitlyNull();
		if (IsNull)
		{
			return false;
		}
		else
		{
			const bool IsStale = HitResult.PhysMaterial.IsStale(true, true) || ExitHitResult.PhysMaterial.IsStale(true, true);
			if (IsStale)
			{
				return false;
			}
			UPhysicalMaterial* PhysMat = HitResult.PhysMaterial.Get();
			UPhysicalMaterial* ExitPhysMat = ExitHitResult.PhysMaterial.Get();
			bool IsValid = PhysMat && ExitPhysMat;
			if (!IsValid)
			{
				return false;
			}
			IsValid = PhysMat != GEngine->DefaultPhysMaterial.Get();
			IsValid &= ExitPhysMat != GEngine->DefaultPhysMaterial.Get();
			return IsValid;
		}
	}

	bool FExitResult::CanCombine(const FExitResult& Other) const
	{
		if (Component != Other.Component)
		{
			return false;
		}
		if (FVector::PointsAreNear(ExitLocation, Other.ImpactPoint, 5e-3))
		{
			return true;
		}

		return false;
	}

	bool FExitResult::Combine(const FExitResult& Other)
	{
		if (!CanCombine(Other))
		{
			return false;
		}
		else
		{
			ExitHitResult = Other.ExitHitResult;
			CalcExitVarsFromExitResult();
			return true;
		}
	}

	void FExitResults::TryShrink()
	{
		int i = 0;
		while (ExitResults.IsValidIndex(i + 1))
		{
			FExitResult& Current = ExitResults[i];
			if (Current.Combine(ExitResults[i + 1]))
			{
				ExitResults.RemoveAt(i + 1);
			}
			else
			{
				i++;
			}
		}
		Shrink();
	}
};