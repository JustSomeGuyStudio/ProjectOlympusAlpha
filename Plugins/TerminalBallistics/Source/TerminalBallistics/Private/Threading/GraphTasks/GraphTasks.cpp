// Copyright 2023, Erik Hedberg. All Rights Reserved.

#include "Threading/GraphTasks/GraphTasks.h"

namespace TB::Configuration
{
	bool bIgnoreImpactEventsWithInvalidData = true;
}

namespace TB::SimTasks
{
	FTaskWithCheckValue::FTaskWithCheckValue(bool* CheckValue)
		: CheckValue(CheckValue)
	{
		if (CheckValue != nullptr)
		{
			HasCheck = true;
		}
	}

	bool FTaskWithCheckValue::Check() const
	{
		if (HasCheck)
		{
			return (CheckValue && !*CheckValue);
		}
		return true;
	}
};

void GameThreadTask(TUniqueFunction<void()> Function, TB::SimTasks::FPendingTaskSynch* SynchObject)
{
	using namespace TB::SimTasks;
	TGraphTask<FLambdaTask>::CreateTask().ConstructAndDispatchWhenReady(MoveTemp(Function), SynchObject);
}
