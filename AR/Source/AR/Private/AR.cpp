/*
*  IAR.cpp
*  ARToolKit5
*
*  This file encapsulate the core module of the plugin
*  File desciption: in this file you will find the implementation of the plugin module
*  this contain the unique singleton class and one unique instance of the ARToolKit Class
*  
*
*  ARToolKit is free software: you can redistribute it and/or modify
*  it under the terms of the GNU Lesser General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*
*  ARToolKit is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU Lesser General Public License for more details.
*
*  You should have received a copy of the GNU Lesser General Public License
*  along with ARToolKit.  If not, see <http://www.gnu.org/licenses/>.
*
*  As a special exception, the copyright holders of this library give you
*  permission to link this library with independent modules to produce an
*  executable, regardless of the license terms of these independent modules, and to
*  copy and distribute the resulting executable under terms of your choice,
*  provided that you also meet, for each linked independent module, the terms and
*  conditions of the license of that module. An independent module is a module
*  which is neither derived from nor based on this library. If you modify this
*  library, you may extend this exception to your version of the library, but you
*  are not obligated to do so. If you do not wish to do so, delete this exception
*  statement from your version.
*
*  Copyright 2015 Daqri, LLC.
*  Copyright 2010-2015 ARToolworks, Inc.
*
*  Author(s): Philip Lamb, Julian Looser.
*
*  Plugin Author(s): Jorge CR (AKA karmakun)
*
*  Special Thanks: tgraupmann (your android knowledge helps a lot)
*/

#include "ARPrivatePCH.h"
#include "IAR.h"
#include "ARToolKit.h"
#if PLATFORM_ANDROID
#include "../../../Core/Public/Android/AndroidApplication.h"
#include "../../../Launch/Public/Android/AndroidJNI.h"
#include <android/log.h>

#define LOG_TAG "ARToolKitLog"

int SetupJNICamera(JNIEnv* env);
JNIEnv* ENV = NULL;
static jmethodID jToast;
static jmethodID AndroidThunkJava_startCamera;
static jmethodID AndroidThunkJava_stopCamera;
static jmethodID AndroidThunkJava_AndroidFileExist;
static jmethodID AndroidThunkJava_MakeARDirectory;
static jmethodID AndroidThunkJava_Log;
static bool newFrame = false;
static unsigned char* rawDataAndroid;
#endif


#define LOCTEXT_NAMESPACE "FARModule"
class FARModule : public IARModule
{

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

};


void FARModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	TSharedRef<ARToolKit> ARToolKitInit(new ARToolKit());
	ARToolkit = ARToolKitInit;

#if PLATFORM_ANDROID
	JNIEnv* env = FAndroidApplication::GetJavaEnv();
	SetupJNICamera(env);
#endif

}

void FARModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	if (ARToolkit.IsValid())
	{
		ARToolkit->shutdownAR();
		ARToolkit = nullptr;
	}
	
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FARModule, AR)

#if PLATFORM_ANDROID
void AndroidThunkCpp_Log(FString tag, FString msg)
{
	if (!AndroidThunkJava_Log) return;
	jstring arg1 = ENV->NewStringUTF(TCHAR_TO_UTF8(*tag));
	jstring arg2 = ENV->NewStringUTF(TCHAR_TO_UTF8(*msg));
	FJavaWrapper::CallVoidMethod(ENV, FJavaWrapper::GameActivityThis, AndroidThunkJava_Log, arg1, arg2);
	ENV->DeleteLocalRef(arg1);
	ENV->DeleteLocalRef(arg2);
}

void AndroidThunkCpp_MakeARDirectory(FString path)
{
	if (!AndroidThunkJava_MakeARDirectory) return;
	jstring arg1 = ENV->NewStringUTF(TCHAR_TO_UTF8(*path));
	FJavaWrapper::CallVoidMethod(ENV, FJavaWrapper::GameActivityThis, AndroidThunkJava_MakeARDirectory, arg1);
	ENV->DeleteLocalRef(arg1);
}

bool AndroidThunkCpp_AFileExist(const FString& path)
{
	bool result = false;
	if (AndroidThunkJava_AndroidFileExist && ENV)
	{
		jstring Argument = ENV->NewStringUTF(TCHAR_TO_UTF8(*path));
		result = FJavaWrapper::CallBooleanMethod(ENV, FJavaWrapper::GameActivityThis, AndroidThunkJava_AndroidFileExist, Argument);
		ENV->DeleteLocalRef(Argument);
	}
	return result;
}

int SetupJNICamera(JNIEnv* env)
{
	if (!env) return JNI_ERR;

	ENV = env;

	AndroidThunkJava_startCamera = FJavaWrapper::FindMethod(ENV, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_startCamera", "()V", false);
	if (!AndroidThunkJava_startCamera)
	{
		__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "ERROR: AndroidThunkJava_startCamera Method cant be found T_T ");
		return JNI_ERR;
	}

	AndroidThunkJava_stopCamera = FJavaWrapper::FindMethod(ENV, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_stopCamera", "()V", false);
	if (!AndroidThunkJava_stopCamera)
	{
		__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "ERROR: AndroidThunkJava_stopCamera Method cant be found T_T ");
		return JNI_ERR;
	}

	AndroidThunkJava_AndroidFileExist = FJavaWrapper::FindMethod(ENV, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_AFileExist", "(Ljava/lang/String;)Z", false);
	if (!AndroidThunkJava_AndroidFileExist)
	{
		__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "ERROR: AndroidThunkJava_AFileExist Method cant be found T_T ");
		return JNI_ERR;
	}

	AndroidThunkJava_Log = FJavaWrapper::FindMethod(ENV, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_Log", "(Ljava/lang/String;Ljava/lang/String;)V", false);
	if (!AndroidThunkJava_Log)
	{
		__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "ERROR: AndroidThunkJava_Log Method cant be found T_T ");
		return JNI_ERR;
	}

	AndroidThunkJava_MakeARDirectory = FJavaWrapper::FindMethod(ENV, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_MakeARDirectory", "(Ljava/lang/String;)V", false);
	if (!AndroidThunkJava_MakeARDirectory)
	{
		__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "ERROR: AndroidThunkJava_MakeARDirectory Method cant be found T_T ");
		return JNI_ERR;
	}

	__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "module load success!!! ^_^");


	FString DirectoryPath = FPaths::GameContentDir(); // inDirectory;
	IFileManager* FileManager = &IFileManager::Get();

	// iterate over all the files in provided directory
	TArray<FString> directoriesToIgnoreAndNotRecurse;
	FLocalTimestampDirectoryVisitor Visitor(FPlatformFileManager::Get().GetPlatformFile(), directoriesToIgnoreAndNotRecurse, directoriesToIgnoreAndNotRecurse, false);
	FileManager->IterateDirectory(*DirectoryPath, Visitor);

	for (TMap<FString, FDateTime>::TIterator TimestampIt(Visitor.FileTimes); TimestampIt; ++TimestampIt)
	{
		// read the file contents and write it if successful to external path
		TArray<uint8> MemFile;
		const FString SourceFilename = TimestampIt.Key();
		FString DestFilename = GExternalFilePath / FPaths::GetCleanFilename(*SourceFilename);
		if (!SourceFilename.Contains(".uasset") && !SourceFilename.Contains(".umap") && !AndroidThunkCpp_AFileExist(DestFilename))
		{
			if (FFileHelper::LoadFileToArray(MemFile, *SourceFilename, 0))
			{
				if (FFileHelper::SaveArrayToFile(MemFile, *DestFilename))
				{
					AndroidThunkCpp_Log(LOG_TAG, "unzipped: " + DestFilename);
				}
			}
		}
		else if (AndroidThunkCpp_AFileExist(DestFilename))
		{
			AndroidThunkCpp_Log(LOG_TAG, "file exists, unzipped before: " + DestFilename);
		}
	}

	
	return JNI_OK;
}

void  AndroidThunkCpp_startCamera()
{
	if (!AndroidThunkJava_startCamera || !ENV) return;
	FJavaWrapper::CallVoidMethod(ENV, FJavaWrapper::GameActivityThis, AndroidThunkJava_startCamera);
}

void AndroidThunkCpp_stopCamera()
{
	if (!AndroidThunkJava_stopCamera || !ENV) return;
	FJavaWrapper::CallVoidMethod(ENV, FJavaWrapper::GameActivityThis, AndroidThunkJava_stopCamera);
}




extern "C" bool Java_com_epicgames_ue4_GameActivity_nativeGetFrameData(JNIEnv* LocalJNIEnv, jobject LocalThiz, jint frameWidth, jint frameHeight, jbyteArray data)
{
	//get the new frame
	int length = LocalJNIEnv->GetArrayLength(data);
	
	unsigned char* buffer = new unsigned char[length];
	LocalJNIEnv->GetByteArrayRegion(data, 0, length, reinterpret_cast<jbyte*>(buffer));
	rawDataAndroid = buffer;
	newFrame = true;
	//__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "new frame arrive ^_^");
	return JNI_TRUE;
}

#endif