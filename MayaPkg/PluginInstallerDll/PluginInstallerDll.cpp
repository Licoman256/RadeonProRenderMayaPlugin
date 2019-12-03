#ifdef _WIN32
#include <windows.h>
#include <winreg.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif
#include "stdafx.h"
#include <msi.h>
#include <msiquery.h>
#include <Shellapi.h>
#include "checkCompatibility.h"
#include <RadeonProRender.h> // for get RPR_API_VERSION


#pragma comment(lib, "msi.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Ole32.lib")

#define VERSION_MAJOR(ver) ((ver) >> 28)
#define VERSION_MINOR(ver) ((ver) & 0xfffffff)

std::vector<std::wstring> getMayaVersionWithInstalledPlugin(MSIHANDLE hInstall)
{
	std::vector<std::wstring> versions = {
		L"2017",
		L"2018",
		L"2019"
	};

	std::vector<std::wstring> res;

	for (int i = 0; i < versions.size(); i++)
	{
		TCHAR val[MAX_PATH];
		DWORD valLen = MAX_PATH;

		// like MAYA2016_INSTALLED
		std::wstring propertyName = L"MAYA" + versions[i] + L"_INSTALLED";
		UINT res2 = MsiGetProperty(hInstall, propertyName.c_str(), val, &valLen);
		std::wstring strVal = val;
		if (strVal.empty())
			continue;
		res.push_back(versions[i]);
	}

	return res;
}


std::wstring getRegistationLink(MSIHANDLE hInstall)
{
	GetSystemInfo();

	// get Maya version with installed plugin
	std::wstring appversion;
	std::vector<std::wstring> versions = getMayaVersionWithInstalledPlugin(hInstall);
	for (size_t i = 0; i < versions.size(); i++)
	{
		if (i > 0)
			appversion += L";";
		appversion += versions[i];
	}

	// get FireRender version
	uint32_t majorVersion = RPR_VERSION_MAJOR;
	uint32_t minorVersion = RPR_VERSION_MINOR;
	uint32_t revisionVersion = RPR_VERSION_REVISION;
	TCHAR paramVal[MAX_PATH];
	wsprintf(paramVal, L"%X.%X.%X", majorVersion, minorVersion, revisionVersion);
	std::wstring frVersion = paramVal;

	std::wstring registrationid = L"5A1E27D27D97ECF5";
	std::wstring appname = L"autodeskmaya";
	std::wstring osVersion = g_systemInfo.osversion;
	std::wstring driverVersion = g_systemInfo.gpuDriver.size() > 0 ? g_systemInfo.gpuDriver[0] : std::wstring(L"");
	std::wstring gfxcard = g_systemInfo.gpuName.size()   > 0 ? g_systemInfo.gpuName[0] : std::wstring(L"");

	//link must look like :
	//"https://feedback.amd.com/se/5A1E27D23E8EC664?registrationid=5A1E27D27D97ECF5&appname=autodeskmaya&appversion=2016&frversion=1.6.30&os=win6.1.7601&gfxcard=AMD_FirePro_W8000__FireGL_V_&driverversion=15.201.2401.0"

	std::wstring sLink = L"https://feedback.amd.com/se/5A1E27D23E8EC664";

	sLink += L"?registrationid=" + URLfirendly(registrationid);
	sLink += L"&appname="		+ URLfirendly(appname);
	sLink += L"&appversion="	+ URLfirendly(appversion);
	sLink += L"&frversion="		+ URLfirendly(frVersion);
	sLink += L"&os="			+ URLfirendly(osVersion);
	sLink += L"&gfxcard="		+ URLfirendly(gfxcard);
	sLink += L"&driverversion=" + URLfirendly(driverVersion);

	LogSystem("getRegistationLink return: %s", WstringToString(sLink).c_str());

	return sLink;
}

bool getMayaPythonDirectory(const std::string& dirName_in, std::string& retPath)
{
	std::string regPath = "SOFTWARE\\Autodesk\\Maya\\" + dirName_in + "\\Setup\\InstallPath";

	HKEY hKey = HKEY();
	if (::RegOpenKeyExA(HKEY_LOCAL_MACHINE, regPath.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS)
	{
		LogSystem(std::string("Could not find path " + regPath + " in the registry!\n").c_str());

		return false;
	}

	char szPath[MAX_PATH] = {};
	DWORD pathSize = sizeof(szPath) / sizeof(szPath[0]);
	LSTATUS status = ::RegGetValueA(hKey, NULL, "MAYA_INSTALL_LOCATION", RRF_RT_ANY, 0, szPath, &pathSize);
	if (status != ERROR_SUCCESS)
	{
		LogSystem(std::string( "failed to read registry record with error code: " + std::to_string(status) + "\n").c_str());

		return false;
	}

	std::string fullPath (szPath);
	fullPath += "bin";

	retPath = fullPath;

	LogSystem(std::string("checking if file path " + fullPath + " exists...\n").c_str());

	DWORD ftyp = GetFileAttributesA(fullPath.c_str());
	if (ftyp == INVALID_FILE_ATTRIBUTES)
	{
		LogSystem(std::string("internal eror when trying to analyze path: " + fullPath + " \n").c_str());

		return false;  // something is wrong with your path!	
	}

	if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
	{
		LogSystem(std::string("Success! file path: " + fullPath + " exists\n").c_str());

		return true;   // this is a directory!
	}

	LogSystem(std::string("file path: " + fullPath + " doesn't exist!\n").c_str());

	return false;    // this is not a directory!
}

void installBoto3()
{
	LogSystem("installBoto3\n");

	const static std::vector<std::string> versions = {
		{"2017"},
		{"2018"},
		{"2019"}
	};

	for (const std::string& version : versions)
	{
		LogSystem(std::string("considering installBoto3 on " + version + "\n").c_str());

		std::string fullPath;
		if (!getMayaPythonDirectory(version, fullPath))
			continue;

		std::string cmdScript =
			"cd " + fullPath + " &&"
			"curl \"https://bootstrap.pypa.io/get-pip.py\" -o \"get-pip.py\" && "
			"mayapy get-pip.py &&"
			"mayapy -m pip install boto3\n"
			;

		try
		{
			LogSystem(std::string("try installBoto3 on " + version + "\n").c_str());

			int res = system(cmdScript.c_str());

			LogSystem(std::string("res = " + std::to_string(res) + "\n").c_str());
		}
		catch(...)
		{
			LogSystem(std::string("failed installBoto3 on " + version + "\n").c_str());
		}
	}
}


void setAutoloadPlugin(const std::wstring &maya_version)
{
	std::wstring destFolder = GetSystemFolderPaths(CSIDL_MYDOCUMENTS) + L"\\maya\\" + maya_version + L"\\prefs";
	std::wstring destF = destFolder + L"\\pluginPrefs.mel";

	std::string autoLoadLine = "evalDeferred(\"autoLoadPlugin(\\\"\\\", \\\"RadeonProRender\\\", \\\"RadeonProRender\\\")\");";

	std::fstream infile(destF);
	std::string line;
	bool found = false;
	while (std::getline(infile, line))
	{
		if (line.compare(autoLoadLine) == 0) {
			found = true;
			break;
		}
	}
	infile.close();

	if (!found) {
		std::fstream oFile(destF, std::ios::app);
		oFile << std::endl;
		oFile << autoLoadLine << std::endl;
	}
}


extern "C" __declspec(dllexport) UINT userRegister(MSIHANDLE hInstall) 
{
	LogSystem("userRegister...");

	std::wstring sLink = getRegistationLink(hInstall);

	HINSTANCE hIst = ShellExecute(NULL, L"open", sLink.c_str(), NULL, NULL, SW_SHOW);

	MsiSetProperty(hInstall, L"REGISTER_LINK", sLink.c_str());

	LogSystem("userRegister OK.");
	return ERROR_SUCCESS;
}


extern "C" __declspec(dllexport) UINT checkActivationKey(MSIHANDLE hInstall) 
{
	LogSystem("checkActivationKey...");
	TCHAR key[MAX_PATH];
	DWORD keyLen = MAX_PATH;

	MsiGetProperty(hInstall, L"ACTIVATION_KEY", key, &keyLen);

	LogSystem("   activation key: %s", WstringToString(key).c_str());

	std::wstring activationKey = L"TagYourRenders#ProRender";
	bool checkOk = (activationKey == key);

	LogSystem("   activation %s", checkOk ? "pass" : "failed");
	MsiSetProperty(hInstall, L"ACTIVATION_KEY_ACCEPTED", checkOk ? L"1" : L"0");

	// for error window
	if (!checkOk)
	{
		std::wstring sLink = getRegistationLink(hInstall);

		copyStringToClipboard(sLink);
		MsiSetProperty(hInstall, L"REGISTER_LINK", sLink.c_str());
	}

	RegSetKeyValue(HKEY_CURRENT_USER, L"SOFTWARE\\AMD\\RadeonProRender\\Maya", L"ACTIVATION_KEY_ACCEPTED", REG_SZ, checkOk ? L"1" : L"0", 2);

	LogSystem("checkActivationKey OK.");
	return ERROR_SUCCESS;
}


extern "C" __declspec(dllexport) UINT hardwareCheck(MSIHANDLE hInstall) 
{
	std::wstring hw_message;
	bool hw_res = checkCompatibility_hardware(hw_message);

	std::wstring sw_message;
	bool sw_res = checkCompatibility_driver(sw_message);

	std::wstring add_message(L"");

	MsiSetProperty(hInstall, L"HARDWARECHECK_RESULT", hw_res ? L"1" : L"0");
	MsiSetProperty(hInstall, L"SOFTWARECHECK_RESULT", sw_res ? L"1" : L"0");
	MsiSetProperty(hInstall, L"ADDITIONALCHECK_RESULT", L"1");

	if (!hw_res || !sw_res)
	{
		std::wstring text;

		if (!hw_res)
			text += L"\r\n" + hw_message;

		if (!sw_res)
			text += L"\r\n" + sw_message;

		std::wstring s = L"Detail info:" + text;

		MsiSetProperty(hInstall, L"CHECK_RESULT_TEXT", s.c_str());
	}

	return ERROR_SUCCESS;
}


extern "C" __declspec(dllexport) UINT postInstall(MSIHANDLE hInstall) 
{
	LogSystem("postInstall\n");

	std::vector<std::wstring> versions = getMayaVersionWithInstalledPlugin(hInstall);

	LogSystem(std::string("after getMayaVersionWithInstalledPlugin, versions.size = " + std::to_string(versions.size()) + "\n").c_str());

	for (size_t i = 0; i < versions.size(); i++)
	{
		setAutoloadPlugin(versions[i]);
	}

	return ERROR_SUCCESS;
}

extern "C" __declspec(dllexport) UINT botoInstall(MSIHANDLE hInstall)
{
	LogSystem("botoInstall\n");

	installBoto3();

	return ERROR_SUCCESS;
}
