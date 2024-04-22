// Copyright 2023, Erik Hedberg. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/GameplayStaticsTypes.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "TerminalBallisticsConfiguration.h"
#include "TerminalBallisticsMacrosAndFunctions.h"
#include "TerminalBallisticsTraits.h"
#include "Threading/TBProjectileTaskResult.h"
#include "Types/ImpactParams.h"
#include "Types/TBEnums.h"
#include "Types/TBProjectileFlightData.h"
#include "Types/TBProjectile.h"
#include "Types/TBProjectileId.h"
#include "Types/TBProjectileInjury.h"

#include "TBSimData.generated.h"


class UNiagaraComponent;
class UNiagaraSystem;
class UTerminalBallisticsSubsystem;

typedef TSharedPtr<struct FTBBullet> BulletPointer;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnComplete, const FTBProjectileId&, Id, const TArray<FPredictProjectilePathPointData>&, Results);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUpdate, const FTBProjectileFlightData&, FlightData);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBulletHit, const FTBImpactParams&, ImpactParams);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBulletExitHit, const FTBImpactParams&, ImpactParams);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBulletInjure, const FTBImpactParams&, ImpactParams, const FTBProjectileInjuryParams&, InjuryParams);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectileHit, const FTBImpactParamsBasic&, ImpactParams);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectileExitHit, const FTBImpactParamsBasic&, ImpactParams);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectileInjure, const FTBImpactParamsBasic&, ImpactParams, const FTBProjectileInjuryParams&, InjuryParams);


USTRUCT(BlueprintInternalUseOnly)
struct TERMINALBALLISTICS_API FTBSimData
{
	GENERATED_BODY()
public:
	template<typename T = FTBImpactParamsBasic>
	using ImpactStruct = T;

	UPROPERTY(BlueprintAssignable, Category = SimData)
	FOnComplete onComplete;
	UPROPERTY(BlueprintAssignable, Category = SimData)
	FOnUpdate OnUpdateDelegate;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = SimData)
	double MaxSimTime = 0.0;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = SimData)
	double Timescale = 1.0;

	UPROPERTY(BlueprintReadWrite, Category = "Launch Params", AdvancedDisplay, meta = (Units = "meters"))
	double OwnerIgnoreDistance = 10.0;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = SimData)
	int MaxIterations;
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = SimData)
	int UpdateBroadcastInterval = TB::Configuration::DefaultThreadUpdateCallbackInterval;
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = SimData)
	ETBDragComplexity DragCalculationType = ETBDragComplexity::DEFAULT;
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = SimData)
	ETBProjectileSimulationType SimType = ETBProjectileSimulationType::Default;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = SimData)
	uint8 bDrawDebugTrace : 1;
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = SimData)
	uint8 bPrintDebugInfo : 1;

	bool IsTestTask : 1;

	bool HasBeenAssignedId : 1;
	bool DefaultDelegatesAreBound : 1;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = SimData)
	FPredictProjectilePathParams PredictParams;
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = SimData)
	TObjectPtr<UNiagaraComponent> TracerComponent = nullptr;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = SimData)
	TObjectPtr<UNiagaraSystem> TracerSystem = nullptr;

protected:
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = SimData)
	FTBProjectileId Id = FTBProjectileId::None;

public:
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = SimData)
	FVector StartLocation = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = SimData)
	FVector StartVelocity = FVector::ZeroVector;

	FCollisionQueryParams QueryParams;
	FCollisionObjectQueryParams ObjQueryParams;

	UTerminalBallisticsSubsystem* TerminalBallisticsSubsystem = nullptr;
	UWorld* World = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = SimData)
	TWeakObjectPtr<AActor> Owner;

	UPROPERTY(BlueprintReadWrite, Category = SimData)
	TWeakObjectPtr<AController> Instigator;

	TFunction<void(const FTBProjectileFlightData&)> OnUpdate;

	bool AreDefaultDelegatesBound() const
	{
		return DefaultDelegatesAreBound;
	}

	void SetId(const FTBProjectileId& NewId)
	{
		Id = NewId;
		HasBeenAssignedId = Id.IsValid();
	}

	FTBProjectileId GetId() const
	{
		return Id;
	}

	void GenerateId()
	{
		Id.Generate();
		HasBeenAssignedId = true;
	}

	void GenerateNewId()
	{
		Id.Regenerate();
		HasBeenAssignedId = true;
	}

public:
	FTBSimData()
		: MaxIterations(TB::Configuration::MaxPenetrations)
		, bDrawDebugTrace(false)
		, bPrintDebugInfo(false)
		, IsTestTask(false)
		, HasBeenAssignedId(false)
		, DefaultDelegatesAreBound(false)
	{
		if (UpdateBroadcastInterval <= 0)
		{
			UpdateBroadcastInterval = TB::Configuration::DefaultThreadUpdateCallbackInterval;
		}
	}

	virtual ~FTBSimData() = default;

	explicit FTBSimData(const uint8 DebugType, const int InUpdateBroadcastInterval = 0, const bool IsTestTask = false, const FTBProjectileId& InId = FTBProjectileId::None)
		: MaxIterations(TB::Configuration::MaxPenetrations)
		, UpdateBroadcastInterval(InUpdateBroadcastInterval)
		, IsTestTask(IsTestTask)
		, HasBeenAssignedId(InId.IsValid())
		, DefaultDelegatesAreBound(false)
		, Id(InId)
	{
		bDrawDebugTrace = (DebugType & (uint8)ETBBallisticsDebugType::DrawDebugTrace) != 0U;
		bPrintDebugInfo = (DebugType & (uint8)ETBBallisticsDebugType::PrintDebugInfo) != 0U;

		if (UpdateBroadcastInterval <= 0)
		{
			UpdateBroadcastInterval = TB::Configuration::DefaultThreadUpdateCallbackInterval;
		}
	}

	FTBSimData(const bool bDrawDebugTrace, const bool bPrintDebugInfo, const int InUpdateBroadcastInterval = 0, const bool IsTestTask = false, const FTBProjectileId& InId = FTBProjectileId::None)
		: MaxIterations(TB::Configuration::MaxPenetrations)
		, UpdateBroadcastInterval(InUpdateBroadcastInterval)
		, bDrawDebugTrace(bDrawDebugTrace)
		, bPrintDebugInfo(bPrintDebugInfo)
		, IsTestTask(IsTestTask)
		, HasBeenAssignedId(InId.IsValid())
		, DefaultDelegatesAreBound(false)
		, Id(InId)
	{

		if (UpdateBroadcastInterval <= 0)
		{
			UpdateBroadcastInterval = TB::Configuration::DefaultThreadUpdateCallbackInterval;
		}
	}

	virtual double GetRadius() const { return 0.0; }

	void RemoveTracer()
	{
		TracerSystem = nullptr;
	}

	bool operator==(const FTBSimData& rhs) const
	{
		return Id == rhs.Id;
	}
};


USTRUCT(BlueprintType)
struct TERMINALBALLISTICS_API FTBBulletSimData : public FTBSimData, public FFastArraySerializerItem
{
	GENERATED_BODY()

public:
	using ImpactStruct = ImpactStruct<FTBImpactParams>;
	UPROPERTY(BlueprintAssignable, Category = SimData)
	FOnBulletHit onBulletHit;
	UPROPERTY(BlueprintAssignable, Category = SimData)
	FOnBulletExitHit onBulletExitHit;
	UPROPERTY(BlueprintAssignable, Category = SimData)
	FOnBulletInjure onBulletInjure;

	BulletPointer Bullet;

	TMulticastDelegate<void(const FBulletTaskResult&)> CompletionDelegate;

	TFunction<void(const FTBImpactParams&)> HitFunction;
	TFunction<void(const FTBImpactParams&)> ExitHitFunction;
	TFunction<void(const FTBImpactParams&, const FTBProjectileInjuryParams&)> InjureFunction;

public:
	FTBBulletSimData() = default;

	explicit FTBBulletSimData(BulletPointer InBullet, const int32 DebugType = 0, const int UpdateBroadcastInterval = 0, const bool IsTestTask = false, const FTBProjectileId& InId = FTBProjectileId::None);

	explicit FTBBulletSimData(BulletPointer InBullet, const bool bDrawDebugTrace = false, const bool bPrintDebugInfo = false, const int UpdateBroadcastInterval = 0, const bool IsTestTask = false, const FTBProjectileId& InId = FTBProjectileId::None);

	explicit FTBBulletSimData(const class UBulletDataAsset* BulletDataAsset, const int32 DebugType = 0, const int UpdateBroadcastInterval = 0, const bool IsTestTask = false, const FTBProjectileId& InId = FTBProjectileId::None);

	~FTBBulletSimData()
	{
		Bullet.Reset();

		onComplete.Clear();
		onBulletHit.Clear();
		onBulletExitHit.Clear();
		onBulletInjure.Clear();
	}

	void SetBullet(BulletPointer InBullet);

	virtual double GetRadius() const override;
};
template<> struct TB::Traits::TIsSimData<FTBBulletSimData> { enum { Value = true }; };

USTRUCT(BlueprintType)
struct TERMINALBALLISTICS_API FTBProjectileSimData : public FTBSimData, public FFastArraySerializerItem
{
	GENERATED_BODY()
public:
	using ImpactStruct = ImpactStruct<FTBImpactParamsBasic>;
	UPROPERTY(BlueprintAssignable, Category = SimData)
	FOnProjectileHit onProjectileHit;
	UPROPERTY(BlueprintAssignable, Category = SimData)
	FOnProjectileExitHit onProjectileExitHit;
	UPROPERTY(BlueprintAssignable, Category = SimData)
	FOnProjectileInjure onProjectileInjure;

	UPROPERTY(BlueprintReadonly, EditDefaultsOnly, Category = SimData)
	FTBProjectile Projectile;
	UPROPERTY(BlueprintReadonly, EditDefaultsOnly, Category = SimData)
	FPhysMatProperties ProjectilePhysicalProperties;

	TMulticastDelegate<void(const FProjectileTaskResult&)> CompletionDelegate;

	TFunction<void(const FTBImpactParamsBasic&)> HitFunction;
	TFunction<void(const FTBImpactParamsBasic&)> ExitHitFunction;
	TFunction<void(const FTBImpactParamsBasic&, const FTBProjectileInjuryParams&)> InjureFunction;

public:
	FTBProjectileSimData() = default;

	FTBProjectileSimData(const FTBProjectile& InProjectile, const FPhysMatProperties& InProjectilePhysicalProperties, const int32 DebugType = 0, int UpdateBroadcastInterval = 0, const bool IsTestTask = false, const FTBProjectileId& InId = FTBProjectileId::None)
		: FTBSimData(DebugType, UpdateBroadcastInterval, IsTestTask, InId)
	{
		Projectile = InProjectile;
		ProjectilePhysicalProperties = InProjectilePhysicalProperties;
	}

	~FTBProjectileSimData()
	{
		onProjectileHit.Clear();
		onProjectileExitHit.Clear();
		onProjectileInjure.Clear();
	}

	virtual inline double GetRadius() const override
	{
		return Projectile.Radius;
	}
};
template<> struct TB::Traits::TIsSimData<FTBProjectileSimData> { enum { Value = true }; };


#pragma region SimDataArrays
USTRUCT(BlueprintType)
struct TERMINALBALLISTICS_API FTBBulletSimDataArray : public FFastArraySerializer
{
	GENERATED_BODY()
public:
	FTBBulletSimDataArray() {}

	explicit FTBBulletSimDataArray(std::initializer_list<FTBBulletSimData>&& InitializerList)
		: SimData(InitializerList)
	{
		MarkArrayDirty();
	}
	explicit FTBBulletSimDataArray(TArray<FTBBulletSimData> InArray)
		: SimData(InArray)
	{
		MarkArrayDirty();
	}

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FTBBulletSimData, FTBBulletSimDataArray>(SimData, DeltaParms, *this);
	}

	inline int32 MarkItemDirtyByIndex(int32 index)
	{
		MarkItemDirty(SimData[index]);
		return index;
	}

	inline int32 Num() const
	{
		return SimData.Num();
	}

	inline int32 Add(const FTBBulletSimData& ToAdd) { return MarkItemDirtyByIndex(SimData.Add(ToAdd)); }

	inline int32 Remove(const FTBBulletSimData& ToRemove) { const int32 index = SimData.Remove(ToRemove); MarkArrayDirty(); return index; }
	inline int32 RemoveSwap(const FTBBulletSimData& ToRemove) { const int32 index = SimData.RemoveSwap(ToRemove); MarkArrayDirty(); return index; }
	inline int32 RemoveSingle(const FTBBulletSimData& ToRemove) { const int32 index = SimData.RemoveSingle(ToRemove); MarkArrayDirty(); return index; }
	inline int32 RemoveSingleSwap(const FTBBulletSimData& ToRemove) { const int32 index = SimData.RemoveSingleSwap(ToRemove); MarkArrayDirty(); return index; }

	inline void Empty(int32 Slack = 0) { SimData.Empty(Slack); }

	inline bool IsEmpty() const { return SimData.IsEmpty(); }

	inline bool Contains(const FTBBulletSimData& Item) const { return SimData.Contains(Item); }
	template<typename Predicate>
	inline bool ContainsByPredicate(Predicate Pred) const { return SimData.ContainsByPredicate(Pred); }

	template<typename Predicate>
	inline int32 IndexOfByPredicate(Predicate Pred) const { return SimData.IndexOfByPredicate(Pred); }

	inline int32 Find(const FTBBulletSimData& Item) { return SimData.Find(Item); }
	template<typename Predicate>
	inline FTBBulletSimData* FindByPredicate(Predicate Pred) { return SimData.FindByPredicate(Pred); }
	template<typename Predicate>
	inline const FTBBulletSimData* FindByPredicate(Predicate Pred) const { return SimData.FindByPredicate(Pred); }

	inline bool Dequeue(FTBBulletSimData& Item)
	{
		if (SimData.IsEmpty())
		{
			return false;
		}
		else {
			Item = SimData[0];
			SimData.RemoveAt(0);
			MarkArrayDirty();
			return true;
		}
	}

	FORCEINLINE FTBBulletSimData& operator[](int32 Index)
	{
		return SimData[Index];
	}
	FORCEINLINE const FTBBulletSimData& operator[](int32 Index) const
	{
		return SimData[Index];
	}

	inline TArray<FTBBulletSimData> Get() { return SimData; }

	TB_ARRAY_WRAPPED_STRUCT_ITERATOR_FUNCS(SimData)

protected:
	UPROPERTY()
	TArray<FTBBulletSimData> SimData;
};
template<> struct TStructOpsTypeTraits<FTBBulletSimDataArray> : public TStructOpsTypeTraitsBase2<FTBBulletSimDataArray>
{
	enum { WithNetDeltaSerializer = true };
};


USTRUCT(BlueprintType)
struct TERMINALBALLISTICS_API FTBProjectileSimDataArray : public FFastArraySerializer
{
	GENERATED_BODY()
public:
	FTBProjectileSimDataArray() {}

	explicit FTBProjectileSimDataArray(TArray<FTBProjectileSimData> InArray)
		: SimData(InArray)
	{
		MarkArrayDirty();
	}

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FTBProjectileSimData, FTBProjectileSimDataArray>(SimData, DeltaParms, *this);
	}

	inline int32 MarkItemDirtyByIndex(int32 index)
	{
		MarkItemDirty(SimData[index]);
		return index;
	}

	inline int32 Num() const
	{
		return SimData.Num();
	}

	inline int32 Add(const FTBProjectileSimData& ToAdd) { return MarkItemDirtyByIndex(SimData.Add(ToAdd)); }

	inline int32 Remove(const FTBProjectileSimData& ToRemove) { const int32 index = SimData.Remove(ToRemove); MarkArrayDirty(); return index; }
	inline int32 RemoveSwap(const FTBProjectileSimData& ToRemove) { const int32 index = SimData.RemoveSwap(ToRemove); MarkArrayDirty(); return index; }
	inline int32 RemoveSingle(const FTBProjectileSimData& ToRemove) { const int32 index = SimData.RemoveSingle(ToRemove); MarkArrayDirty(); return index; }
	inline int32 RemoveSingleSwap(const FTBProjectileSimData& ToRemove) { const int32 index = SimData.RemoveSingleSwap(ToRemove); MarkArrayDirty(); return index; }
	inline void RemoveAt(const int32 Index) { SimData.RemoveAt(Index); }

	inline bool IsValidIndex(const int32 Index) const { return SimData.IsValidIndex(Index); }

	inline void Empty(int32 Slack = 0) { SimData.Empty(Slack); }

	inline bool IsEmpty() const { return SimData.IsEmpty(); }

	inline bool Contains(const FTBProjectileSimData& Item) const { return SimData.Contains(Item); }
	template<typename Predicate>
	inline bool ContainsByPredicate(Predicate Pred) const { return SimData.ContainsByPredicate(Pred); }

	template<typename Predicate>
	inline int32 IndexOfByPredicate(Predicate Pred) const { return SimData.IndexOfByPredicate(Pred); }

	inline int32 Find(const FTBProjectileSimData& Item) { return SimData.Find(Item); }
	template<typename Predicate>
	inline FTBProjectileSimData* FindByPredicate(Predicate Pred) { return SimData.FindByPredicate(Pred); }
	template<typename Predicate>
	inline const FTBProjectileSimData* FindByPredicate(Predicate Pred) const { return SimData.FindByPredicate(Pred); }

	inline bool Dequeue(FTBProjectileSimData& Item)
	{
		if (SimData.IsEmpty())
		{
			return false;
		}
		else {
			Item = SimData[0];
			SimData.RemoveAt(0);
			MarkArrayDirty();
			return true;
		}
	}

	FORCEINLINE FTBProjectileSimData& operator[](int32 Index)
	{
		return SimData[Index];
	}
	FORCEINLINE const FTBProjectileSimData& operator[](int32 Index) const
	{
		return SimData[Index];
	}

	TB_ARRAY_WRAPPED_STRUCT_ITERATOR_FUNCS(SimData)

protected:
	UPROPERTY()
	TArray<FTBProjectileSimData> SimData;
};
template<> struct TStructOpsTypeTraits<FTBProjectileSimDataArray> : public TStructOpsTypeTraitsBase2<FTBProjectileSimDataArray>
{
	enum { WithNetDeltaSerializer = true };
};
#pragma endregion
