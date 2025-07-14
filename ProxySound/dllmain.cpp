#include <windows.h>
#include <strsafe.h>
#include <filesystem>
#include <thread>

HMODULE real_dsound = nullptr;

void LoadOriginalDSound() {
    if (!real_dsound) {
        const char* sys_path = "C:\\Windows\\System32\\dsound.dll";
        real_dsound = LoadLibraryA(sys_path);
        if (!real_dsound) {
			printf("Failed to load original dsound.dll from %s\n", sys_path);
            return;
        }
        
    }
}

std::filesystem::path GetInjectedDllDirectory(HMODULE hModule) {
    char path[MAX_PATH];
    GetModuleFileNameA(hModule, path, MAX_PATH);
    return std::filesystem::path(path).parent_path();
}

FARPROC GetRealProc(const char* name) {
    LoadOriginalDSound();
    return GetProcAddress(real_dsound, name);
}

extern "C" {
    __declspec(dllexport) HRESULT WINAPI DirectSoundCreate(
        const GUID* lpcGuidDevice,
        void** ppDS,
        void* pUnkOuter
    ) {
        using Fn = HRESULT(WINAPI*)(const GUID*, void**, void*);
        return ((Fn)GetRealProc("DirectSoundCreate"))(lpcGuidDevice, ppDS, pUnkOuter);
    }

    typedef BOOL(CALLBACK* DSENUMCALLBACKA)(GUID*, LPCSTR, LPCSTR, void*);
    __declspec(dllexport) HRESULT WINAPI DirectSoundEnumerateA(
        DSENUMCALLBACKA cb,
        void* ctx
    ) {
        using Fn = HRESULT(WINAPI*)(DSENUMCALLBACKA, void*);
        return ((Fn)GetRealProc("DirectSoundEnumerateA"))(cb, ctx);
    }

    typedef BOOL(CALLBACK* DSENUMCALLBACKW)(GUID*, LPCWSTR, LPCWSTR, void*);
    __declspec(dllexport) HRESULT WINAPI DirectSoundEnumerateW(
        DSENUMCALLBACKW cb,
        void* ctx
    ) {
        using Fn = HRESULT(WINAPI*)(DSENUMCALLBACKW, void*);
        return ((Fn)GetRealProc("DirectSoundEnumerateW"))(cb, ctx);
    }

    __declspec(dllexport) HRESULT WINAPI DirectSoundCaptureCreate(
        const GUID* lpcGUID,
        void** lplpDSC,
        void* pUnkOuter
    ) {
        using Fn = HRESULT(WINAPI*)(const GUID*, void**, void*);
        return ((Fn)GetRealProc("DirectSoundCaptureCreate"))(lpcGUID, lplpDSC, pUnkOuter);
    }

    __declspec(dllexport) HRESULT WINAPI DirectSoundCaptureEnumerateA(
        DSENUMCALLBACKA cb,
        void* ctx
    ) {
        using Fn = HRESULT(WINAPI*)(DSENUMCALLBACKA, void*);
        return ((Fn)GetRealProc("DirectSoundCaptureEnumerateA"))(cb, ctx);
    }

    __declspec(dllexport) HRESULT WINAPI DirectSoundCaptureEnumerateW(
        DSENUMCALLBACKW cb,
        void* ctx
    ) {
        using Fn = HRESULT(WINAPI*)(DSENUMCALLBACKW, void*);
        return ((Fn)GetRealProc("DirectSoundCaptureEnumerateW"))(cb, ctx);
    }

    __declspec(dllexport) HRESULT WINAPI GetDeviceID(
        const GUID* pCaptureGuid,
        LPWSTR pwszDeviceID,
        DWORD* pdwSize
    ) {
        using Fn = HRESULT(WINAPI*)(const GUID*, LPWSTR, DWORD*);
        return ((Fn)GetRealProc("GetDeviceID"))(pCaptureGuid, pwszDeviceID, pdwSize);
    }

    struct DSCBUFFERDESC;
    struct DSBUFFERDESC;

    __declspec(dllexport) HRESULT WINAPI DirectSoundFullDuplexCreate(
        const GUID* lpCaptureGuid,
        const GUID* lpRenderGuid,
        const DSCBUFFERDESC* pDSCBufferDesc,
        const DSBUFFERDESC* pDSBufferDesc,
        HWND hWnd,
        DWORD dwLevel,
        void** ppDSFD,
        void* pUnkOuter
    ) {
        using Fn = HRESULT(WINAPI*)(
            const GUID*, const GUID*,
            const DSCBUFFERDESC*, const DSBUFFERDESC*,
            HWND, DWORD, void**, void*);
        return ((Fn)GetRealProc("DirectSoundFullDuplexCreate"))(
            lpCaptureGuid, lpRenderGuid, pDSCBufferDesc, pDSBufferDesc,
            hWnd, dwLevel, ppDSFD, pUnkOuter);
    }

    __declspec(dllexport) HRESULT WINAPI DirectSoundCreate8(
        const GUID* lpcGuidDevice,
        void** ppDS8,
        void* pUnkOuter
    ) {
        using Fn = HRESULT(WINAPI*)(const GUID*, void**, void*);
        return ((Fn)GetRealProc("DirectSoundCreate8"))(lpcGuidDevice, ppDS8, pUnkOuter);
    }

    __declspec(dllexport) HRESULT WINAPI DirectSoundCaptureCreate8(
        const GUID* lpcGUID,
        void** lplpDSC8,
        void* pUnkOuter
    ) {
        using Fn = HRESULT(WINAPI*)(const GUID*, void**, void*);
        return ((Fn)GetRealProc("DirectSoundCaptureCreate8"))(lpcGUID, lplpDSC8, pUnkOuter);
    }
}

HRESULT WINAPI pDllCanUnloadNow() {
    using Fn = HRESULT(WINAPI*)();
    return ((Fn)GetRealProc("DllCanUnloadNow"))();
}

HRESULT WINAPI pDllGetClassObject(const GUID& rclsid, const GUID& riid, void** ppv) {
    using Fn = HRESULT(WINAPI*)(const GUID&, const GUID&, void**);
    return ((Fn)GetRealProc("DllGetClassObject"))(rclsid, riid, ppv);
}

BOOL WINAPI DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        AllocConsole();
        freopen_s(new FILE*, "CONOUT$", "w+", stdout);
        LoadOriginalDSound();

        std::thread([hModule]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(4000));

            std::filesystem::path dllDir = GetInjectedDllDirectory(hModule) / "dlls";

            if (std::filesystem::exists(dllDir) && std::filesystem::is_directory(dllDir)) {
                for (const auto& entry : std::filesystem::directory_iterator(dllDir)) {
                    if (entry.path().extension() == ".dll") {
                        LoadLibraryA(entry.path().string().c_str());
                    }
                }
            }
            }).detach();
    }
    else if (reason == DLL_PROCESS_DETACH && real_dsound) {
        FreeLibrary(real_dsound);
    }
    return TRUE;
}