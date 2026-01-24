// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MyProject : ModuleRules
{
	public MyProject(ReadOnlyTargetRules Target) : base(Target)
	{
		// ─────────────────────────────────────────────────────────────
		// Build Optimization Settings
		// ─────────────────────────────────────────────────────────────
		
		// Use shared PCH - significantly reduces compile times
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivatePCHHeaderFile = "MyProjectPCH.h";
		
		// Enable IWYU for cleaner includes and faster iteration
		bEnforceIWYU = true;
		
		// Unity builds - 2-3x faster full rebuilds
		bUseUnity = true;
		MinFilesUsingPrecompiledHeaderOverride = 1;
		
		// Optimize for fast iteration in non-shipping builds
		OptimizeCode = CodeOptimization.InShippingBuildsOnly;

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
		// Networking (for multiplayer dialogue replication)
		// ─────────────────────────────────────────────────────────────
		PublicDependencyModuleNames.AddRange(new string[] {
			"NetCore"
		});

		// ─────────────────────────────────────────────────────────────
		// Audio (for TTS and spatial dialogue)
		// ─────────────────────────────────────────────────────────────
		PrivateDependencyModuleNames.AddRange(new string[] {
			"AudioMixer"
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
