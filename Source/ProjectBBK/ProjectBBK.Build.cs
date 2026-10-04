// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ProjectBBK : ModuleRules
{
	public ProjectBBK(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// 모듈 폴더를 include 경로에 추가해서 에디터에서 헤더 파일을 찾을 수 있도록 설정
		PublicIncludePaths.Add(ModuleDirectory);

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "GameplayAbilities", "GameplayTags", "GameplayTasks", "Niagara", "LevelSequence", "MovieScene", "Landscape" });

		PrivateDependencyModuleNames.AddRange(new string[] { "UMG", "Slate", "SlateCore", "GameplayAbilities", "MoviePlayer" });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
