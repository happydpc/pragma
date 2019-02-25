#include "stdafx_weave_server.h"
#include "definitions.h"
#include <string>
#include <sstream>
//#include <stdlib.h>
#ifdef _WIN32
	#include <windows.h>
#else
	#include <unistd.h>
	#include <dlfcn.h>
	#include <algorithm>
	#include <iostream>
#endif
/*
#pragma comment(lib,"vfilesystem.lib")
#pragma comment(lib,"lua530.lib")
#pragma comment(lib,"luabind.lib")
#pragma comment(lib,"Shlwapi.lib")
#pragma comment(lib,"bz2-sgd-x86.lib")
#pragma comment(lib,"engine.lib")
*/

#ifdef _WIN32
static std::string get_last_system_error_string(DWORD errorMessageID)
{
    if(errorMessageID == 0)
        return "No error message has been recorded";
    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,errorMessageID,MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),(LPSTR)&messageBuffer,0,nullptr);
    std::string message(messageBuffer,size);
    LocalFree(messageBuffer);
    return message;
}
#endif

static std::string GetAppPath()
{
#ifdef __linux__
	std::string path = "";
	pid_t pid = getpid();
	char buf[20] = {0};
	sprintf(buf,"%d",pid);
	std::string _link = "/proc/";
	_link.append(buf);
	_link.append("/exe");
	char proc[512];
	int ch = readlink(_link.c_str(),proc,512);
	if(ch != -1)
	{
		proc[ch] = 0;
		path = proc;
		std::string::size_type t = path.find_last_of("/");
		path = path.substr(0,t);
	}
	return path;
#else
	char path[MAX_PATH +1];
	GetModuleFileName(NULL,path,MAX_PATH +1); // Requires windows.h

	std::string appPath = path;
	appPath = appPath.substr(0,appPath.rfind("\\"));
	return appPath;
#endif
}

int main(int argc,char* argv[]) try
{
	#if defined(_M_X64) || defined(__amd64__)
		#ifdef __linux__
			const char *library = "libengine_x64.so";
		#else
			const char *library = "engine.dll";
		#endif
	#else
		#ifdef __linux__
			const char *library = "libengine.so";
		#else
			const char *library = "engine.dll";
		#endif
	#endif
	#ifdef _WIN32
		std::string path = GetAppPath();
		path += "\\bin\\";
		path += library;
		HINSTANCE hEngine = LoadLibraryEx(path.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
		if(hEngine == nullptr)
		{
			auto err = GetLastError();
			std::stringstream msg;
			msg<<"Unable to load library 'bin\\"<<library<<"': "<<get_last_system_error_string(err)<<"("<<std::to_string(err)<<")";
			MessageBox(nullptr,msg.str().c_str(),"Critical Error",MB_OK | MB_ICONERROR);
			return EXIT_FAILURE;
		}
		void(*runEngine)(int,char*[]) = (void(*)(int,char*[]))GetProcAddress(hEngine,"RunEngine");
		if(runEngine != nullptr)
			runEngine(argc,argv);
	#else
		std::string path = "lib/";
		path += library;
		std::replace(path.begin(),path.end(),'\\','/');
		void *hEngine = dlopen(path.c_str(),RTLD_LAZY);
		if(hEngine == nullptr)
		{
			char *err = dlerror();
			std::cout<<"Unable to load library 'lib/"<<library<<"': "<<err<<std::endl;
			sleep(5);
			return EXIT_FAILURE;
		}
		void(*runEngine)(int,char*[]) = (void(*)(int,char*[]))dlsym(hEngine,"RunEngine");
		if(runEngine != nullptr)
			runEngine(argc,argv);
	#endif
#ifdef _DEBUG
	//_CrtDumpMemoryLeaks();
#endif
	return 0;
}
catch (...) {
	// Note: Calling std::current_exception in a std::set_terminate handler will return NULL due to a bug in the VS libraries.
	// Catching all unhandled exceptions here and then calling the handler works around that issue.
	std::get_terminate()();
}

#ifdef _WIN32
	int WINAPI WinMain(HINSTANCE,HINSTANCE,LPSTR,int)
	{
		return main(__argc,__argv);
	}
#endif