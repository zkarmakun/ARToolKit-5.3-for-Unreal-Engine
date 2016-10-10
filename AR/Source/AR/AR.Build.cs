// Some copyright should be here...

using UnrealBuildTool;
using System.IO;

public class AR : ModuleRules
{
	public AR(TargetInfo Target)
	{
		
		PublicIncludePaths.AddRange(
			new string[] {
				"AR/Public"
				
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				"AR/Private",
				
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
                "RenderCore",
                "RHI",
                "ShaderCore",

				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            string ThirdParty = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../ThirdParty/"));

            string LibraryPath = Path.Combine(ThirdParty, "ARToolKit", "lib", "x64");
            string IncludePath = Path.Combine(ThirdParty, "ARToolKit", "include", "x64");
            string BinariesPath = Path.Combine(ThirdParty, "ARToolKit", "binaries");

            //include opencv
            PublicLibraryPaths.Add(LibraryPath);
            PublicIncludePaths.Add(IncludePath);
            PublicAdditionalLibraries.Add(LibraryPath + "/AR.lib");
            PublicAdditionalLibraries.Add(LibraryPath + "/AR2.lib");
            PublicAdditionalLibraries.Add(LibraryPath + "/ARICP.lib");
            PublicAdditionalLibraries.Add(LibraryPath + "/ARMulti.lib");
            PublicAdditionalLibraries.Add(LibraryPath + "/ARUtil.lib");
            PublicAdditionalLibraries.Add(LibraryPath + "/ARvideo.lib");
            PublicAdditionalLibraries.Add(LibraryPath + "/KPM.lib");
            PublicAdditionalLibraries.Add(LibraryPath + "/jpeg.lib");
            PublicAdditionalLibraries.Add(LibraryPath + "/pthreadVC2.lib");

            string projectPath = Directory.GetParent(Path.GetFullPath(ModuleDirectory)).ToString();
            projectPath = Directory.GetParent(projectPath).ToString();
            projectPath = Directory.GetParent(projectPath).ToString();
            projectPath = Directory.GetParent(projectPath).ToString();
            string BinaryMasterPath = Path.Combine(projectPath, "Binaries", Target.Platform.ToString());
            string[] dependencies = { "ARvideo.dll" , "DSVL.dll", "pthreadVC2.dll"};
            foreach(string dep in dependencies)
            {
                if(!File.Exists(Path.Combine(BinaryMasterPath,dep)))
                {
                    File.Copy(Path.Combine(BinariesPath, dep), Path.Combine(BinaryMasterPath, dep), true);
                }
            }
            //File.Copy("D:/UE/ARTOOLKIT_V2/Plugins/AR/ThirdParty/ARToolKit/binaries/ARvideo.dll", "D:/UE/ARTOOLKIT_V2/Binaries/Win64/ARvideo.dll", true);
  
            //Include dependencies
            RuntimeDependencies.Add(new RuntimeDependency(Path.Combine(BinaryMasterPath, "ARvideo.dll")));
            RuntimeDependencies.Add(new RuntimeDependency(Path.Combine(BinaryMasterPath, "DSVL.dll")));
            RuntimeDependencies.Add(new RuntimeDependency(Path.Combine(BinaryMasterPath, "pthreadVC2.dll")));

            


        }
        else if (Target.Platform == UnrealTargetPlatform.Android)
        {
            

            string ThirdParty = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../ThirdParty/"));

            string LibraryPath = Path.Combine(ThirdParty, "ARToolKit", "lib", "Android");
            string IncludePath = Path.Combine(ThirdParty, "ARToolKit", "include", "Android", "include");
            PublicIncludePaths.Add(IncludePath);
            PublicLibraryPaths.Add(LibraryPath);
            PublicAdditionalLibraries.Add(LibraryPath + "/libar.a");
            PublicAdditionalLibraries.Add(LibraryPath + "/libar2.a");
            PublicAdditionalLibraries.Add(LibraryPath + "/libaricp.a");
            PublicAdditionalLibraries.Add(LibraryPath + "/libarmulti.a");
            PublicAdditionalLibraries.Add(LibraryPath + "/libjpeg.a");
            PublicAdditionalLibraries.Add(LibraryPath + "/libkpm.a");
            PublicAdditionalLibraries.Add(LibraryPath + "/libutil.a");
            PublicAdditionalLibraries.Add(LibraryPath + "/libc++_shared.so");

            string pluginPath = Utils.MakePathRelativeTo(ModuleDirectory, BuildConfiguration.RelativeEnginePath);
            AdditionalPropertiesForReceipt.Add(new ReceiptProperty("AndroidPlugin", Path.Combine(pluginPath, "AR_APL.xml")));

        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            string ThirdParty = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../ThirdParty/"));

            string LibraryPath = Path.Combine(ThirdParty, "ARToolKit", "lib", "MAC");
            string IncludePath = Path.Combine(ThirdParty, "ARToolKit", "include", "MAC", "include");
            PublicIncludePaths.Add(IncludePath);
            PublicLibraryPaths.Add(LibraryPath);
            PublicAdditionalLibraries.Add(LibraryPath + "/libAR.a");
            PublicAdditionalLibraries.Add(LibraryPath + "/libAR2.a");
            PublicAdditionalLibraries.Add(LibraryPath + "/libARICP.a");
            PublicAdditionalLibraries.Add(LibraryPath + "/libARMulti.a");
            PublicAdditionalLibraries.Add(LibraryPath + "/libjpeg.a");
            PublicAdditionalLibraries.Add(LibraryPath + "/libKPM.a");
            PublicAdditionalLibraries.Add(LibraryPath + "/libARUtil.a");
            PublicAdditionalLibraries.Add(LibraryPath + "/libARVideo.a");
            PublicAdditionalLibraries.Add(LibraryPath + "/libEden.a");

            PublicFrameworks.AddRange(
                new string[] {
                    "QTKit",
                    "CoreVideo",
                    "Accelerate"
                }
            );
        }
        else if (Target.Platform == UnrealTargetPlatform.IOS)
        {
            string ThirdParty = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../ThirdParty/"));

            string LibraryPath = Path.Combine(ThirdParty, "ARToolKit", "lib", "IOS");
            string IncludePath = Path.Combine(ThirdParty, "ARToolKit", "include", "IOS", "include");
            PublicIncludePaths.Add(IncludePath);
            PublicLibraryPaths.Add(LibraryPath);
            PublicAdditionalLibraries.Add(LibraryPath + "/libAR.a");
            PublicAdditionalLibraries.Add(LibraryPath + "/libAR2.a");
            PublicAdditionalLibraries.Add(LibraryPath + "/libARICP.a");
            PublicAdditionalLibraries.Add(LibraryPath + "/libARMulti.a");
            PublicAdditionalLibraries.Add(LibraryPath + "/libjpeg.a");
            PublicAdditionalLibraries.Add(LibraryPath + "/libjpeg64.a");
            PublicAdditionalLibraries.Add(LibraryPath + "/libKPM.a");
            PublicAdditionalLibraries.Add(LibraryPath + "/libARUtil.a");
            PublicAdditionalLibraries.Add(LibraryPath + "/libARVideo.a");
            PublicAdditionalLibraries.Add(LibraryPath + "/libEden.a");

            PublicFrameworks.AddRange(
                new string[] {
                    "AVFoundation",
                    "CoreVideo",
                    "Accelerate"
                }
            );
        }

    }
}
