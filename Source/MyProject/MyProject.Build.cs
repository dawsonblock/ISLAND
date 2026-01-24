// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MyProject : ModuleRules
{
	public MyProject(ReadOnlyTargetRules Target) : base(Target)
	{
		// ─────────────────────────────────────────────────────────────
		// Build Optimization Settings
		// ─────────────────────────────────────────────────────────────
		
		// Use shared PCH for faster compilation
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		// Enable IWYU for cleaner includes
		bEnforceIWYU = true;
		
		// Faster iteration during development
		MinFilesUsingPrecompiledHeaderOverride = 1;
		bUseUnity = true; // Unity builds for faster full rebuilds

		// ─────────────────────────────────────────────────────────────
		// Core Dependencies (always needed)
		// ─────────────────────────────────────────────────────────────
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput"
		});

		// ─────────────────────────────────────────────────────────────
		// Gameplay Systems
		// ─────────────────────────────────────────────────────────────
		PublicDependencyModuleNames.AddRange(new string[] {
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"GameplayTasks",
			"NavigationSystem"
		});

		// ─────────────────────────────────────────────────────────────
		// UI Dependencies
		// ─────────────────────────────────────────────────────────────
		PublicDependencyModuleNames.AddRange(new string[] {
			"UMG",
			"Slate",
			"SlateCore"
		});

		// ─────────────────────────────────────────────────────────────
		// RFSN Integration (HTTP/JSON for AI dialogue)
		// ─────────────────────────────────────────────────────────────
		PublicDependencyModuleNames.AddRange(new string[] {
			"HTTP",
			"Json",
			"JsonUtilities"
		});

		// ─────────────────────────────────────────────────────────────
		// Private Dependencies (implementation only)
		// ─────────────────────────────────────────────────────────────
		PrivateDependencyModuleNames.AddRange(new string[] {
			"Niagara",   // VFX - only used in implementation
			"TraceLog"   // Debug logging
		});

		// ─────────────────────────────────────────────────────────────
		// Include Paths
		// ─────────────────────────────────────────────────────────────
		PublicIncludePaths.AddRange(new string[] {
			"MyProject",
			"MyProject/Public"
		});
		
		PrivateIncludePaths.AddRange(new string[] {
			"MyProject/Private",
			"MyProject/Variant_Horror",
			"MyProject/Variant_Horror/UI",
			"MyProject/Variant_Shooter",
			"MyProject/Variant_Shooter/AI",
			"MyProject/Variant_Shooter/UI",
			"MyProject/Variant_Shooter/Weapons"
		});
	}
}
