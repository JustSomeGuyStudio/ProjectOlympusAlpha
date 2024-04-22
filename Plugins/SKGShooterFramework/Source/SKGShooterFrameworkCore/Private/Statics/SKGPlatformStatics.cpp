// Copyright 2023, Dakota Dawe, All rights reserved


#include "Statics/SKGPlatformStatics.h"

#include "HAL/PlatformApplicationMisc.h"

void USKGPlatformStatics::ClipboardCopy(const FString& Text)
{
	FPlatformApplicationMisc::ClipboardCopy(*Text);
}

void USKGPlatformStatics::ClipboardCopyText(const FText& Text)
{
	ClipboardCopy(Text.ToString());
}

void USKGPlatformStatics::ClipboardCopyName(const FName& Text)
{
	ClipboardCopy(Text.ToString());
}

FString USKGPlatformStatics::ClipboardPaste()
{
	FString Result;
	FPlatformApplicationMisc::ClipboardPaste(Result);
	return Result;
}

FName USKGPlatformStatics::ClipboardPasteName()
{
	return FName(ClipboardPaste());
}

FText USKGPlatformStatics::ClipboardPasteText()
{
	return FText::FromString(ClipboardPaste());
}

bool USKGPlatformStatics::IsClipboardEmpty()
{
	return ClipboardPaste().IsEmpty();
}
