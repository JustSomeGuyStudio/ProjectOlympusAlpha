// Copyright 2023, Dakota Dawe, All rights reserved


#include "Statics/SKGShooterFrameworkHelpers.h"
#include "Components/SKGShooterPawnComponent.h"
#include "Components/SKGProceduralAnimComponent.h"
#include "Components/SKGFirearmComponent.h"
#include "Components/SKGOpticComponent.h"
#include "Components/SKGFirearmAttachmentStatComponent.h"
#include "Components/SKGLightLaserComponent.h"

#include "GameFramework/Actor.h"

USKGShooterPawnComponent* USKGShooterFrameworkHelpers::GetShooterPawnComponent(const AActor* Actor)
{
	if (Actor)
	{
		return Actor->FindComponentByClass<USKGShooterPawnComponent>();
	}
	return nullptr;
}

USKGProceduralAnimComponent* USKGShooterFrameworkHelpers::GetProceduralAnimComponent(const AActor* Actor)
{
	if (Actor)
	{
		return Actor->FindComponentByClass<USKGProceduralAnimComponent>();
	}
	return nullptr;
}

USKGFirearmComponent* USKGShooterFrameworkHelpers::GetFirearmComponent(const AActor* Actor)
{
	if (Actor)
	{
		return Actor->FindComponentByClass<USKGFirearmComponent>();
	}
	return nullptr;
}

USKGOpticComponent* USKGShooterFrameworkHelpers::GetOpticComponent(const AActor* Actor)
{
	if (Actor)
	{
		return Actor->FindComponentByClass<USKGOpticComponent>();
	}
	return nullptr;
}

USKGFirearmAttachmentStatComponent* USKGShooterFrameworkHelpers::GetFirearmAttachmentStatComponent(const AActor* Actor)
{
	if (Actor)
	{
		return Actor->FindComponentByClass<USKGFirearmAttachmentStatComponent>();
	}
	return nullptr;
}

USKGLightLaserComponent* USKGShooterFrameworkHelpers::GetLightLaserComponent(const AActor* Actor)
{
	if (Actor)
	{
		return Actor->FindComponentByClass<USKGLightLaserComponent>();
	}
	return nullptr;
}

AActor* USKGShooterFrameworkHelpers::GetParentFirearmComponent(const AActor* Actor)
{
	if (Actor)
	{
		for (AActor* ParentActor = Actor->GetOwner(); ParentActor; ParentActor = ParentActor->GetOwner())
		{
			if (GetFirearmComponent(ParentActor))
			{
				return ParentActor;
			}
		}
	}
	return nullptr;
}
