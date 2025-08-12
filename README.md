`Engel Weather Project`

Let unreal project name be `EngelProject`

add these `module names` to `EngelProject.Build.cs` found in `EngelProject/Source/EngelProject`

PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "RenderCore", "RHI", "Niagara" });

If the file do not exist, add `Visual Studio` option to your project

Copy the content of scripts to `EngelProject/Source/EngelProject`

if the corresponding Private and Public folders already exist in `EngelProject/Source/EngelProject`, the copy their contents to `EngelProject/Source/EngelProject/Private` and `EngelProject/Source/EngelProject/Public` respectively

Copy the rest of this folder to the content browser of `EngelProject`

In Unreal Engine, Go to ` Project Settings > Project > Maps & Modes` select `BP_engle_GameMode` as default GameMode and `BP_engel_Blueprint` as Defaut Pawn Class