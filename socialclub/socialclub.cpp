#include <Windows.h>

#include <stdio.h>
#include <stdint.h>
#include <string>
#include <time.h>

#include <shlobj.h>
#include <chrono>

#include <iostream>
#include <fstream>
#include "third-party/CMemory.h"
#include <TlHelp32.h>
#include <Thread>

/*
std::ofstream log_file("log_file.txt", std::ios_base::out | std::ios_base::app);
std::wofstream log_filew("log_file_w.txt", std::ios_base::out | std::ios_base::app);
*/
using namespace std;

#define PROGRAM_NAME L"Goldberg SocialClub Emu"
#define EMU_RELEASE_BUILD L"Deftones"
#define GAME_NAME L"GTA_DEF"
wstring gameExe;
static uint64_t ROS_DUMMY_ACCOUNT_ID = 0x0F74F4C4;
static char ROS_DUMMY_ACCOUNT_USERNAME[32] = "Goldberg";

#include <codecvt>

std::wstring toUTF16(std::string s)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return std::wstring(converter.from_bytes(s));
}

std::string toUTF8(std::wstring w)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return std::string(converter.to_bytes(w));
}

static BOOL DirectoryExists(LPCWSTR szPath)
{
    DWORD dwAttrib = GetFileAttributesW(szPath);

    return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
        (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

static void createDirectoryRecursively(std::wstring path)
{
    unsigned long long pos = 0;
    do
    {
        pos = path.find_first_of(L"\\/", pos + 1);
        CreateDirectoryW(path.substr(0, pos).c_str(), NULL);
    } while (pos != std::wstring::npos);
}

static void create_directory(std::wstring strPath)
{
    if (DirectoryExists(strPath.c_str()) == FALSE)
        createDirectoryRecursively(strPath);
}

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
std::wstring get_full_lib_path()
{
    std::wstring program_path;
    WCHAR   DllPath[MAX_PATH] = { 0 };
    GetModuleFileNameW((HINSTANCE)&__ImageBase, DllPath, _countof(DllPath));
    program_path = DllPath;
    return program_path;
}

std::wstring get_full_program_path()
{
    std::wstring program_path;
    program_path = get_full_lib_path();
    return program_path.substr(0, program_path.rfind(L"\\")).append(L"\\");
}


std::wstring get_userdata_path()
{
    static std::wstring full_path;
    if (full_path.size()) {
        return full_path;
    }

    std::wstring user_appdata_path = L"SAVE";
    WCHAR szPath[MAX_PATH] = {};

    HRESULT hr = SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, szPath);

    if (SUCCEEDED(hr)) {
        user_appdata_path = szPath;
    }

    full_path = user_appdata_path.append(L"\\");
    return full_path;
}

int get_file_data(std::wstring full_path, char* data, unsigned int max_length, unsigned int offset);
std::wstring get_save_path()
{
    std::wstring path;
    std::wstring prog_path = get_full_program_path();
    char output[1024] = {};
    if (get_file_data(prog_path + L"local_save.txt", output, sizeof(output), 0) < 0) {
        path = get_userdata_path();
        return path.append(PROGRAM_NAME).append(L" Saves").append(L"\\");
    }
    else {
        std::string data = output;
        if (!data.size()) {
            data = "SAVE";
        }

        path = prog_path;
        return path.append(toUTF16(data)).append(L"\\");
    }
}

std::wstring get_game_save_path()
{
    static std::wstring full_path;
    if (full_path.size()) {
        return full_path;
    }

    // char profile_folder[32];
    // snprintf(profile_folder, sizeof(profile_folder), "%08X", ROS_DUMMY_ACCOUNT_ID);
    // full_path = get_save_path().append(GAME_NAME).append("\\").append(profile_folder).append("\\");
    full_path = get_save_path().append(GAME_NAME).append(L"\\").append(L"0F74F4C4").append(L"\\");
    createDirectoryRecursively(full_path);
    return full_path;
}

std::wstring get_global_settings_path()
{
    static std::wstring full_path;
    if (full_path.size()) {
        return full_path;
    }

    full_path = get_save_path().append(L"settings").append(L"\\");
    return full_path;
}

#include <fstream>
int get_file_data(std::wstring full_path, char* data, unsigned int max_length, unsigned int offset)
{
    std::ifstream myfile;
    myfile.open(full_path, std::ios::binary | std::ios::in);
    if (!myfile.is_open()) return -1;

    myfile.seekg(offset, std::ios::beg);
    myfile.read(data, max_length);
    myfile.close();
    return myfile.gcount();
}

int get_data_settings(std::wstring file, char* data, unsigned int max_length)
{
    std::wstring full_path = get_global_settings_path() + file;
    return get_file_data(full_path, data, max_length, 0);
}

int store_file_data(std::wstring folder, std::wstring file, char* data, unsigned int length)
{
    if (folder.back() != *L"\\") {
        folder.append(L"\\");
    }

    std::wstring::size_type pos = file.rfind(L"\\");

    std::wstring file_folder;
    if (pos == 0 || pos == std::wstring::npos) {
        file_folder = L"";
    }
    else {
        file_folder = file.substr(0, pos);
    }

    create_directory(folder + file_folder);
    std::ofstream myfile;
    myfile.open(folder + file, std::ios::binary | std::ios::out);
    if (!myfile.is_open()) return -1;
    myfile.write(data, length);
    int position = myfile.tellp();
    myfile.close();
    return position;
}

int store_data_settings(std::wstring file, char* data, unsigned int length)
{
    return store_file_data(get_global_settings_path(), file, data, length);
}


#ifdef EMU_RELEASE_BUILD
#define PRINT_DEBUG(a, ...);
#else
#define PRINT_DEBUG(a, ...) do {FILE *t = fopen("socialclub_LOG.txt", "a"); fprintf(t, "%u " a, GetCurrentThreadId(), __VA_ARGS__); fclose(t);} while (0)
#endif


class IProfileV3
{
public:
    virtual HRESULT __stdcall QueryInterface(GUID* iid, void** out) = 0;
    virtual void __stdcall SetAccountId(uint64_t accountId) = 0;
    virtual void __stdcall SetUsername(const char* userName) = 0;
    virtual void __stdcall SetKey(const uint8_t* key) = 0;
    virtual void __stdcall SetBool1(bool value) = 0;
    virtual void __stdcall SetTicket(const char* ticket) = 0;
    virtual void __stdcall SetEmail(const char* email) = 0;
    virtual void __stdcall SetUnkString(const char* str) = 0;
    virtual void __stdcall SetScAuthToken(const char* str) = 0;

    //functions to get values after
};


class ProfileManager
{
public:
    virtual int __stdcall GetInterface(const /*Guid_t*/ void* guid, ProfileManager** profile_interface) = 0;
    virtual bool __stdcall b() = 0;
    virtual bool __stdcall isOnline() = 0;
    virtual int __stdcall getProfileData(IProfileV3* profile) = 0;
    virtual int __stdcall e() = 0;
    virtual bool __stdcall f() = 0;
    virtual bool __stdcall g() = 0;
    virtual void __stdcall h(void* b) = 0;
    virtual void __stdcall i(void* b, void* c, void* d) = 0;
    virtual void __stdcall j() = 0;
    virtual void __stdcall k(void* b, void* c) = 0;
    virtual bool __stdcall l(void* b, void* c) = 0;
};

class AchievementA
{
    virtual void a() = 0;
    virtual int b() = 0;
    virtual void  c() = 0;
    virtual void  d() = 0;
    virtual void  e() = 0;
    virtual void  f() = 0;
    virtual void  g() = 0;
    virtual void  h() = 0;
    virtual void   i() = 0;
    virtual void   j() = 0;
    virtual void   k() = 0;
    virtual void   l() = 0;
    virtual void   m() = 0;
    virtual void   n() = 0;
    virtual void   o() = 0;
    virtual void   p() = 0;
    virtual void   q() = 0;
    virtual void   r() = 0;
    virtual void   s() = 0;
    virtual void   t() = 0;
    virtual void   u() = 0;
};

class AchievementManager
{
public:
    virtual int __stdcall GetInterface(const /*Guid_t*/ void* guid, AchievementManager** manager_interface) = 0;
    virtual int __stdcall b(void* b, void* c, void* d, void* e, void* f, void* g, void* h, void* i, void* j) = 0;
    virtual int __stdcall c(void* b, void* c, void* d, AchievementA** e) = 0;
    virtual int __stdcall d(int b) = 0; //called to set achievement
    virtual int __stdcall e(void* b, void* c, void* d, void* e, void* f) = 0;
    virtual int __stdcall f(void* b, void* c, void* d) = 0;
    virtual int __stdcall g() = 0;
    //
    virtual void __stdcall h() = 0;
    virtual void __stdcall i() = 0;
    virtual void __stdcall j() = 0;
    virtual void __stdcall k() = 0;
    virtual void __stdcall l() = 0;
    virtual void  __stdcall m() = 0;
    virtual void  __stdcall n() = 0;
    virtual void  __stdcall o() = 0;
    virtual void  __stdcall p() = 0;
    virtual void  __stdcall q() = 0;
    virtual void  __stdcall r() = 0;
    virtual void  __stdcall s() = 0;
    virtual void  __stdcall t() = 0;
    virtual void  __stdcall u() = 0;
    virtual void  __stdcall v() = 0;
    virtual void  __stdcall w() = 0;
    virtual void  __stdcall x() = 0;
    virtual void  __stdcall y() = 0;
    virtual void  __stdcall z() = 0;
    virtual void __stdcall aa() = 0;
    virtual void __stdcall ab() = 0;
    virtual void __stdcall ac() = 0;
    virtual void __stdcall ad() = 0;
    virtual void __stdcall ae() = 0;
    virtual void __stdcall af() = 0;
};

class PlayerManager
{
public:
    virtual int __stdcall GetInterface(const /*Guid_t*/ void* guid, PlayerManager** manager_interface) = 0;
    virtual int __stdcall b(void* b, void* c, void* d, void* e, void* f, void* g, void* h, void* i) = 0;
    virtual int __stdcall c(void* b, void* c, void* d, void* e, void* f) = 0;
    virtual bool __stdcall d(void* b, void* c) = 0;
    virtual int __stdcall e(void* b, void* c) = 0;
    virtual int __stdcall f(void* b, void* c) = 0;
    virtual int __stdcall g(void* b, void* c, void* d) = 0;
    virtual bool __stdcall h(void* b, void* c) = 0;
    virtual int __stdcall i() = 0;
    virtual bool __stdcall j() = 0;
    virtual bool __stdcall k() = 0;
    virtual int __stdcall l() = 0;
};

class PresenceManager
{
public:
    virtual int __stdcall GetInterface(const /*Guid_t*/ void* guid, PresenceManager** presence_interface) = 0;
    virtual bool __stdcall b(void* b, char* c, void* d, void* e) = 0;
    virtual bool __stdcall c(void* b, void* c, void* d, void* e) = 0;
    virtual bool __stdcall d(void* b, char* key, char* value) = 0;
    virtual bool __stdcall e(void* b, void* c, void* d) = 0;
    virtual bool __stdcall f(void* b, void* c, void* d) = 0;
    virtual bool __stdcall g(void* b, void* c, void* d, void* e) = 0;
    virtual bool __stdcall h(void* b, void* c, void* d, void* e, void* f) = 0;
    virtual bool __stdcall i(void* b, void* c, void* d, void* e, void* f, void* g) = 0;
    virtual bool __stdcall j(void* b, void* c, void* d, void* e, void* f, void* g, void* h, void* i, void* j, void* k) = 0;
    virtual bool __stdcall k(void* b, void* c, void* d, void* e) = 0;
    virtual bool __stdcall l(void* b, void* c, void* d, void* e) = 0;
    virtual bool __stdcall m(void* b) = 0;
    virtual bool __stdcall n(void* b) = 0;
    virtual bool __stdcall o(void* b) = 0;
    virtual bool __stdcall p(void* b) = 0;
    virtual bool  __stdcall q(void* b, void* c, void* d) = 0;
    virtual bool  __stdcall r(void* b, void* c, void* d) = 0;
    virtual bool  __stdcall s(void* b) = 0;
    virtual bool  __stdcall t(void* b, void* c, void* d, void* e, void* f, void* g) = 0;
    virtual bool  __stdcall u(void* b) = 0;
    virtual bool  __stdcall v(void* b) = 0;
    virtual bool  __stdcall w(void* b, void* c, void* d, void* e) = 0;
    virtual bool  __stdcall x(void* b) = 0;
    virtual bool  __stdcall y(void* b) = 0;
    virtual bool  __stdcall z(void* b) = 0;
};

class AsyncTask
{
public:
    virtual int __stdcall GetInterface(const /*Guid_t*/ void* guid, AsyncTask** manager_interface) = 0;
    virtual void __stdcall b() = 0;
    virtual bool __stdcall c() = 0;
    virtual void __stdcall d(void* b, void* c) = 0;
    virtual int __stdcall e() = 0;
    virtual void __stdcall f() = 0;
    virtual bool __stdcall g() = 0;
    virtual bool __stdcall h() = 0;
    virtual bool __stdcall i() = 0;
    virtual bool __stdcall j() = 0;
    virtual void __stdcall k() = 0;
    virtual void __stdcall l() = 0;
};


class CommerceManager
{
public:
    virtual int __stdcall  GetInterface(const /*Guid_t*/ void* guid, CommerceManager** manager_interface) = 0;
    virtual int __stdcall  b(void* b) = 0;
    virtual int __stdcall  c(void* b) = 0;
    virtual int __stdcall  d(void* b, void* c, void* d) = 0;
    virtual void __stdcall  e(void* b, void* c) = 0;
    virtual int __stdcall  f(void* b, void* c, void* d, void* e) = 0;
    virtual void __stdcall  g(void* b) = 0;
    virtual bool __stdcall  h() = 0;
    virtual bool __stdcall  i() = 0;
    virtual bool __stdcall  j(void* b, void* c, void* d, void* e, void* f, void* g, void* h, void* i) = 0;
    virtual bool __stdcall  k(void* b, void* c, void* d) = 0;
    virtual bool __stdcall  l(char* str, AsyncTask* task) = 0;
    virtual bool __stdcall  DLC_check_2(char* dlc_name) = 0;
    virtual bool __stdcall  DLC_check(char* dlc_name, bool* c, bool* d, bool* e) = 0;
    virtual bool __stdcall  o(void* b, void* c, void* d) = 0;
    virtual bool __stdcall  p(void* b, void* c, void* d, void* e) = 0;

    virtual void  __stdcall q() = 0;
    virtual void  __stdcall r() = 0;
    virtual void  __stdcall s() = 0;
    virtual void  __stdcall t() = 0;
    virtual void  __stdcall u() = 0;
    virtual void  __stdcall v(void* b) = 0; //b is function pointer
    virtual void  __stdcall w() = 0;
    virtual void  __stdcall x() = 0;
    virtual void  __stdcall y() = 0;
    virtual void  __stdcall z() = 0;
    virtual void __stdcall aa() = 0;
    virtual void __stdcall ab() = 0;
    virtual void __stdcall ac() = 0;
    virtual void __stdcall ad() = 0;
    virtual void __stdcall ae() = 0;
    virtual void __stdcall af() = 0;
    virtual void __stdcall ag() = 0;
    virtual void __stdcall ah() = 0;
    virtual void __stdcall ai() = 0;
    virtual void __stdcall aj() = 0;
    virtual void __stdcall ak() = 0;
    virtual void __stdcall al() = 0;
    virtual void __stdcall am() = 0;
    virtual void __stdcall an() = 0;
    virtual void __stdcall ao() = 0;
    virtual void  __stdcall ap() = 0;
    virtual void  __stdcall aq() = 0;
    virtual void  __stdcall ar() = 0;
    virtual void  __stdcall as() = 0;
    virtual void  __stdcall at() = 0;
    virtual void  __stdcall au() = 0;
    virtual void  __stdcall av() = 0;
    virtual void  __stdcall aw() = 0;
    virtual void  __stdcall ax() = 0;
    virtual void  __stdcall ay() = 0;
    virtual void  __stdcall az() = 0;
    virtual void __stdcall ba() = 0;
    virtual void __stdcall bb() = 0;
    virtual void __stdcall bc() = 0;
    virtual void __stdcall bd() = 0;
    virtual void __stdcall be() = 0;
    virtual void __stdcall bf() = 0;
    virtual void __stdcall bg() = 0;
    virtual void __stdcall bh() = 0;
    virtual void __stdcall bi() = 0;
};

class FileSystem
{
public:
    virtual int __stdcall  GetInterface(const /*Guid_t*/ void* guid, FileSystem** manager_interface) = 0;
    virtual int __stdcall  b(void* b, void* c) = 0;
    virtual int __stdcall  getProfilePath(char* buffer, int flag) = 0;
    virtual int __stdcall  d(void* b) = 0;
    virtual int __stdcall  e(void* b) = 0;
    virtual int __stdcall  f(void* b, void* c) = 0;
    virtual void __stdcall  g(bool b) = 0;
    //
    virtual void __stdcall  h() = 0;
};

class SocialClubH
{
public:
    virtual int __stdcall  GetInterface(const /*Guid_t*/ void* guid, SocialClubH** manager_interface) = 0;
    virtual void __stdcall  b() = 0;
    virtual void __stdcall  c() = 0;
    virtual void __stdcall  d() = 0;
    virtual void __stdcall  e() = 0;
    virtual void __stdcall  f() = 0;
    virtual void __stdcall  g() = 0;
    virtual void __stdcall  h() = 0;
    virtual void __stdcall  i() = 0;
    virtual void __stdcall  j() = 0;
    virtual void __stdcall  k() = 0;
    virtual void __stdcall  l() = 0;
};
class SocialClubI
{
public:
    virtual int __stdcall  GetInterface(const /*Guid_t*/ void* guid, SocialClubI** manager_interface) = 0;
    virtual void __stdcall  b() = 0;
    virtual void __stdcall  c() = 0;
    virtual void __stdcall  d() = 0;
    virtual void __stdcall  e() = 0;
    virtual void __stdcall  f() = 0;
    virtual void __stdcall  g() = 0;
    virtual void __stdcall  h() = 0;
    virtual void __stdcall  i() = 0;
    virtual void __stdcall  j() = 0;
    virtual void __stdcall  k() = 0;
    virtual void __stdcall  l() = 0;
};
class TaskManager
{
public:
    virtual int __stdcall  GetInterface(const /*Guid_t*/ void* guid, TaskManager** manager_interface) = 0;
    virtual int __stdcall  CreateTask(void* b) = 0;
    virtual void __stdcall  c() = 0;
};

class Ui
{
public:
    virtual int __stdcall  GetInterface(const /*Guid_t*/ void* guid, Ui** manager_interface) = 0;
    virtual void* __stdcall b() = 0;
    virtual bool __stdcall c() = 0;
    virtual bool __stdcall  d() = 0;
    virtual int __stdcall  e(void* b, void* c) = 0;
    virtual bool __stdcall  f() = 0;
    virtual int __stdcall  g(void* b) = 0;
    virtual bool __stdcall  h() = 0;
    virtual bool __stdcall  i(bool b) = 0;
    virtual int __stdcall  j() = 0;
    virtual int __stdcall  k() = 0;
    virtual int __stdcall  l(void* b) = 0;
    virtual SocialClubH* __stdcall m() = 0;
    virtual SocialClubI* __stdcall n() = 0;
    virtual int  __stdcall o(void* b, void* c, int d, int e) = 0;
    virtual int  __stdcall p() = 0;
    virtual int  __stdcall q(void* b) = 0;
    virtual int  __stdcall r(void* b, void* c) = 0;
    virtual void  __stdcall s(void* b) = 0;
    virtual bool  __stdcall t(void* b) = 0; //not sure about this ret type
    virtual void  __stdcall u(void* b, void* c) = 0;
    virtual int  __stdcall v() = 0;
    virtual void  __stdcall w(bool b) = 0;
    virtual bool  __stdcall x() = 0;
    virtual bool  __stdcall y(void* b) = 0;
    virtual void  __stdcall setUiScale(float scale) = 0;
    virtual bool __stdcall aa() = 0;
    virtual bool __stdcall ab() = 0;

    virtual bool __stdcall ac() = 0;
    virtual bool __stdcall ad() = 0;
    virtual bool __stdcall ae() = 0;
    virtual bool __stdcall af(void* b, void* c) = 0;
    virtual int __stdcall ag() = 0;
    virtual void __stdcall ah(void* b, void* c, void* d, void* e) = 0;
    virtual void __stdcall ai(void* b) = 0;
    virtual bool __stdcall aj() = 0;
    virtual void __stdcall ak() = 0;
    virtual void __stdcall al() = 0;
    virtual void __stdcall am() = 0;
    virtual void __stdcall an() = 0;
    virtual void __stdcall ao() = 0;
    virtual void  __stdcall ap() = 0;
    virtual void  __stdcall aq() = 0;
    virtual void  __stdcall ar() = 0;
    virtual void  __stdcall as() = 0;
    virtual void  __stdcall at() = 0;
    virtual void  __stdcall au() = 0;
    virtual void  __stdcall av() = 0;
    virtual void  __stdcall aw() = 0;
    virtual void  __stdcall ax() = 0;
    virtual void  __stdcall ay() = 0;
    virtual void  __stdcall az() = 0;
    virtual void __stdcall ba() = 0;
    virtual void __stdcall bb() = 0;
    virtual void __stdcall bc() = 0;
    virtual void __stdcall bd() = 0;
    virtual void __stdcall be() = 0;
    virtual void __stdcall bf() = 0;
    virtual void __stdcall bg() = 0;
    virtual void __stdcall bh() = 0;
    virtual void __stdcall bi() = 0;
};

class TelemetryManager
{
public:
    virtual int __stdcall  GetInterface(const /*Guid_t*/ void* guid, TelemetryManager** manager_interface) = 0;
    virtual void __stdcall  b() = 0;
    virtual void __stdcall  c() = 0;
    virtual void __stdcall  d() = 0;
    virtual void __stdcall  e() = 0;
    virtual void __stdcall  f() = 0;
    virtual void __stdcall  g() = 0;
    virtual void __stdcall  h() = 0;
    virtual void __stdcall  i() = 0;
    virtual void __stdcall  j() = 0;
    virtual void __stdcall  k() = 0;
    virtual void __stdcall  l() = 0;
    virtual void  __stdcall m() = 0;
    virtual void  __stdcall n() = 0;
    virtual void  __stdcall o() = 0;
    virtual void  __stdcall p() = 0;
    virtual void  __stdcall q() = 0;
    virtual void  __stdcall r() = 0;
    virtual void  __stdcall s() = 0;
    virtual void  __stdcall t() = 0;
    virtual void  __stdcall u() = 0;
};

class GamePadManager
{
public:
    virtual int __stdcall  GetInterface(const /*Guid_t*/ void* guid, GamePadManager** manager_interface) = 0;
    virtual int __stdcall  b(void* b, void* c) = 0;
    virtual void __stdcall  c() = 0;
    virtual void __stdcall  d() = 0;
    virtual void __stdcall  e(bool b) = 0;
    virtual void __stdcall  f() = 0;
    virtual void __stdcall  g() = 0;
    virtual void __stdcall  h() = 0;
};

class Network
{
public:
    virtual int __stdcall  GetInterface(const /*Guid_t*/ void* guid, Network** manager_interface) = 0;
    virtual void __stdcall  b(void* unknown_interface) = 0;
    virtual void __stdcall  c() = 0;
    virtual void __stdcall  d() = 0;
};

class CloudSave_g_param
{
    virtual int __stdcall  GetInterface(const /*Guid_t*/ void* guid, CloudSave_g_param** manager_interface) = 0;
    virtual char* __stdcall  b() = 0;
    virtual long __stdcall  c() = 0;
    virtual void* __stdcall  d() = 0;
    virtual CloudSave_g_param* __stdcall  e() = 0;
};

class CloudSaveG
{
    virtual int __stdcall  GetInterface(const /*Guid_t*/ void* guid, CloudSaveG** manager_interface) = 0;
    virtual void* __stdcall  b() = 0;
    virtual void __stdcall  c() = 0;
    virtual int __stdcall  d() = 0;
    virtual void* __stdcall  e() = 0;
    virtual void __stdcall  f() = 0;
    virtual void __stdcall  g() = 0;
    virtual void __stdcall  h() = 0;
    virtual void __stdcall  i() = 0;
    virtual void __stdcall  j() = 0;
    virtual void __stdcall  k() = 0;
    virtual void __stdcall  l() = 0;
    virtual void  __stdcall m() = 0;
    virtual void  __stdcall n() = 0;
    virtual void  __stdcall o() = 0;
    virtual void  __stdcall p() = 0;
    virtual void  __stdcall q() = 0;
    virtual void  __stdcall r() = 0;
    virtual void  __stdcall s() = 0;
    virtual void  __stdcall t() = 0;
    virtual void  __stdcall u() = 0;
    virtual void  __stdcall v() = 0;
    virtual void  __stdcall w() = 0;
};

class CloudSaveManager
{
public:
    virtual int __stdcall  GetInterface(const /*Guid_t*/ void* guid, CloudSaveManager** manager_interface) = 0;
    virtual void __stdcall  b(void* b, void* c) = 0;
    virtual void __stdcall  c() = 0;
    virtual void __stdcall  d() = 0;
    virtual void __stdcall  e() = 0;
    virtual void __stdcall  f() = 0;
    virtual CloudSaveG* __stdcall  g(CloudSave_g_param* b) = 0;
    virtual void __stdcall  h() = 0;
    virtual void __stdcall  i() = 0;
    virtual void __stdcall  j() = 0;
    virtual void __stdcall  k() = 0;
    virtual void __stdcall  l() = 0;
    virtual bool  __stdcall m(void* b, void* c, void* d) = 0;
    virtual void  __stdcall n() = 0;
    virtual void  __stdcall o() = 0;
    virtual void  __stdcall p() = 0;
    virtual void  __stdcall q() = 0;
    virtual void  __stdcall r() = 0;
    virtual void  __stdcall s() = 0;
    virtual void  __stdcall t() = 0;
    virtual void  __stdcall u() = 0;
    virtual void  __stdcall v() = 0;
    virtual void  __stdcall w() = 0;
    virtual void  __stdcall x() = 0;
    virtual void  __stdcall y() = 0;
    virtual void  __stdcall z() = 0;
    virtual void __stdcall aa() = 0;
    virtual void __stdcall ab() = 0;
    virtual void __stdcall ac() = 0;
};

class GamerPicManager
{
public:
    virtual int __stdcall  GetInterface(const /*Guid_t*/ void* guid, GamerPicManager** manager_interface) = 0;
    virtual void __stdcall  b() = 0;
    virtual void __stdcall  c() = 0;
    virtual void __stdcall  d() = 0;
    virtual void __stdcall  e() = 0;
    virtual void __stdcall  f() = 0;
};

enum class RgscEvent
{
    GameInviteAccepted = 1,
    FriendStatusChanged = 2,
    RosTicketChanged = 3, // arg: const char* with CreateTicketResponse
    SigninStateChanged = 4, // arg: int
    SocialClubMessage = 5,
    JoinedViaPresence = 6,
    SdkInitError = 7, // arg: int
    UiEvent = 8, // arg: const char* with json/xml/something
    OpenUrl = 9,
    UnkSteamUserOp = 10,
    UnkCommerce = 11
};

class IRgscDelegate
{
public:
    virtual HRESULT __stdcall QueryInterface(GUID* iid, void** out) = 0;
    virtual void* __stdcall m_01(void* a1, void* a2) = 0;
    virtual void* __stdcall m_02(void* a1) = 0; // implemented in gta, return 0?
    virtual void* __stdcall m_03(void* a1) = 0;
    virtual void* __stdcall m_04(void* a1) = 0;
    virtual void* __stdcall m_05(void* a1) = 0;
    virtual void* __stdcall m_06(bool a1, void* a2, void* a3, bool a4, uint32_t a5) = 0; // implemented in gta
    virtual void* __stdcall OnEvent(RgscEvent event, const void* data) = 0;
};

static bool signed_in;

struct FriendStatus
{
    uint32_t status;
    uint8_t pad[32 - 4 - 8];
    const char* jsonDataString;
    uint32_t jsonDataLength;
    uint32_t unk0x1000;
    uint32_t unk0x1;
    uint32_t unk0x7FFA;
};


class IRgsc
{
public:
    virtual /*error_t */ int __stdcall GetInterface(const /*Guid_t*/ void* guid, IRgsc** rgsc_interface) = 0;
    virtual void __stdcall b() = 0;
    virtual void __stdcall c() = 0;
    virtual void __stdcall d() = 0;
    virtual AchievementManager* __stdcall GetAchievementManager() = 0;
    virtual ProfileManager* __stdcall GetProfileManager() = 0;
    virtual FileSystem* __stdcall _GetFileSystem() = 0;
    virtual Ui* __stdcall GetUi() = 0;
    virtual int  __stdcall initialize(void* b, void* c, int d, IRgscDelegate* delegate) = 0;
    virtual PlayerManager* __stdcall GetPlayerManager() = 0;
    virtual TaskManager* __stdcall GetTaskManager() = 0;
    virtual PresenceManager* __stdcall GetPresenceManager() = 0;
    virtual CommerceManager* __stdcall GetCommerceManager() = 0;
    virtual void __stdcall n() = 0;
    virtual void __stdcall o() = 0;
    virtual TelemetryManager* __stdcall GetTelemetryManager() = 0;
    virtual void __stdcall q() = 0;
    virtual bool __stdcall r() = 0;
    virtual GamePadManager* __stdcall GetGamePadManager() = 0;
    virtual Network* __stdcall GetNetwork() = 0;
    virtual CloudSaveManager* __stdcall GetCloudSaveManager() = 0;
    virtual GamerPicManager* __stdcall GetGamerPicManager() = 0;
    virtual void __stdcall w() = 0;
    virtual void __stdcall x() = 0;
    virtual void __stdcall y() = 0;
    virtual void __stdcall z() = 0;
    virtual void __stdcall aa() = 0;
    virtual void __stdcall ab() = 0;
    virtual void __stdcall ac() = 0;
    virtual void __stdcall ad() = 0;
    virtual void __stdcall ae() = 0;
    virtual void __stdcall af() = 0;
    virtual void __stdcall ag() = 0;
    virtual void __stdcall ah() = 0;
    virtual void __stdcall ai() = 0;
    virtual void __stdcall aj() = 0;
};

class ProfileManagerImp : public ProfileManager
{
public:
    int test = 0;

    int __stdcall GetInterface(const /*Guid_t*/ void* guid, ProfileManager** profile_interface) {
        PRINT_DEBUG("%s %p %p %p\n", __FUNCTION__, this, guid, profile_interface);
        *profile_interface = this;
        return 0;
    }

    bool __stdcall b() {
        PRINT_DEBUG("%s %p\n", __FUNCTION__, this);
        return signed_in;
    }
    bool __stdcall isOnline() {
        PRINT_DEBUG("%s %p\n", __FUNCTION__, this);
        // return 0;
#ifdef ONLINE
        return 1;
#else
        return 0;
#endif
    }
    int __stdcall getProfileData(IProfileV3* profile) {
        PRINT_DEBUG("%s, %p %p\n", __FUNCTION__, this, profile);
        //profile->SetAccountId(ROS_DUMMY_ACCOUNT_ID);
        profile->SetAccountId(ROS_DUMMY_ACCOUNT_ID);
        profile->SetUsername(ROS_DUMMY_ACCOUNT_USERNAME);
        profile->SetBool1(false);
        uint8_t key[32] = {};
        profile->SetKey(key);
#ifdef ONLINE
        profile->SetEmail("gold@berg.org");
        profile->SetTicket("YWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFh");
        profile->SetUnkString("");
        profile->SetScAuthToken("AAAAArgQdyps/xBHKUumlIADBO75R0gAekcl3m2pCg3poDsXy9n7Vv4DmyEmHDEtv49b5BaUWBiRR/lVOYrhQpaf3FJCp4+22ETI8H0NhuTTijxjbkvDEViW9x6bOEAWApixmQue2CNN3r7X8vQ/wcXteChEHUHi");
#else
        profile->SetTicket("");
        profile->SetEmail("");
        profile->SetScAuthToken("");
        profile->SetUnkString("");
#endif
        return 0;
    }
    int __stdcall e() {
        PRINT_DEBUG("%s %p\n", __FUNCTION__, this);
        return 1;
    }

    bool __stdcall f() {
        PRINT_DEBUG("%s %p\n", __FUNCTION__, this);
#ifdef ONLINE
        return 1;
#else
        return 0;
#endif
    }

    bool __stdcall g() {
        PRINT_DEBUG("%s %p\n", __FUNCTION__, this);
        return 1;
    }

    void __stdcall h(void* b) {
        PRINT_DEBUG("%s %p %p %p\n", __FUNCTION__, b);
    }

    void __stdcall i(void* b, void* c, void* d) {
        PRINT_DEBUG("%s %p %p %p\n", __FUNCTION__, b, c, d);
    }

    void __stdcall j() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall k(void* b, void* c) {
        PRINT_DEBUG("%s %p %p\n", __FUNCTION__, b, c);
    }

    bool __stdcall l(void* b, void* c) {
        PRINT_DEBUG("%s %p %p\n", __FUNCTION__, b, c);
        return 1;
    }
};

class AchievementAImp : public AchievementA
{
    void a() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    int b() { PRINT_DEBUG("%s\n", __FUNCTION__); return 0; }
    void  c() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void  d() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void  e() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void  f() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void  g() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void  h() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void  i() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void  j() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void  k() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void  l() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void   m() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void   n() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void   o() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void   p() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void   q() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void   r() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void   s() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void   t() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void   u() { PRINT_DEBUG("%s\n", __FUNCTION__); }
};

class AchievementManagerImp : public AchievementManager
{
public:
    int test = 0;
    AchievementAImp ach_a;

    int __stdcall GetInterface(const /*Guid_t*/ void* guid, AchievementManager** manager_interface) {
        PRINT_DEBUG("%s %p %p %p\n", __FUNCTION__, this, guid, manager_interface);
        *manager_interface = this;
        return 0;
    }
    int __stdcall b(void* b, void* c, void* d, void* e, void* f, void* g, void* h, void* i, void* j) {
        PRINT_DEBUG("%s %p %p %p %p %p %p %p %p %p %p\n", __FUNCTION__, this, b, c, d, e, f, g, h, i, j);
        return 0;
    }

    int __stdcall c(void* b, void* c, void* d, AchievementA** e) {
        PRINT_DEBUG("%s %p %p %p %p\n", __FUNCTION__, b, c, d, e);
        *e = &ach_a;
        return 0;
    }

    int __stdcall d(int b) {
        PRINT_DEBUG("%s %i\n", __FUNCTION__, b);
        return 0;
    }

    int __stdcall e(void* b, void* c, void* d, void* e, void* f) {
        PRINT_DEBUG("%s %p %p %p %p %p\n", __FUNCTION__, b, c, d, e, f);
        return 0;
    }

    int __stdcall f(void* b, void* c, void* d) {
        PRINT_DEBUG("%s %p %p %p\n", __FUNCTION__, b, c, d);
        return 0;
    }

    int  __stdcall g() {
        PRINT_DEBUG("%s\n", __FUNCTION__);
        return -2147467259; // 0x80004005
    }
    void __stdcall h() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall i() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall j() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall k() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall l() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  m() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  n() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  o() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  p() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  q() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  r() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  s() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  t() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  u() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  v() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  w() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  x() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  y() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  z() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall aa() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall ab() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall ac() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall ad() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall ae() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall af() { PRINT_DEBUG("%s\n", __FUNCTION__); }
};

class PlayerManagerImp : public PlayerManager
{
public:
    int test = 0;

    int __stdcall GetInterface(const /*Guid_t*/ void* guid, PlayerManager** manager_interface) {
        PRINT_DEBUG("%s %p %p %p\n", __FUNCTION__, this, guid, manager_interface);
        *manager_interface = this;
        return 0;
    }
    int __stdcall b(void* b, void* c, void* d, void* e, void* f, void* g, void* h, void* i) {
        PRINT_DEBUG("%s %p %p %p %p %p %p %p %p\n", __FUNCTION__, this, b, c, d, e, f, g, h, i);
        return 0;
    }

    int __stdcall c(void* b, void* c, void* d, void* e, void* f) {
        PRINT_DEBUG("%s %p %p %p %p %p\n", __FUNCTION__, b, c, d, e, f);
        return 0;
    }

    bool __stdcall d(void* b, void* c) {
        PRINT_DEBUG("%s %p %p\n", __FUNCTION__, b, c);
        return 1;
    }

    int __stdcall e(void* b, void* c) {
        PRINT_DEBUG("%s %p %p\n", __FUNCTION__, b, c);
        return 0;
    }

    int __stdcall f(void* b, void* c) {
        PRINT_DEBUG("%s %p %p\n", __FUNCTION__, b, c);
        return 0;
    }

    int __stdcall g(void* b, void* c, void* d) {
        PRINT_DEBUG("%s %p %p %p\n", __FUNCTION__, b, c, d);
        return 1;
    }

    bool __stdcall h(void* b, void* c) {
        PRINT_DEBUG("%s %p %p\n", __FUNCTION__, b, c);
        return 1;
    }

    int __stdcall i() {
        PRINT_DEBUG("%s\n", __FUNCTION__);
        return 0;
    }
    bool __stdcall j() {
        PRINT_DEBUG("%s\n", __FUNCTION__);
        return 1;
    }
    bool __stdcall k() {
        PRINT_DEBUG("%s\n", __FUNCTION__);
        return 1;
    }
    int __stdcall l() {
        PRINT_DEBUG("%s\n", __FUNCTION__);
        return 0;
    }
};

class PresenceManagerImp : public PresenceManager
{
public:
    int test = 0;

    int __stdcall GetInterface(const /*Guid_t*/ void* guid, PresenceManager** presence_interface) {
        PRINT_DEBUG("%s %p %p %p\n", __FUNCTION__, this, guid, presence_interface);
        *presence_interface = this;
        return 0;
    }

    bool __stdcall b(void* b, char* c, void* d, void* e) {
        PRINT_DEBUG("%s %p %s %p %p\n", __FUNCTION__, b, c, d, e);
        return 0;

    }
    bool __stdcall c(void* b, void* c, void* d, void* e) {
        PRINT_DEBUG("%s %p %s %p %p\n", __FUNCTION__, b, c, d, e);
        return 0;
    }
    bool __stdcall d(void* b, char* key, char* value) {
        PRINT_DEBUG("%s %p %s %s\n", __FUNCTION__, b, key, value);
        return 0;
        //todo: ret value
    }
    bool __stdcall e(void* b, void* c, void* d) {
        PRINT_DEBUG("%s %p %s %p %p\n", __FUNCTION__, b, c, d);
        return 0;
    }
    bool __stdcall f(void* b, void* c, void* d) {
        PRINT_DEBUG("%s %p %s %p %p\n", __FUNCTION__, b, c, d);
        return 0;
    }
    bool __stdcall g(void* b, void* c, void* d, void* e) {
        PRINT_DEBUG("%s %p %s %p %p\n", __FUNCTION__, b, c, d, e);
        return 0;
    }

    bool __stdcall h(void* b, void* c, void* d, void* e, void* f) {
        PRINT_DEBUG("%s %p %p %p %p %p\n", __FUNCTION__, b, c, d, e, f);
        return 0;
    }

    bool __stdcall i(void* b, void* c, void* d, void* e, void* f, void* g) {
        PRINT_DEBUG("%s %p %p %p %p %p %p\n", __FUNCTION__, b, c, d, e, f, g);
        return 0;
    }

    bool __stdcall j(void* b, void* c, void* d, void* e, void* f, void* g, void* h, void* i, void* j, void* k) {
        PRINT_DEBUG("%s %p %p %p %p %p %p %p %p %p %p\n", __FUNCTION__, b, c, d, e, f, g, h, i, j, k);
        return 0;
    }

    bool __stdcall k(void* b, void* c, void* d, void* e) {
        PRINT_DEBUG("%s %p %s %p %p\n", __FUNCTION__, b, c, d, e);
        return 0;
    }

    bool __stdcall l(void* b, void* c, void* d, void* e) {
        PRINT_DEBUG("%s %p %p %p %p %p\n", __FUNCTION__, this, b, c, d, e);
        return true; //No idea what this should return
    }

    bool __stdcall m(void* b) {
        PRINT_DEBUG("%s %p %p\n", __FUNCTION__, this, b);
        return 0;
    }

    bool __stdcall n(void* b) {
        PRINT_DEBUG("%s %p %p\n", __FUNCTION__, this, b);
        return 0;
    }

    bool __stdcall o(void* b) {
        PRINT_DEBUG("%s %p %p\n", __FUNCTION__, this, b);
        return 0;
    }
    bool __stdcall p(void* b) {
        PRINT_DEBUG("%s %p %p\n", __FUNCTION__, this, b);
        return 0;
    }
    bool __stdcall  q(void* b, void* c, void* d) {
        PRINT_DEBUG("%s %p %s %p %p\n", __FUNCTION__, b, c, d);
        return 0;
    }
    bool __stdcall  r(void* b, void* c, void* d) {
        PRINT_DEBUG("%s %p %s %p %p\n", __FUNCTION__, b, c, d);
        return 0;
    }
    bool __stdcall  s(void* b) {
        PRINT_DEBUG("%s %p %p\n", __FUNCTION__, this, b);
        return 0;
    }
    bool __stdcall  t(void* b, void* c, void* d, void* e, void* f, void* g) {
        PRINT_DEBUG("%s %p %p %p %p %p %p\n", __FUNCTION__, b, c, d, e, f, g);
        return 0;
    }

    bool __stdcall  u(void* b) {
        PRINT_DEBUG("%s %p %p\n", __FUNCTION__, this, b);
        return 0;
    }

    bool __stdcall  v(void* b) {
        PRINT_DEBUG("%s %p %p\n", __FUNCTION__, this, b);
        return 0;
    }

    bool __stdcall  w(void* b, void* c, void* d, void* e) {
        PRINT_DEBUG("%s %p %s %p %p\n", __FUNCTION__, b, c, d, e);
        return 0;
    }

    bool __stdcall  x(void* b) {
        PRINT_DEBUG("%s %p %p\n", __FUNCTION__, this, b);
        return 0;
    }
    bool __stdcall  y(void* b) {
        PRINT_DEBUG("%s %p %p\n", __FUNCTION__, this, b);
        //set something = b
        return true;
    }
    bool __stdcall  z(void* b) {
        PRINT_DEBUG("%s %p %p\n", __FUNCTION__, this, b);
        //note: b is interface to something most likely, first function gets called
        return true;
    }
};

class CommerceManagerImp : public CommerceManager
{
public:
    int test = 0;

    int __stdcall GetInterface(const /*Guid_t*/ void* guid, CommerceManager** manager_interface) {
        PRINT_DEBUG("%s %p %p %p\n", __FUNCTION__, this, guid, manager_interface);
        *manager_interface = this;
        return 0;
    }

    int __stdcall b(void* b) {
        PRINT_DEBUG("%s %p %p\n", __FUNCTION__, this, b);
        return 0;
    }
    int __stdcall c(void* b) {
        PRINT_DEBUG("%s %p %p\n", __FUNCTION__, this, b);
        return 0;
    }
    int __stdcall d(void* b, void* c, void* d) {
        PRINT_DEBUG("%s %p %s %p %p\n", __FUNCTION__, b, c, d);
        return 0;
    }
    void __stdcall e(void* b, void* c) {
        PRINT_DEBUG("%s %p %p\n", __FUNCTION__, b, c);
    }

    int __stdcall f(void* b, void* c, void* d, void* e) {
        PRINT_DEBUG("%s %p %s %p %p\n", __FUNCTION__, b, c, d, e);
        return 0;
    }
    void __stdcall g(void* b) {
        PRINT_DEBUG("%s %p %p\n", __FUNCTION__, this, b);
    }

    bool __stdcall h() {
        PRINT_DEBUG("%s %p\n", __FUNCTION__, this);
        //in mp3 when 0 check for steam dlc
        //when 1 don't check for steam dlc
        return 0;
    }

    bool __stdcall i() {
        PRINT_DEBUG("%s %p\n", __FUNCTION__, this);
        return 1;
    }
    bool __stdcall j(void* b, void* c, void* d, void* e, void* f, void* g, void* h, void* i) {
        PRINT_DEBUG("%s %p %p %p %p %p %p %p %p\n", __FUNCTION__, b, c, d, e, f, g, h, i);
        return true;
    }
    bool __stdcall k(void* b, void* c, void* d) {
        PRINT_DEBUG("%s %p %s %p %p\n", __FUNCTION__, b, c, d);
        return true;
    }
    bool __stdcall l(char* str, AsyncTask* task) {
        PRINT_DEBUG("%s %p %s %p\n", __FUNCTION__, this, str, task);
        return true;
    }
    bool __stdcall DLC_check_2(char* dlc_name) {
        PRINT_DEBUG("%s %p %s\n", __FUNCTION__, this, dlc_name);
        if (!dlc_name || !dlc_name[0]) return false;
        return true;
    }

    bool __stdcall DLC_check(char* dlc_name, bool* c, bool* d, bool* e) {
        PRINT_DEBUG("%s %p %s %p %p %p\n", __FUNCTION__, this, dlc_name, c, d, e);
        //if (!dlc_name || !dlc_name[0]) return false;
        if (c) *c = true;
        if (d) *d = true;
        if (e) *e = true; // not sure about e
        return true;
    }

    bool __stdcall o(void* b, void* c, void* d) {
        PRINT_DEBUG("%s %p %s %p %p\n", __FUNCTION__, b, c, d);
        return true;
    }
    bool __stdcall p(void* b, void* c, void* d, void* e) {
        PRINT_DEBUG("%s %p %s %p %p\n", __FUNCTION__, b, c, d, e);
        return true;
    }
    void __stdcall  q() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  r() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  s() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  t() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  u() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  v(void* b) {
        PRINT_DEBUG("%s %p\n", __FUNCTION__, b);
    }
    void __stdcall  w() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  x() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  y() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  z() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall aa() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall ab() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall ac() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall ad() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall ae() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall af() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall ag() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall ah() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall ai() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall aj() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall ak() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall al() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall am() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall an() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall ao() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall ap() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall aq() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall ar() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall as() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall at() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall au() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall av() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall aw() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall ax() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall ay() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall az() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall ba() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall bb() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall bc() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall bd() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall be() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall bf() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall bg() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall bh() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall bi() { PRINT_DEBUG("%s\n", __FUNCTION__); }
};

class FileSystemImp : public FileSystem
{
public:
    int test = 0;
    std::wstring _folder = get_game_save_path();

    int __stdcall GetInterface(const /*Guid_t*/ void* guid, FileSystem** manager_interface) {
        PRINT_DEBUG("%s %p %p %p\n", __FUNCTION__, this, guid, manager_interface);
        *manager_interface = this;
        return 0;
    }

    int __stdcall b(void* b, void* c) {
        PRINT_DEBUG("%s %p %p\n", __FUNCTION__, b, c);
        return 0;
    }

    int __stdcall getProfilePath(char* buffer, int flag) {
        PRINT_DEBUG("%s %p %i\n", __FUNCTION__, buffer, flag);
        std::string folder = toUTF8(_folder);
        folder.copy(buffer, 256);
        buffer[folder.size()] = 0;
        return 0;
    }

    int __stdcall d(void* b) {
        PRINT_DEBUG("%s %p %p\n", __FUNCTION__, b);
        return 0;
    }
    int __stdcall e(void* b) {
        PRINT_DEBUG("%s %p %p\n", __FUNCTION__, b);
        return 0;
    }
    int __stdcall f(void* b, void* c) {
        PRINT_DEBUG("%s %p %p\n", __FUNCTION__, b, c);
        return 0;
    }
    void __stdcall g(bool b) { PRINT_DEBUG("%s %u\n", __FUNCTION__, b); }
    void __stdcall h() { PRINT_DEBUG("%s\n", __FUNCTION__); }
};

class SocialClubHImp : public SocialClubH
{
public:
    int test = 0;

    int __stdcall GetInterface(const /*Guid_t*/ void* guid, SocialClubH** manager_interface) {
        PRINT_DEBUG("%s %p %p %p\n", __FUNCTION__, this, guid, manager_interface);
        *manager_interface = this;
        return 0;
    }

    void __stdcall b() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall c() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall d() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall e() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall f() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall g() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall h() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall i() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall j() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall k() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall l() { PRINT_DEBUG("%s\n", __FUNCTION__); }
};

class SocialClubIImp : public SocialClubI
{
public:
    int test = 0;

    int __stdcall GetInterface(const /*Guid_t*/ void* guid, SocialClubI** manager_interface) {
        PRINT_DEBUG("%s %p %p %p\n", __FUNCTION__, this, guid, manager_interface);
        *manager_interface = this;
        return 0;
    }

    void __stdcall b() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall c() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall d() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall e() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall f() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall g() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall h() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall i() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall j() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall k() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall l() { PRINT_DEBUG("%s\n", __FUNCTION__); }
};

class AsyncTaskImp : public AsyncTask
{
public:
    int test = 0;

    int __stdcall GetInterface(const /*Guid_t*/ void* guid, AsyncTask** manager_interface) {
        PRINT_DEBUG("%s %p %p %p\n", __FUNCTION__, this, guid, manager_interface);
        *manager_interface = this;
        return 0;
    }
    void __stdcall b() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    bool __stdcall c() {
        PRINT_DEBUG("%s %p\n", __FUNCTION__, this);
        //call function if 0?
        //return [[a] + 4] - 1
        return 0;
    }

    void __stdcall d(void* b, void* c) {
        PRINT_DEBUG("%s %p %p %p\n", __FUNCTION__, this, b, c);
        //ecx = [a]
        //[ecx + 0x10] = b
        //[ecx + 0x14] = c
        return;
    }

    int __stdcall e() {
        PRINT_DEBUG("%s %p\n", __FUNCTION__, this);
        //return [[a] + 0xC]
        return 0;
    }

    void __stdcall f() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    bool __stdcall g() {
        PRINT_DEBUG("%s %p\n", __FUNCTION__, this);
        //return [[a] + 8] == 1
        return false;
    }

    bool __stdcall h() {
        PRINT_DEBUG("%s %p\n", __FUNCTION__, this);
        //return [[a] + 8] == 3
        return false;
    }

    bool __stdcall i() {
        PRINT_DEBUG("%s %p\n", __FUNCTION__, this);
        //return [[a] + 8] == 2
        return false;
    }

    bool __stdcall j() {
        PRINT_DEBUG("%s %p\n", __FUNCTION__, this);
        //return [[a] + 8] == 4
        return false;
    }

    void __stdcall k() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall l() { PRINT_DEBUG("%s\n", __FUNCTION__); }
};

class TaskManagerImp : public TaskManager
{
public:
    int test = 0;

    int __stdcall GetInterface(const /*Guid_t*/ void* guid, TaskManager** manager_interface) {
        PRINT_DEBUG("%s %p %p %p\n", __FUNCTION__, this, guid, manager_interface);
        *manager_interface = this;
        return 0;
    }
    int __stdcall CreateTask(void* b) {
        PRINT_DEBUG("%s %p %p\n", __FUNCTION__, this, b);
        static AsyncTask* k_imp = new AsyncTaskImp();
        //write pointer to interface? something to b
        memcpy(b, &k_imp, sizeof(k_imp));
        return 0;
    }
    void __stdcall c() { PRINT_DEBUG("%s\n", __FUNCTION__); }
};

class UiImp : public Ui
{
public:
    int test = 0;
    SocialClubHImp sc_h;
    SocialClubIImp sc_i;

    int __stdcall GetInterface(const /*Guid_t*/ void* guid, Ui** manager_interface) {
        PRINT_DEBUG("%s %p %p %p\n", __FUNCTION__, this, guid, manager_interface);
        *manager_interface = this;
        return 0;
    }

    void* __stdcall b() {
        PRINT_DEBUG("%s %p\n", __FUNCTION__, this);
        return NULL; //returns interface pointer
    }

    bool __stdcall  c() {
        //0 seems to be the right value
        PRINT_DEBUG("%s %p\n", __FUNCTION__, this);
        return 0;
    }
    bool __stdcall  d() {
        PRINT_DEBUG("%s\n", __FUNCTION__);
        return 0;
    }
    int __stdcall  e(void* b, void* c) {
        PRINT_DEBUG("%s %p %p %p\n", __FUNCTION__, this, b, c);
        return 0;
    }

    bool __stdcall  f() {
        PRINT_DEBUG("%s %p\n", __FUNCTION__, this);
        return 0;
    }

    int __stdcall  g(void* b) {
        //on window resize
        PRINT_DEBUG("%s %p %p\n", __FUNCTION__, this, b);
        return 0;
    }

    bool __stdcall  h() {
        PRINT_DEBUG("%s %p\n", __FUNCTION__, this);
        return true; //must return true or mp3 gets stuck checking activation
    }
    bool __stdcall  i(bool b) {
        PRINT_DEBUG("%s %u\n", __FUNCTION__, b);
        return b;//no idea if this is good
    }
    int __stdcall  j() {
        //not sure about this function
        PRINT_DEBUG("%s %p\n", __FUNCTION__, this);
        return 0;
    }
    int __stdcall  k() {
        PRINT_DEBUG("%s %p\n", __FUNCTION__, this);
        return 0;
    }
    int __stdcall  l(void* b) {
        PRINT_DEBUG("%s %u\n", __FUNCTION__, b);
        return -2147467259; // 0x80004005
    }

    SocialClubH* __stdcall m() {
        PRINT_DEBUG("%s %p\n", __FUNCTION__, this);
        return &sc_h;
    }

    SocialClubI* __stdcall n() {
        PRINT_DEBUG("%s %p\n", __FUNCTION__, this);
        return &sc_i;
    }

    int __stdcall  o(void* b, void* c, int d, int e) {
        PRINT_DEBUG("%s %p %p %p %i %i\n", __FUNCTION__, this, b, c, d, e);
        return 0;
    }

    int __stdcall  p() {
        PRINT_DEBUG("%s\n", __FUNCTION__);
        //bool to int?
        return 0;
    }

    int __stdcall  q(void* b) {
        PRINT_DEBUG("%s %u\n", __FUNCTION__, b);
        return 0;
    }
    int __stdcall  r(void* b, void* c) {
        PRINT_DEBUG("%s %p %p %p\n", __FUNCTION__, this, b, c);
        return 0;
    }
    void __stdcall  s(void* b) {
        PRINT_DEBUG("%s %u\n", __FUNCTION__, b);
    }
    bool __stdcall  t(void* b) {
        PRINT_DEBUG("%s %u\n", __FUNCTION__, b);
        return 1;
    }
    void __stdcall  u(void* b, void* c) {
        PRINT_DEBUG("%s %p %p %p\n", __FUNCTION__, this, b, c);
    }

    int __stdcall  v() {
        PRINT_DEBUG("%s\n", __FUNCTION__);
        //bool to int?
        return 0;
    }
    void __stdcall  w(bool b) {
        PRINT_DEBUG("%s %u\n", __FUNCTION__, b);
    }

    bool __stdcall  x() {
        PRINT_DEBUG("%s %p\n", __FUNCTION__, this);
        return 0;
    }
    bool __stdcall  y(void* b) {
        PRINT_DEBUG("%s %u\n", __FUNCTION__, b);
        return 1;
    }
    void __stdcall  setUiScale(float scale) {
        PRINT_DEBUG("%s %f\n", __FUNCTION__, scale);
    }
    bool __stdcall aa() {
        PRINT_DEBUG("%s\n", __FUNCTION__);
        return 1;
    }
    bool __stdcall ab() {
        PRINT_DEBUG("%s\n", __FUNCTION__);
        return 0;
    }


    bool __stdcall ac() {
        PRINT_DEBUG("%s\n", __FUNCTION__);
        return 0;
    }
    bool __stdcall ad() {
        PRINT_DEBUG("%s\n", __FUNCTION__);
        return 0;
    }
    bool __stdcall ae() {
        PRINT_DEBUG("%s\n", __FUNCTION__);
        return 0;
    }
    bool __stdcall af(void* b, void* c) {
        PRINT_DEBUG("%s %p %p %p\n", __FUNCTION__, this, b, c);
        return 0;
    }
    int __stdcall ag() {
        PRINT_DEBUG("%s\n", __FUNCTION__);
        //bool to int?
        return 0;
    }
    void __stdcall ah(void* b, void* c, void* d, void* e) {
        PRINT_DEBUG("%s %p %s %p %p\n", __FUNCTION__, b, c, d, e);
    }
    void __stdcall ai(void* b) {
        PRINT_DEBUG("%s %u\n", __FUNCTION__, b);
    }
    bool __stdcall aj() {
        PRINT_DEBUG("%s\n", __FUNCTION__);
        return 0;
    }

    void __stdcall ak() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall al() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall am() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall an() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall ao() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall ap() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall aq() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall ar() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall as() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall at() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall au() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall av() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall aw() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall ax() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall ay() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall az() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall ba() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall bb() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall bc() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall bd() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall be() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall bf() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall bg() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall bh() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall bi() { PRINT_DEBUG("%s\n", __FUNCTION__); }

};

class TelemetryManagerImp : public TelemetryManager
{
public:
    int test = 0;

    int __stdcall GetInterface(const /*Guid_t*/ void* guid, TelemetryManager** manager_interface) {
        PRINT_DEBUG("%s %p %p %p\n", __FUNCTION__, this, guid, manager_interface);
        *manager_interface = this;
        return 0;
    }

    void __stdcall b() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall c() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall d() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall e() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall f() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall g() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall h() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall i() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall j() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall k() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall l() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  m() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  n() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  o() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  p() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  q() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  r() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  s() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  t() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  u() { PRINT_DEBUG("%s\n", __FUNCTION__); }
};

class GamePadManagerImp : public GamePadManager
{
public:
    int test = 0;

    int __stdcall GetInterface(const /*Guid_t*/ void* guid, GamePadManager** manager_interface) {
        PRINT_DEBUG("%s %p %p %p\n", __FUNCTION__, this, guid, manager_interface);
        *manager_interface = this;
        return 0;
    }

    int __stdcall b(void* b, void* c) {
        PRINT_DEBUG("%s %p %p\n", __FUNCTION__, b, c);
        return 0;
    }

    void __stdcall c() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall d() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall e(bool b) {
        PRINT_DEBUG("%s %u\n", __FUNCTION__, b);
    }
    void __stdcall f() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall g() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall h() { PRINT_DEBUG("%s\n", __FUNCTION__); }
};

class NetworkImp : public Network
{
public:
    int test = 0;

    int __stdcall GetInterface(const /*Guid_t*/ void* guid, Network** manager_interface) {
        PRINT_DEBUG("%s %p %p %p\n", __FUNCTION__, this, guid, manager_interface);
        *manager_interface = this;
        return 0;
    }

    void __stdcall b(void* unknown_interface) {
        PRINT_DEBUG("%s %p %p\n", __FUNCTION__, unknown_interface);
    }
    void __stdcall c() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall d() { PRINT_DEBUG("%s\n", __FUNCTION__); }
};

class CloudSaveGImp : public CloudSaveG
{
public:
    int __stdcall GetInterface(const /*Guid_t*/ void* guid, CloudSaveG** manager_interface) {
        PRINT_DEBUG("%s %p %p %p\n", __FUNCTION__, this, guid, manager_interface);
        *manager_interface = this;
        return 0;
    }
    void* __stdcall b() { PRINT_DEBUG("%s\n", __FUNCTION__); return NULL; }
    void __stdcall c() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    int __stdcall d() { PRINT_DEBUG("%s\n", __FUNCTION__); return 0; }
    void* __stdcall e() { PRINT_DEBUG("%s\n", __FUNCTION__); return NULL; }
    void __stdcall f() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall g() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall h() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall i() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall j() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall k() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall l() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  m() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  n() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  o() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  p() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  q() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  r() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  s() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  t() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  u() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  v() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  w() { PRINT_DEBUG("%s\n", __FUNCTION__); }
};

class CloudSaveManagerImp : public CloudSaveManager
{
public:
    int test = 0;
    CloudSaveGImp g_imp;

    int __stdcall GetInterface(const /*Guid_t*/ void* guid, CloudSaveManager** manager_interface) {
        PRINT_DEBUG("%s %p %p %p\n", __FUNCTION__, this, guid, manager_interface);
        *manager_interface = this;
        return 0;
    }

    void __stdcall b(void* b, void* c) {
        PRINT_DEBUG("%s %p %p\n", __FUNCTION__, b, c);
    }
    void __stdcall c() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall d() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall e() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall f() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    CloudSaveG* __stdcall g(CloudSave_g_param* b) {
        PRINT_DEBUG("%s %p\n", __FUNCTION__, b);
        return &g_imp;
    }
    void __stdcall h() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall i() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall j() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall k() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall l() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    bool __stdcall  m(void* b, void* c, void* d) {
        PRINT_DEBUG("%s %p %p %p\n", __FUNCTION__, b, c, d);
        return 0;
    }
    void __stdcall  n() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  o() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  p() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  q() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  r() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  s() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  t() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  u() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  v() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  w() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  x() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  y() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall  z() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall aa() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall ab() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall ac() { PRINT_DEBUG("%s\n", __FUNCTION__); }
};

class GamerPicManagerImp : public GamerPicManager
{
public:
    int test = 0;

    int __stdcall GetInterface(const /*Guid_t*/ void* guid, GamerPicManager** manager_interface) {
        PRINT_DEBUG("%s %p %p %p\n", __FUNCTION__, this, guid, manager_interface);
        *manager_interface = this;
        return 0;
    }

    void __stdcall b() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall c() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall d() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall e() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall f() { PRINT_DEBUG("%s\n", __FUNCTION__); }
};

#ifdef ONLINE
#include "tinyxml2.h"
std::string GetRockstarTicketXml()
{
    // generate initial XML to be contained by JSON
    tinyxml2::XMLDocument document;

    auto rootElement = document.NewElement("Response");
    document.InsertFirstChild(rootElement);

    // set root attributes
    rootElement->SetAttribute("ms", 30.0);
    rootElement->SetAttribute("xmlns", "CreateTicketResponse");

    // elements
    auto appendChildElement = [&](tinyxml2::XMLNode* node, const char* key, auto value)
    {
        auto element = document.NewElement(key);
        element->SetText(value);

        node->InsertEndChild(element);

        return element;
    };

    auto appendElement = [&](const char* key, auto value)
    {
        return appendChildElement(rootElement, key, value);
    };

    char account_id[32] = {};
    sprintf(account_id, "%lld", ROS_DUMMY_ACCOUNT_ID);

    // create the document
    appendElement("Status", 1);
    appendElement("Ticket", "YWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFh"); // 'a' repeated
    appendElement("PosixTime", static_cast<unsigned int>(time(nullptr)));
    appendElement("SecsUntilExpiration", 86399);
    appendElement("PlayerAccountId", account_id);
    appendElement("PublicIp", "127.0.0.1");
    appendElement("SessionId", 5);
    appendElement("SessionKey", "MDEyMzQ1Njc4OWFiY2RlZg=="); // '0123456789abcdef'
    appendElement("SessionTicket", "vhASmPR0NnA7MZsdVCTCV/3XFABWGa9duCEscmAM0kcCDVEa7YR/rQ4kfHs2HIPIttq08TcxIzuwyPWbaEllvQ==");
    appendElement("CloudKey", "8G8S9JuEPa3kp74FNQWxnJ5BXJXZN1NFCiaRRNWaAUR=");

    // services
    auto servicesElement = appendElement("Services", "");
    servicesElement->SetAttribute("Count", 0);

    // Rockstar account
    tinyxml2::XMLNode* rockstarElement = appendElement("RockstarAccount", "");
    appendChildElement(rockstarElement, "RockstarId", account_id);
    appendChildElement(rockstarElement, "Age", 18);
    appendChildElement(rockstarElement, "AvatarUrl", "Bully/b20.png");
    appendChildElement(rockstarElement, "CountryCode", "CA");
    appendChildElement(rockstarElement, "Email", "gold@berg.org");
    appendChildElement(rockstarElement, "LanguageCode", "en");
    appendChildElement(rockstarElement, "Nickname", ROS_DUMMY_ACCOUNT_USERNAME);

    appendElement("Privileges", "1,2,3,4,5,6,8,9,10,11,14,15,16,17,18,19,21,22,27");

    auto privsElement = appendElement("Privs", "");
    auto privElement = appendChildElement(privsElement, "p", "");
    privElement->SetAttribute("id", "27");
    privElement->SetAttribute("g", "True");

    // format as string
    tinyxml2::XMLPrinter printer;
    document.Print(&printer);

    return printer.CStr();
}
#endif

class IRgscImp : public IRgsc
{
    ProfileManagerImp profile_manager;
    AchievementManagerImp achievement_manager;
    FileSystemImp file_system;
    UiImp ui;
    PlayerManagerImp player_manager;
    TaskManagerImp task_manager;
    PresenceManagerImp presence_manager;
    CommerceManagerImp commerce_manager;
    TelemetryManagerImp telemetry_manager;
    GamePadManagerImp gamepad_manager;
    NetworkImp network_manager;
    CloudSaveManagerImp cloudsave_manager;
    GamerPicManagerImp gamerpic_manager;

    IRgscDelegate* delegate = NULL;
    bool call_sign_in = false;


public:

    IRgscImp()
    {
        static bool run_once = false;

#ifdef CUSTOM_IDS
        if (!run_once) {
            // Acount name
            char name[32] = {};
            if (get_data_settings(L"account_name.txt", name, sizeof(name) - 1) <= 0) {
                store_data_settings(L"account_name.txt", ROS_DUMMY_ACCOUNT_USERNAME, strlen(ROS_DUMMY_ACCOUNT_USERNAME));
            }
            else {
                if (!name[1]) name[1] = ' ';
                if (!name[2]) name[2] = ' ';
                strcpy(ROS_DUMMY_ACCOUNT_USERNAME, name);
            }

            char array_rs_id[32] = {};
            bool generate_new = false;
            uint64_t user_id;

            if (get_data_settings(L"rs_id.txt", array_rs_id, sizeof(array_rs_id) - 1) > 0) {
                user_id = ((uint64_t)std::atoll(array_rs_id)) & 0xFFFFFFF;
                if (!(user_id)) {
                    generate_new = true;
                }
            }
            else {
                generate_new = true;
            }

            if (generate_new) {
                user_id = (((uint64_t)this) ^ ((uint64_t)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count())) & 0xFFFFFFF;
                if (!user_id) ++user_id;
                char temp_text[32] = {};
                snprintf(temp_text, sizeof(temp_text), "%llu", user_id);
                store_data_settings(L"rs_id.txt", temp_text, strlen(temp_text));
            }

            ROS_DUMMY_ACCOUNT_ID = user_id;
            PRINT_DEBUG("name: %s, user_id: %X\n", ROS_DUMMY_ACCOUNT_USERNAME, ROS_DUMMY_ACCOUNT_ID);
            run_once = true;
        }

#endif
    }

    int __stdcall GetInterface(const /*Guid_t*/ void* guid, IRgsc** rgsc_interface) {
        PRINT_DEBUG("%s %p | %p %p\n", __FUNCTION__, this, guid, rgsc_interface);
        *rgsc_interface = this;
        return 0;
    }
    void __stdcall b() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall c() {
        //cleanup function?
        PRINT_DEBUG("%s %p\n", __FUNCTION__, this);
    }
    void __stdcall d() {
        PRINT_DEBUG("%s %p\n", __FUNCTION__, this);
        PRINT_DEBUG("%u %p\n", signed_in, this->delegate);

        if (call_sign_in) {
            PRINT_DEBUG("%s Sign In\n", __FUNCTION__);
            int zero = 0;
            this->delegate->OnEvent((RgscEvent)0xD, &zero);

            int changed = 1;
            this->delegate->OnEvent(RgscEvent::FriendStatusChanged, &changed);

#ifdef ONLINE
            FriendStatus fs = { 0 };
            fs.status = 5;
            fs.unk0x1 = 1;
            fs.unk0x1000 = 0x1000;
            fs.unk0x7FFA = 0x7FFA;
            fs.jsonDataLength = 0x1D0;
            fs.jsonDataString = R"({"SignedIn":true,"SignedOnline":true,"ScAuthToken":"AAAAArgQdyps/xBHKUumlIADBO75R0gAekcl3m2pCg3poDsXy9n7Vv4DmyEmHDEtv49b5BaUWBiRR/lVOYrhQpaf3FJCp4+22ETI8H0NhuTTijxjbkvDEViW9x6bOEAWApixmQue2CNN3r7X8vQ/wcXteChEHUHi","ScAuthTokenError":false,"ProfileSaved":true,"AchievementsSynced":false,"FriendsSynced":false,"Local":false,"NumFriendsOnline":0,"NumFriendsPlayingSameTitle":0,"NumBlocked":0,"NumFriends":0,"NumInvitesReceieved":0,"NumInvitesSent":0,"CallbackData":2})";

            delegate->OnEvent(RgscEvent::FriendStatusChanged, &fs);

            delegate->OnEvent(RgscEvent::RosTicketChanged, GetRockstarTicketXml().c_str());

            int yes = 1;
            this->delegate->OnEvent(RgscEvent::SigninStateChanged, &yes);
#endif
            signed_in = true;
            call_sign_in = false;
        }

        if (!call_sign_in && !signed_in && (this->delegate != nullptr)) {
            call_sign_in = true;
        }
    }
    AchievementManager* __stdcall GetAchievementManager() {
        PRINT_DEBUG("%s %p\n", __FUNCTION__, this);
        return &achievement_manager;
    }
    ProfileManager* __stdcall GetProfileManager() {
        PRINT_DEBUG("%s %p\n", __FUNCTION__, this);
        return &profile_manager;
    }
    FileSystem* __stdcall _GetFileSystem() {
        PRINT_DEBUG("%s %p\n", __FUNCTION__, this);
        return &file_system;
    }
    Ui* __stdcall GetUi() {
        PRINT_DEBUG("%s %p\n", __FUNCTION__, this);
        return &ui;
    }

    int __stdcall initialize(void* b, void* c, int d, IRgscDelegate* dele) {
        PRINT_DEBUG("%s %p %p %p %i %p\n", __FUNCTION__, this, b, c, d, dele);
        this->delegate = dele;
        return 0;
    }

    PlayerManager* __stdcall GetPlayerManager() {
        PRINT_DEBUG("%s %p\n", __FUNCTION__, this);
        return &player_manager;
    }

    TaskManager* __stdcall GetTaskManager() {
        PRINT_DEBUG("%s %p\n", __FUNCTION__, this);
        return &task_manager;
    }

    PresenceManager* __stdcall GetPresenceManager() {
        PRINT_DEBUG("%s %p\n", __FUNCTION__, this);
        return &presence_manager;
    }

    CommerceManager* __stdcall GetCommerceManager() {
        PRINT_DEBUG("%s %p\n", __FUNCTION__, this);
        return &commerce_manager;
    }

    void __stdcall n() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall o() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    TelemetryManager* __stdcall GetTelemetryManager() {
        PRINT_DEBUG("%s %p\n", __FUNCTION__, this);
        return &telemetry_manager;
    }
    void __stdcall q() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    bool __stdcall r() {
        //gets called on exit, gta5 doesn't exist if it isnt't 1
        PRINT_DEBUG("%s %p\n", __FUNCTION__, this);
        return 1;
    }
    GamePadManager* __stdcall GetGamePadManager() {
        PRINT_DEBUG("%s %p\n", __FUNCTION__, this);
        return &gamepad_manager;
    }
    Network* __stdcall GetNetwork() {
        PRINT_DEBUG("%s %p\n", __FUNCTION__, this);
        return &network_manager;
    }
    CloudSaveManager* __stdcall GetCloudSaveManager() {
        PRINT_DEBUG("%s %p\n", __FUNCTION__, this);
        return &cloudsave_manager;
    }
    GamerPicManager* __stdcall GetGamerPicManager() {
        PRINT_DEBUG("%s %p\n", __FUNCTION__, this);
        return &gamerpic_manager;
    }
    void __stdcall w() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall x() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall y() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall z() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall aa() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall ab() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall ac() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall ad() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall ae() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall af() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall ag() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall ah() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall ai() { PRINT_DEBUG("%s\n", __FUNCTION__); }
    void __stdcall aj() { PRINT_DEBUG("%s\n", __FUNCTION__); }
};

IRgsc* (*g_getInterface)();

IRgsc* getInterface()
{
    PRINT_DEBUG("%s\n", __FUNCTION__);
    static IRgscImp* imp = new IRgscImp();
    return imp;
}

bool SCEmuLoader_CStrEndsWith(const char* fullString, const char* ending)
{
    size_t fullString_len = strlen(fullString);
    size_t ending_len = strlen(ending);

    if (fullString_len >= ending_len && strcmp(fullString + (fullString_len - ending_len), ending) == 0)
    {
        return true;
    }

    return false;
}

bool SCEmuLoader_CStrEndsWithW(const WCHAR* fullString, const WCHAR* ending)
{
    size_t fullString_len = wcslen(fullString);
    size_t ending_len = wcslen(ending);

    if (fullString_len >= ending_len && wcscmp(fullString + (fullString_len - ending_len), ending) == 0)
    {
        return true;
    }

    return false;
}

DWORD FindProcessId(const std::wstring& processName)
{
    PROCESSENTRY32 processInfo;
    processInfo.dwSize = sizeof(processInfo);

    HANDLE processesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    if (processesSnapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    Process32First(processesSnapshot, &processInfo);
    if (!processName.compare(processInfo.szExeFile))
    {
        CloseHandle(processesSnapshot);
        return processInfo.th32ProcessID;
    }

    while (Process32Next(processesSnapshot, &processInfo))
    {
        if (!processName.compare(processInfo.szExeFile))
        {
            CloseHandle(processesSnapshot);
            return processInfo.th32ProcessID;
        }
    }

    CloseHandle(processesSnapshot);
    return 0;
}

uintptr_t GetModuleBaseAddress(DWORD procId, const wchar_t* modName)
{
    uintptr_t modBaseAddr = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
    if (hSnap != INVALID_HANDLE_VALUE)
    {
        MODULEENTRY32 modEntry;
        modEntry.dwSize = sizeof(modEntry);
        if (Module32First(hSnap, &modEntry))
        {
            do
            {
                if (!_wcsicmp(modEntry.szModule, modName))
                {
                    modBaseAddr = (uintptr_t)modEntry.modBaseAddr;
                    break;
                }
            } while (Module32Next(hSnap, &modEntry));
        }
    }
    CloseHandle(hSnap);
    return modBaseAddr;
}


typedef HMODULE(WINAPI* PFN_LoadLibraryW)(LPCWSTR);
typedef HMODULE(WINAPI* PFN_LoadLibraryExW)(LPCWSTR, HANDLE, DWORD);
typedef DWORD(WINAPI* PFN_GetFileAttributesW)(LPCWSTR);
typedef DWORD(WINAPI* PFN_GetFileAttributesExW)(LPCWSTR, GET_FILEEX_INFO_LEVELS, LPVOID);

PFN_LoadLibraryW Real_LoadLibraryW = NULL;
PFN_LoadLibraryExW Real_LoadLibraryExW = NULL;
PFN_GetFileAttributesW Real_GetFileAttributesW = NULL;
PFN_GetFileAttributesExW Real_GetFileAttributesExW = NULL;

HMODULE WINAPI Hooked_LoadLibraryW(LPCWSTR lpLibFileName)
{

    if (SCEmuLoader_CStrEndsWithW(lpLibFileName, L"socialclub.dll"))
    {
        return Real_LoadLibraryW(L"socialclub.dll");
    }

    return Real_LoadLibraryW(lpLibFileName);
}
uintptr_t scAddr = 0x0;

HMODULE WINAPI Hooked_LoadLibraryExW(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
    if (SCEmuLoader_CStrEndsWithW(lpLibFileName, L"socialclub.dll"))
    {
        HMODULE WINAPI ret = Real_LoadLibraryExW(L"socialclub.dll", hFile, dwFlags);
        CMemory::Base() = (uintptr_t)GetModuleBaseAddress(FindProcessId(gameExe), L"socialclub.dll");
        scAddr = CMemory(0x86910).Get<uintptr_t>();
        if (scAddr != 0x0 && scAddr != 0x0000000000086910)
        {
            Sleep(1000);
            CMemory(0x86910).Detour(getInterface, &g_getInterface);
        }
        return ret;
    }

    return Real_LoadLibraryExW(lpLibFileName, hFile, dwFlags);
}

DWORD WINAPI Hooked_GetFileAttributesW(LPCWSTR lpFileName)
{
    if (SCEmuLoader_CStrEndsWithW(lpFileName, L"socialclub.dll"))
    {
        memcpy((void*)lpFileName, L"socialclub.dll", sizeof(L"socialclub.dll"));
    }

    return Real_GetFileAttributesW(lpFileName);
}

DWORD WINAPI Hooked_GetFileAttributesExW(LPCTSTR lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId, LPVOID lpFileInformation)
{
    if (SCEmuLoader_CStrEndsWithW(lpFileName, L"socialclub.dll"))
    {
        memcpy((void*)lpFileName, L"socialclub.dll", sizeof(L"socialclub.dll"));
    }

    return Real_GetFileAttributesExW(lpFileName, fInfoLevelId, lpFileInformation);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    string tmp;
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        CMemory::Base() = 0;
        char buf[MAX_PATH];
        GetModuleFileNameA(nullptr, buf, MAX_PATH);
        tmp = buf;
        tmp = tmp.substr(tmp.find_last_of("/\\") + 1);
        gameExe = std::wstring(tmp.begin(), tmp.end());

        MH_Initialize();
        MH_CreateHook((LPVOID*)(&LoadLibraryW), (LPVOID*)(&Hooked_LoadLibraryW), (LPVOID*)(&Real_LoadLibraryW));
        MH_CreateHook((LPVOID*)(&LoadLibraryExW), (LPVOID*)(&Hooked_LoadLibraryExW), (LPVOID*)(&Real_LoadLibraryExW));
        MH_CreateHook((LPVOID*)(&GetFileAttributesW), (LPVOID*)(&Hooked_GetFileAttributesW), (LPVOID*)(&Real_GetFileAttributesW));
        MH_CreateHook((LPVOID*)(&GetFileAttributesExW), (LPVOID*)(&Hooked_GetFileAttributesExW), (LPVOID*)(&Real_GetFileAttributesExW));
        MH_EnableHook(MH_ALL_HOOKS);

        break;
    case DLL_PROCESS_DETACH:
        MH_DisableHook(MH_ALL_HOOKS);
        break;
    }
    return true;
}

