`Engel Weather Project`

Let unreal project name be `EngelProject`

1.  add these `module names` to `EngelProject.Build.cs` found in `EngelProject/Source/EngelProject`

    PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "RenderCore", "RHI", "Niagara" });

If the file do not exist, add `Visual Studio` option to your project

2. Copy the content of scripts to `EngelProject/Source/EngelProject`

    if the corresponding Private and Public folders already exist in `EngelProject/Source/EngelProject`, the copy their contents to `EngelProject/Source/EngelProject/Private` and `EngelProject/Source/EngelProject/Public` respectively

3.  Close Unreal Engine and Visual Studio.

4.  Right-click your .uproject file → Generate Visual Studio project files.

5.  Open AMyActor_controllingWeather.h.

6.  Change `ENGELWEATHERUE5__API` from class ENGELWEATHERUE5_API AMyActor_controllingWeather to `EngelProject_API`

7.  Copy the rest of this folder to the content browser of `EngelProject`

8.  In Unreal Engine, Go to ` Project Settings > Project > Maps & Modes` select `BP_engle_GameMode` as default GameMode and `BP_engel_Blueprint` as Defaut Pawn Class