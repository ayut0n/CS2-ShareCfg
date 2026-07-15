#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <windows.h>
#include <tlhelp32.h>
#include <shlobj.h>
#include <cstdlib>

namespace fs = std::filesystem;

// Функция для вывода ASCII-арта
void PrintASCII() {
    std::cout << R"(   _____ __                    __________________)" << "\n";
    std::cout << R"(  / ___// /_  ____ _________  / ____/ ____/ ____/)" << "\n";
    std::cout << R"(  \__ \/ __ \/ __ `/ ___/ _ \/ /   / /_  / / __  )" << "\n";
    std::cout << R"( ___/ / / / / /_/ / /  /  __/ /___/ __/ / /_/ /  )" << "\n";
    std::cout << R"(/____/_/ /_/\__,_/_/   \___/\____/_/    \____/   )" << "\n\n";
}

// Функция для проверки запущенного процесса
bool IsProcessRunning(const std::wstring& processName) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            if (processName == pe32.szExeFile) {
                CloseHandle(hSnapshot);
                return true;
            }
        } while (Process32NextW(hSnapshot, &pe32));
    }
    CloseHandle(hSnapshot);
    return false;
}

int main() {
    // Принудительно устанавливаем кодировку UTF-8 для консоли
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // Выводим логотип
    PrintASCII();

    std::cout << "Ожидание запуска Counter-Strike 2 (cs2.exe)...\n";
    
    // Блокирующее ожидание запуска игры
    while (!IsProcessRunning(L"cs2.exe")) {
        Sleep(2000);
    }
    std::cout << "\n[!] Процесс cs2.exe обнаружен! Начинаем сбор данных...\n";

    // Получаем активный SteamID из реестра
    DWORD activeUser = 0;
    DWORD dataSize = sizeof(activeUser);
    LSTATUS status = RegGetValueA(HKEY_CURRENT_USER, "Software\\Valve\\Steam\\ActiveProcess", 
                                  "ActiveUser", RRF_RT_DWORD, nullptr, &activeUser, &dataSize);

    if (status != ERROR_SUCCESS || activeUser == 0) {
        std::cerr << "[-] Ошибка: Не удалось получить активный Steam аккаунт. Убедитесь, что Steam запущен.\n";
        system("pause");
        return 1;
    }

    // Получаем путь установки Steam
    char steamPath[MAX_PATH];
    DWORD pathSize = sizeof(steamPath);
    status = RegGetValueA(HKEY_CURRENT_USER, "Software\\Valve\\Steam", 
                          "SteamPath", RRF_RT_REG_SZ, nullptr, steamPath, &pathSize);

    if (status != ERROR_SUCCESS) {
        std::cerr << "[-] Ошибка: Не удалось найти путь к Steam в реестре.\n";
        system("pause");
        return 1;
    }

    // Формируем путь к оригинальным файлам настроек
    fs::path cfgDirectory = fs::path(steamPath) / "userdata" / std::to_string(activeUser) / "730" / "local" / "cfg";
    std::cout << "Путь к конфигурациям: " << cfgDirectory.string() << "\n\n";

    std::vector<std::string> targetFiles = {
        "cs2_machine_convars.vcfg",
        "cs2_user_convars_0_slot0.vcfg",
        "cs2_user_convars_0_slot0.vcfg_lastclouded",
        "cs2_user_keys_0_slot0.vcfg",
        "cs2_user_keys_0_slot0.vcfg_lastclouded",
        "cs2_user_keys_0_slot1.vcfg",
        "cs2_user_keys_0_slot2.vcfg",
        "cs2_user_keys_0_slot3.vcfg",
        "cs2_video.txt",
        "cs2_video.txt.bak",
        "trustedlaunch.cfg"
    };

    // Определяем путь к папке "Документы" и создаем там временную папку для экспорта
    char docsPath[MAX_PATH];
    fs::path exportFolder = "CS2_SharedConfig_Temp"; 
    
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, 0, docsPath))) {
        exportFolder = fs::path(docsPath) / "CS2_SharedConfig_Temp";
    }

    if (!fs::exists(exportFolder)) {
        fs::create_directory(exportFolder);
    }

    int copiedCount = 0;
    
    for (const auto& fileName : targetFiles) {
        fs::path src = cfgDirectory / fileName;
        fs::path dst = exportFolder / fileName;

        if (fs::exists(src)) {
            try {
                fs::copy_file(src, dst, fs::copy_options::overwrite_existing);
                std::cout << "[+] Скопирован в буфер: " << fileName << "\n";
                copiedCount++;
            } catch (const fs::filesystem_error& e) {
                std::cerr << "[-] Ошибка при копировании " << fileName << ": " << e.what() << "\n";
            }
        }
    }

    // Создаем текстовый файл с инструкцией
    std::ofstream readmeFile(exportFolder / "ИНСТРУКЦИЯ.txt");
    if (readmeFile) {
        readmeFile << "=== Инструкция по установке настроек ===\n\n";
        readmeFile << "1. Откройте папку Steam на вашем компьютере (обычно C:\\Program Files (x86)\\Steam).\n";
        readmeFile << "2. Перейдите в папку userdata.\n";
        readmeFile << "3. Найдите папку с вашим SteamID.\n";
        readmeFile << "   (Если папок с цифрами несколько, зайдите в игру, измените любую настройку, выйдите и посмотрите, какая папка обновилась по времени).\n";
        readmeFile << "4. Перейдите по пути: [Ваш_SteamID]\\730\\local\\cfg\\\n";
        readmeFile << "5. Сделайте бэкап своих файлов на всякий случай.\n";
        readmeFile << "6. Скопируйте все файлы из этого архива в папку cfg с заменой.\n\n";
        readmeFile << "Внимание: Т.к. передается файл cs2_video.txt, разрешение экрана в игре будет таким же, как у передавшего. Если нужно, поменяйте его в настройках после запуска.\n";
        readmeFile.close();
    }

    // Путь для создания ZIP-архива
    fs::path zipFilePath = exportFolder.parent_path() / "CS2_SharedConfig.zip";
    std::cout << "\n[*] Упаковка файлов в " << zipFilePath.filename().string() << "...\n";

    // Формируем команду PowerShell для создания ZIP-архива
    std::string sourcePath = exportFolder.string() + "\\*";
    std::string psCommand = "powershell -NoProfile -Command \"Compress-Archive -Path '" + sourcePath + "' -DestinationPath '" + zipFilePath.string() + "' -Force\"";

    // Выполняем архивацию
    int zipResult = system(psCommand.c_str());

    if (zipResult == 0) {
        std::cout << "[+] Архив успешно создан!\n";
        // Удаляем временную папку, оставляем только ZIP
        fs::remove_all(exportFolder);
        std::cout << "Путь к готовому архиву: " << zipFilePath.string() << "\n\n";
    } else {
        std::cerr << "[-] Возникла ошибка при создании архива. Распакованные файлы лежат в:\n";
        std::cerr << exportFolder.string() << "\n\n";
    }
    
    system("pause");
    return 0;
}