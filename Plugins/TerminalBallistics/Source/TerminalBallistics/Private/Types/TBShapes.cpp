// Copyright 2023, Erik Hedberg. All Rights Reserved.

#include  "Types/TBShapes.h"
#include "MathUtils.h"

double FTBCylinder::GetCSA(const bool bForceRecalculate) const
{
	if (CSA <= 0 || bForceRecalculate)
	{
		const double RadiusMeters = Radius / 100.0;
		const double LengthMeters = Length / 100.0;
		CSA = RadiusMeters * LengthMeters;
	}
	return CSA;
}

double FTBCylinder::GetVolume(const bool bForceRecalculate) const
{
	if (Volume <= 0 || bForceRecalculate)
	{
		Volume = TB::MathUtils::sqr(Radius / 100) * UE_DOUBLE_PI * (Length / 100);
	}
	return Volume;
}

double FTBCylinder::GetCD() const
{
	return 1.17;
}

double FTBCylinder::GetFrontalCd() const
{
	return 1.15;
}

FVector FTBCylinder::GetInertia(const double Mass) const
{
	constexpr double OneTwelveth = 1.0 / 12.0;
	const double rSqr = TB::MathUtils::sqr(Radius/100);
	const double hSqr = TB::MathUtils::sqr(Length/100);
	const double Ix = OneTwelveth * Mass * (3.0 * rSqr + hSqr);
	const double Iy = Ix;
	const double Iz = 0.5 * Mass * rSqr;
	return FVector(Ix, Iy, Iz);
}

void FTBCylinder::RecalculateValues()
{
	GetCSA(true);
	GetVolume(true);
}



double FTBEllipticalNose::GetShapeLength() const
{
	return MajorRadius;
}

double FTBEllipticalNose::GetShapeRadius(const double x) const
{
	if (x <= 0.0)
	{
		return 0.0;
	}
	if (x >= MajorRadius)
	{
		return MinorRadius;
	}
	
	using namespace TB::MathUtils;
	const double R = MinorRadius;
	const double L = MajorRadius;
	const double InsideRoot = 1.0 - (sqr(L - x) / sqr(L));
	return R * sqrt(InsideRoot);
}

double FTBEllipticalNose::GetShapeVolume(const bool bForceRecalculate) const
{
	if(Volume <= 0 || bForceRecalculate)
	{
		/**
		* Volume of an prolate spheroid : 4/3 pi * major * minor^2
		* Since this is a prolate hemispheroid, we can simplify it to 2/3 pi * major * minor^2
		*/
		static constexpr double TwoThirdsPi = UE_DOUBLE_PI * (2.0 / 3.0);
		Volume = TwoThirdsPi * (MajorRadius / 100.0) * TB::MathUtils::sqr(MinorRadius / 100.0);
	}
	return Volume;
}

double FTBEllipticalNose::GetShapeSurfaceArea(const bool bForceRecalculate) const
{
	if(SurfaceArea <= 0 || bForceRecalculate)
	{
		using namespace TB::MathUtils;
		const double a = MinorRadius;
		const double c = MajorRadius;
		const double e = sqrt(1.0 - (sqr(a) / sqr(c)));
		const double cOver_ae = c / (a * e);
		SurfaceArea = UE_DOUBLE_PI * sqr(a) * (1.0 + cOver_ae * FMath::Asin(e)); // Half the surface area of a prolate spheroid
		SurfaceArea /= 10000.0; // cm^2 to m^2
	}
	return SurfaceArea;
}

double FTBEllipticalNose::GetShapeFrontalCSA() const
{
	return UE_DOUBLE_PI * TB::MathUtils::sqr(MinorRadius);
}



double FTBOgive::GetSurfaceArea(const bool bForceRecalculate) const
{
	const double CylinderSurfaceArea = 2.0 * UE_DOUBLE_PI * TB::MathUtils::sqr(Cylinder.Radius / 100.0) * (Cylinder.Length / 100.0);
	return GetShapeSurfaceArea() + CylinderSurfaceArea;
}

double FTBOgive::GetShapeSurfaceArea(const bool bForceRecalculate) const
{
	if (SurfaceArea <= 0 || bForceRecalculate)
	{
		// Calculate the surface area of the ogive. (Ignoring the end caps)
		const double dx = Length / 1000.0;
		double x = dx;
		SurfaceArea = 0.0;
		while (x < Length)
		{
			double r1 = GetOgiveRadius(x - dx);
			double r2 = GetOgiveRadius(x);
			double h = sqrt((r2 - r1) * (r2 - r1) + dx * dx);
			double segmentArea = UE_DOUBLE_PI * (r1 + r2) * h;
			SurfaceArea += segmentArea;
			x += dx;
		}
		SurfaceArea /= 10000.0; // cm^2 to m^2
	}
	return SurfaceArea;
}

double FTBOgive::GetAlpha(const bool bForceRecalculate) const
{
	if (alpha <= 0 || bForceRecalculate)
	{
		using namespace TB::MathUtils;
		const double R = Cylinder.Radius;
		const double cosNumerator = sqrt(sqr(Length) + sqr(R));
		alpha = acos(cosNumerator / (2 * OgiveRadius)) - atan(R / Length);
	}
	return alpha;
}

double FTBOgive::GetSinAlpha(const bool bForceRecalculate) const
{
	if (sinAlpha <= 0 || bForceRecalculate)
	{
		sinAlpha = sin(GetAlpha(bForceRecalculate));
	}
	return sinAlpha;
}

double FTBOgive::GetCosAlpha(const bool bForceRecalculate) const
{
	if (cosAlpha <= 0 || bForceRecalculate)
	{
		cosAlpha = cos(GetAlpha(bForceRecalculate));
	}
	return cosAlpha;
}

double FTBOgive::CalculateOgiveRadius()
{
	using namespace TB::MathUtils;
	return (sqr(Radius) + sqr(Length)) / (2.0 * Radius);
}

double FTBOgive::GetOgiveRadius(const double x) const
{
	if (x < 0 || x > Length)
	{
		if (x > Length)
		{
			return Cylinder.Radius;
		}
		return -1;
	}

	using namespace TB::MathUtils;
	const double R = Cylinder.Radius;
	const double sqrOgiveRadius = sqr(OgiveRadius);
	if (IsTangentOgive())
	{
		const double root = sqrt(sqrOgiveRadius - sqr(x));
		return root + R - OgiveRadius;
	}
	else
	{
		return sqrt(sqrOgiveRadius - sqr(OgiveRadius * GetCosAlpha() - x)) - OgiveRadius * GetSinAlpha();
	}
}

double FTBOgive::GetSlope(const double x, const bool IsTop) const
{
	const double SideMult = IsTop ? 1.0 : -1.0;
	if (x < 0 || x > Length)
	{
		if (x > Length)
		{
			return SideMult;
		}
		else
		{
			return 0.0;
		}
	}

	using namespace TB::MathUtils;
	const double delta = Length / 1e4;
	const double R1 = GetOgiveRadius(x);
	const double R2 = GetOgiveRadius(x + delta);
	double dR = R1 - R2;
	if (dR == 0)
	{
		dR += 1e-6;
	}
	return SideMult * dR;
}

double FTBOgive::GetTangentAngle(const double x, const bool IsTop) const
{
	return atan(GetSlope(x, IsTop));
}

double FTBOgive::GetFrontalCSA(const bool bForceRecalculate) const
{
	if (FrontalCSA <= 0 || bForceRecalculate)
	{
		FrontalCSA = TB::MathUtils::sqr(Radius / 100.0);
	}
	return FrontalCSA;
}

double FTBOgive::GetCD(const bool bForceRecalculate) const
{
	if (Cd <= 0 || bForceRecalculate)
	{
		const double OgiveWeight = CSA / (CSA + Cylinder.GetCSA());
		const double CylinderWeight = Cylinder.GetCSA() / (CSA + Cylinder.GetCSA());
		Cd = (OgiveWeight * GetOgiveCd()) + (CylinderWeight * Cylinder.GetCD());
	}
	return Cd;
}

double FTBOgive::GetShapeVolume(const bool bForceRecalculate) const
{
	if (Volume <= 0 || bForceRecalculate)
	{
		using namespace TB::MathUtils;
		Volume = 0.0;

		constexpr double OneThirdPi = UE_DOUBLE_PI / 3.0;

		const double dx = Length / 1000.0;
		double x = dx;
		double radius = 0.0;
		double prevRadius = GetOgiveRadius(0.0) / 100.0; // cm to m
		double sqrPrevRadius = sqr(prevRadius);
		while (x < Length)
		{
			radius = GetOgiveRadius(x) / 100.0; // cm to m
			const double sqrRadius = sqr(radius);
			Volume += OneThirdPi * dx * (sqrPrevRadius + radius * prevRadius + sqrRadius);
			prevRadius = radius;
			sqrPrevRadius = sqrRadius;
			x += dx;
		}
	}
	return Volume;
}

double FTBOgive::GetCenterOfMass() const
{
	const double f = OgiveRadius / GetRadius();
	const double lambda = sqrt(2 * f - 1);

	const double fSqr = f * f;
	const double fMinusOneSqr = pow(f - 1, 2);

	const double zTerm1 = -(2.0 / 3.0) * (f - 1) * (pow(f, 3.0) - pow(f - 1, 3.0));
	const double zTerm2 = 0.5 * (fSqr + fMinusOneSqr) * pow(lambda, 2.0);
	const double zTerm3 = -0.25 * pow(lambda, 4.0);
	const double V = UE_DOUBLE_PI * ((fSqr - (1.0 / 3.0) * pow(lambda, 2.0)) * lambda + fSqr * (f - 1.0) * asin(lambda / f));
	const double Z = (UE_DOUBLE_PI / V) * (zTerm1 + zTerm2 + zTerm3);
	return Z;
}

double FTBOgive::GetCenterOfMassFull(const double Mass) const
{
	const double OgiveVolume = GetShapeVolume();
	const double CylinderVolume = Cylinder.GetVolume();
	const double Density = Mass / (OgiveVolume + CylinderVolume);
	const double OgiveMass = Density * OgiveVolume;
	const double CylinderMass = Density * CylinderVolume;
	const double OgiveCoM = GetCenterOfMass();
	const double CylinderCoM = Cylinder.GetCenterOfMass();
	const double CoM = (OgiveMass * OgiveCoM) / OgiveMass + (CylinderMass * CylinderCoM) / CylinderMass;
	return CoM;
}

FVector FTBOgive::GetOgiveInertia(const double Mass) const
{
	const double f = OgiveRadius / GetRadius();
	const double lambda = sqrt(2 * f - 1);

	constexpr double b3 = 1.0;
	const double b2 = 3 + (7.0 / 5.0) * b3 * f;
	const double b1 = -3 + (5.0 / 4.0) * b2 * f;
	const double b0 = 1 + b1 * f;

	const double fSqr = f * f;
	const double asinLambdaOverF = asin(lambda / f);

	const double Izz = UE_DOUBLE_PI * ((fSqr * asinLambdaOverF - (f - 1) * lambda) * b0 - 2 * (b1/3.0 + b2/4.0 + b3/5.0) * pow(lambda, 3.0));

	const double fMinusOneSqr = pow(f - 1, 2);

	const double zTerm1 = -(2.0 / 3.0) * (f - 1) * (pow(f, 3.0) - pow(f - 1, 3.0));
	const double zTerm2 = 0.5 * (fSqr + fMinusOneSqr) * pow(lambda, 2.0);
	const double zTerm3 = -0.25 * pow(lambda, 4.0);
	const double V = UE_DOUBLE_PI * ((fSqr - (1.0 / 3.0) * pow(lambda, 2.0)) * lambda + fSqr * (f - 1.0) * asinLambdaOverF);
	const double Z = (UE_DOUBLE_PI / V) * (zTerm1 + zTerm2 + zTerm3);


	const double Term1 = fSqr * (fSqr + 7.0 / 2.0 * fMinusOneSqr) * lambda;
	const double Term2 = (1.0 / 15.0) * pow(lambda, 5.0);
	const double Term3 = -fSqr * (f - 1) * ((5.0 / 2.0) * fSqr + 2 * fMinusOneSqr) * asinLambdaOverF;
	const double Ixx_base = (UE_DOUBLE_PI / 4.0) * (Term1 + Term2 + Term3);

	const double Ixx_com = Ixx_base - V * pow(Z, 2.0);
	

	return FVector(Ixx_com, Ixx_com, Izz);
}

FVector FTBOgive::GetFullInertia(const double Mass) const
{
	const double OgiveVolume = GetShapeVolume();
	const double CylinderVolume = Cylinder.GetVolume();
	const double Density = Mass / (OgiveVolume + CylinderVolume);
	const double OgiveMass = Density * OgiveVolume;
	const double CylinderMass = Density * CylinderVolume;

	FVector OgiveInertia = GetOgiveInertia(OgiveMass);
	FVector CylinderIntertia = Cylinder.GetInertia(CylinderMass);

	const double OgiveCoM = GetCenterOfMass();
	const double CylinderCoM = Cylinder.GetCenterOfMass();
	const double CoM = (OgiveMass * OgiveCoM) / OgiveMass + (CylinderMass * CylinderCoM) / CylinderMass;

	OgiveInertia = OgiveInertia - OgiveVolume * pow(OgiveCoM - CoM, 2.0);
	CylinderIntertia = CylinderIntertia - CylinderVolume * pow(CylinderCoM - CoM, 2.0);

	return OgiveInertia + CylinderIntertia;

	/*const double OgiveLengthMeters = Length / 100.0;
	const double CylinderLengthMeters = Cylinder.Length / 100.0;
	const double TotalLengthMeters = OgiveLengthMeters + CylinderLengthMeters;
	const double LengthDifference = TotalLengthMeters - OgiveLengthMeters;

	const double RadiusSqr = TB::MathUtils::sqr(GetRadiusMeters());
	const double OgiveLengthSqr = TB::MathUtils::sqr(OgiveLengthMeters);

	const double Ixx = (Mass / 12.0) * (3.0 * RadiusSqr + GetFullVolume() / TotalLengthMeters);
	const double Iyy = (Mass / 12.0) * (3.0 * RadiusSqr + GetFullVolume() / TotalLengthMeters + 4.0 * OgiveLengthSqr);
	const double Izz = 0.5 * Mass * RadiusSqr;

	FMatrix InertiaTensor;
	InertiaTensor.SetIdentity();
	InertiaTensor.M[0][0] = Ixx;
	InertiaTensor.M[1][1] = Iyy;
	InertiaTensor.M[2][2] = Izz;

	return InertiaTensor;*/
}


double FTBNose::GetLength() const
{
	return std::visit([](auto&& Shape) { return Shape.GetShapeLength(); }, ShapeVariant);
}

double FTBNose::GetRadius() const
{
	return std::visit([](auto&& Shape) { return Shape.GetShapeRadius(); }, ShapeVariant);
}

double FTBNose::GetRadiusAt(const double x) const
{
	return std::visit([&](auto&& Shape) { return Shape.GetShapeRadius(x); }, ShapeVariant);
}

double FTBNose::GetSlopeAt(const double x) const
{
	if (x < 0 || x > GetLength())
	{
		if (x > GetLength())
		{
			return 1.0;
		}
		else
		{
			return 0.0;
		}
	}

	using namespace TB::MathUtils;
	const double delta = GetLength() / 1e4;
	const double R1 = GetRadiusAt(x);
	const double R2 = GetRadiusAt(x + delta);
	double dR = R1 - R2;
	if (dR == 0)
	{
		dR += 1e-6;
	}
	return dR;
}

double FTBNose::GetAngleAt(const double x) const
{
	return atan(GetSlopeAt(x));
}

double FTBNose::GetCSA() const
{
	return std::visit([](auto&& Shape) { return Shape.GetShapeCSA(); }, ShapeVariant);
}

double FTBNose::GetFrontalCSA() const
{
	return std::visit([](auto&& Shape) { return Shape.GetShapeFrontalCSA(); }, ShapeVariant);
}

double FTBNose::GetPerpendicularCd() const
{
	return std::visit([](auto&& Shape) { return Shape.GetShapeCd(); }, ShapeVariant);
}

double FTBNose::GetSurfaceArea(const bool bForceRecalculate) const
{
	return std::visit([bForceRecalculate](auto&& Shape) { return Shape.GetShapeVolume(bForceRecalculate); }, ShapeVariant);
}

double FTBNose::GetVolume(const bool bForceRecalculate) const
{
	return std::visit([bForceRecalculate](auto&& Shape) { return Shape.GetShapeVolume(bForceRecalculate); }, ShapeVariant);
}

void FTBNose::RecalculateValues()
{
	std::visit([](auto&& Shape) { Shape.RecalculateValues(); }, ShapeVariant);
	GetSurfaceArea(true);
	GetVolume(true);
}



bool FTBBulletShape::IsOgive() const
{
	bIsOgive = std::holds_alternative<FTBOgive>(Nose.ShapeVariant);
	return bIsOgive;
}

double FTBBulletShape::GetLength() const
{
	return Nose.GetLength() + Cylinder.Length;
}

double FTBBulletShape::GetRadius() const
{
	return fmax(Nose.GetRadius(), Cylinder.Radius);
}

double FTBBulletShape::GetRadiusAt(const double x) const
{
	if (x < Nose.GetLength())
	{
		return 0.0;
	}
	else if (x > Nose.GetLength())
	{
		return Cylinder.Radius;
	}
	else
	{
		return Nose.GetRadiusAt(x);
	}
}

double FTBBulletShape::GetVolume(const bool bForceRecalculate) const
{
	return Nose.GetVolume() + Cylinder.GetVolume();
}

double FTBBulletShape::GetSurfaceArea() const
{
	const double CylinderSurfaceArea = 2.0 * UE_DOUBLE_PI * TB::MathUtils::sqr(Cylinder.Radius / 100.0) * (Cylinder.Length / 100.0);
	return Nose.GetSurfaceArea() + CylinderSurfaceArea;
}

double FTBBulletShape::GetRadiusMeters() const
{
	return fmax(GetRadius(), Cylinder.Radius / 100.0);
}

double FTBBulletShape::GetCD(const bool bForceRecalculate) const
{
	const double ShapeCd = Nose.GetPerpendicularCd();
	const double ShapeWeight = Nose.GetCSA() / (Nose.GetCSA() + Cylinder.GetCSA());
	const double CylinderWeight = Cylinder.GetCSA() / (Nose.GetCSA() + Cylinder.GetCSA());
	return (ShapeWeight * ShapeCd) + (CylinderWeight * Cylinder.GetCD());
}

double FTBBulletShape::GetSlope(const double x, const bool IsTop) const
{
	const double SideMult = IsTop ? 1.0 : -1.0;
	if (x < 0 || x > Nose.GetLength())
	{
		if (x > Nose.GetLength())
		{
			return SideMult;
		}
		else
		{
			return 0.0;
		}
	}

	using namespace TB::MathUtils;
	const double delta = Nose.GetLength() / 1e4;
	const double R1 = Nose.GetRadiusAt(x);
	const double R2 = Nose.GetRadiusAt(x + delta);
	double dR = R1 - R2;
	if (dR == 0)
	{
		dR += 1e-6;
	}
	return SideMult * dR;
}

double FTBBulletShape::GetTangentAngle(const double x, const bool IsTop) const
{
	return atan(GetSlope(x, IsTop));
}

double FTBBulletShape::GetCSA() const
{
	return Nose.GetCSA() + Cylinder.GetCSA();
}

void FTBBulletShape::RecalculateValues()
{
	Nose.RecalculateValues();
	Cylinder.RecalculateValues();
}

double FTBBulletShape::GetFrontalCSA() const
{
	const double ShapeFrontalCSA = Nose.GetFrontalCSA();
	const double CylinderFrontalCSA = UE_DOUBLE_PI * TB::MathUtils::sqr(Cylinder.Radius / 100.0);
	return fmax(ShapeFrontalCSA, CylinderFrontalCSA);
}
