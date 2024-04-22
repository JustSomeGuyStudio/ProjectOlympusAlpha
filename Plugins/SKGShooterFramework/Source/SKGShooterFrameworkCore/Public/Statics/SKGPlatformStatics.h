// Copyright 2023, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SKGPlatformStatics.generated.h"

/**
 * 
 */
UCLASS()
class SKGSHOOTERFRAMEWORKCORE_API USKGPlatformStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Copies the given FString to your platforms clipboard
	UFUNCTION(BlueprintCallable, Category = "SKGPlatformStatics|Clipboard")
	static void ClipboardCopy(const FString& Text);
	// Copies the given FName to your platforms clipboard
	UFUNCTION(BlueprintCallable, Category = "SKGPlatformStatics|Clipboard")
	static void ClipboardCopyName(const FName& Text);
	// Copies the given FText to your platforms clipboard
	UFUNCTION(BlueprintCallable, Category = "SKGPlatformStatics|Clipboard")
	static void ClipboardCopyText(const FText& Text);
	
	// Returns the text from your platforms clipboard as type FString
	UFUNCTION(BlueprintPure, Category = "SKGPlatformStatics|Clipboard")
	static FString ClipboardPaste();
	// Returns the text from your platforms clipboard as type FName
	UFUNCTION(BlueprintPure, Category = "SKGPlatformStatics|Clipboard")
	static FName ClipboardPasteName();
	// Returns the text from your platforms clipboard as type FText
	UFUNCTION(BlueprintPure, Category = "SKGPlatformStatics|Clipboard")
	static FText ClipboardPasteText();
	
	// Returns true if you have nothing copied to the clipboard
	UFUNCTION(BlueprintPure, Category = "SKGPlatformStatics|Clipboard")
	static bool IsClipboardEmpty();
};
