#include "DeepCoreTune.h"

FDeepCoreTune& FDeepCoreTune::Get()
{
	static FDeepCoreTune Instance;
	return Instance;
}

void FDeepCoreTune::ParseCommandLine()
{
	// The final `false` is bShouldStopOnSeparator, and it is load-bearing. It defaults to TRUE,
	// which makes FParse::Value stop at a comma -- so "amb=0.1,ev=3,alb=2" silently parsed as
	// just "amb=0.1" and every other key kept its default. A whole 19-variant sweep was captured
	// before this was noticed, in which each variant differed from the next only by its FIRST
	// key, and the variants therefore looked nearly identical for entirely the wrong reason.
	FString Raw;
	if (!FParse::Value(FCommandLine::Get(), TEXT("DeepCoreTune="), Raw, false))
	{
		UE_LOG(LogTemp, Display, TEXT("DeepCore: tune -- defaults"));
		return;
	}

	FDeepCoreTune& T = Get();

	TArray<FString> Pairs;
	Raw.ParseIntoArray(Pairs, TEXT(","), true);

	TArray<FString> Unknown;
	for (const FString& Pair : Pairs)
	{
		FString Key, Value;
		if (!Pair.Split(TEXT("="), &Key, &Value))
		{
			Unknown.Add(Pair);
			continue;
		}
		Key = Key.TrimStartAndEnd().ToLower();
		const float V = FCString::Atof(*Value);

		if      (Key == TEXT("ev"))       { T.Ev = V; }
		else if (Key == TEXT("sky"))      { T.Sky = V; }
		else if (Key == TEXT("fog"))      { T.FogDensity = V; }
		else if (Key == TEXT("fogext"))   { T.FogExtinct = V; }
		else if (Key == TEXT("foganiso")) { T.FogAniso = V; }
		else if (Key == TEXT("wl"))       { T.Worklight = V; }
		else if (Key == TEXT("wlz"))      { T.WorklightZ = V; }
		else if (Key == TEXT("wlstep"))   { T.WorklightStep = FMath::Max(2, FMath::RoundToInt(V)); }
		else if (Key == TEXT("lamp"))     { T.CapLamp = V; }
		else if (Key == TEXT("amb"))      { T.Ambient = V; }
		else if (Key == TEXT("rough"))    { T.Roughness = V; }
		else if (Key == TEXT("alb"))      { T.AlbedoScale = V; }
		else if (Key == TEXT("disp"))     { T.Displace = V; }
		else if (Key == TEXT("strata"))   { T.Strata = V; }
		else if (Key == TEXT("rock"))     { T.RockHeight = V; }
		else                              { Unknown.Add(Key); }
	}

	// Reported, not ignored. A misspelt key would otherwise produce a variant identical to the
	// previous one, and the natural conclusion from that is "this setting does nothing" --
	// which is exactly the wrong lesson to draw in the middle of a tuning sweep.
	if (Unknown.Num() > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("DeepCore: tune -- UNKNOWN KEYS: %s"),
		       *FString::Join(Unknown, TEXT(" ")));
	}

	UE_LOG(LogTemp, Display,
	       TEXT("DeepCore: tune -- ev=%.2f sky=%.2f fog=%.4f fogext=%.2f aniso=%.2f wl=%.0f wlz=%.0f ")
	       TEXT("wlstep=%d lamp=%.0f amb=%.3f rough=%.2f alb=%.2f disp=%.2f strata=%.2f rock=%.0f"),
	       T.Ev, T.Sky, T.FogDensity, T.FogExtinct, T.FogAniso, T.Worklight, T.WorklightZ,
	       T.WorklightStep, T.CapLamp, T.Ambient, T.Roughness, T.AlbedoScale, T.Displace, T.Strata,
	       T.RockHeight);
}
