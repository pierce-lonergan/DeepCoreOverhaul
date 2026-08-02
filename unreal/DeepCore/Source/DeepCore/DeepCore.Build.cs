using UnrealBuildTool;

public class DeepCore : ModuleRules
{
	public DeepCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"ProceduralMeshComponent",
			"EnhancedInput",
			// FImageUtils::ImportBufferAsTexture2D decodes the generated PNGs at runtime.
			"ImageWrapper",
			"ImageCore"
		});

		// The game logic headers are shared verbatim with the standalone build. They are
		// stdlib-only by construction, which is exactly what makes this port possible.
		PublicIncludePaths.Add(ModuleDirectory + "/Shared");

		// The shared code uses sscanf to parse its own text level format. That is correct and
		// deliberate there -- the format is fixed and the destination is a std::string -- so
		// the deprecation warning is suppressed here rather than the shared file being forked
		// just to satisfy one compiler.
		PublicDefinitions.Add("_CRT_SECURE_NO_WARNINGS=1");
	}
}
