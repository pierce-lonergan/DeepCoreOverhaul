using UnrealBuildTool;

public class DeepCoreEditorTarget : TargetRules
{
	public DeepCoreEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("DeepCore");
	}
}
