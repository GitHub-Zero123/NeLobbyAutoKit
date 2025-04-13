#include <iostream>
#include <thread>
#include <miniz.h>
#include "libs/Utils/EnvEncode.h"
#include "NeAutoKit.hpp"
// By Zero123

#ifdef _WIN32
static void showToast(const std::wstring& title, const std::wstring& message)
{
    // 虽然很鞋垫 但很方便
    std::wstring command = L"powershell -Command \"& {"
        L" [Windows.UI.Notifications.ToastNotificationManager, Windows.UI.Notifications, ContentType = WindowsRuntime] > $null;"
        L" $template = [Windows.UI.Notifications.ToastTemplateType]::ToastText02;"
        L" $xml = [Windows.UI.Notifications.ToastNotificationManager]::GetTemplateContent($template);"
        L" $textNodes = $xml.GetElementsByTagName('text');"
        L" $textNodes.Item(0).AppendChild($xml.CreateTextNode('" + title + L"')) > $null;"
        L" $textNodes.Item(1).AppendChild($xml.CreateTextNode('" + message + L"')) > $null;"
        L" $toast = [Windows.UI.Notifications.ToastNotification]::new($xml);"
        L" $notifier = [Windows.UI.Notifications.ToastNotificationManager]::CreateToastNotifier('QNeAutoKit');"
        L" $notifier.Show($toast)"
        L" }\"";
    _wsystem(command.c_str());
}
#else
static void showToast(const std::wstring& title, const std::wstring& message) {}
#endif

// ZIP压缩器 基于miniz封装
class MemoryZipCompressor
{
public:
    bool zipSuccess = false;

    std::vector<uint8_t> compressToMemory(const std::filesystem::path& inputDir,
        const std::vector<std::string>& ignoreGlobs)
    {
        namespace fs = std::filesystem;
        zipSuccess = true;

        std::vector<uint8_t> result;

        if (!fs::exists(inputDir) || !fs::is_directory(inputDir))
        {
            std::cerr << "无效的目录\n";
            zipSuccess = false;
            return result;
        }

        std::vector<std::regex> ignorePatterns;
        for (const auto& glob : ignoreGlobs)
        {
            ignorePatterns.emplace_back(globToRegex(glob));
        }

        mz_zip_archive zip{};
        memset(&zip, 0, sizeof(zip));
        // 缓冲内存分配
        const size_t ESTIMATED_SIZE = static_cast<size_t>(8 * 1024) * 1024;
        if (!mz_zip_writer_init_heap(&zip, 0, ESTIMATED_SIZE))
        {
            std::cerr << "ZIP内存写入异常\n";
            zipSuccess = false;
            return result;
        }

        const auto inputAbs = fs::absolute(inputDir);
        size_t addedCount = 0;

        for (auto& entry : fs::recursive_directory_iterator(inputDir))
        {
            if (!entry.is_regular_file())
            {
                continue;
            }

            auto relPath = fs::relative(entry.path(), inputAbs).generic_string();
            if (shouldIgnore(relPath, ignorePatterns))
            {
                continue;
            }

            if (!mz_zip_writer_add_file(&zip, relPath.c_str(), entry.path().string().c_str(),
                nullptr, 0, MZ_BEST_SPEED))
            {
                std::cerr << "文件添加异常: " << relPath << "\n";
                zipSuccess = false;
                mz_zip_writer_end(&zip);
                return {};
            }
            addedCount++;
        }

        void* buf = nullptr;
        size_t bufSize = 0;

        if (!mz_zip_writer_finalize_heap_archive(&zip, &buf, &bufSize))
        {
            std::cerr << "内存写入失败\n";
            zipSuccess = false;
            mz_zip_writer_end(&zip);
            return {};
        }

        result.assign((uint8_t*)buf, (uint8_t*)buf + bufSize);
        mz_free(buf);  // 释放内存

        mz_zip_writer_end(&zip);
        return result;
    }

private:
    std::regex globToRegex(const std::string& glob)
    {
        std::string regexStr;
        for (char c : glob)
        {
            switch (c)
            {
            case '*': regexStr += ".*"; break;
            case '?': regexStr += "."; break;
            case '.': regexStr += "\\."; break;
            case '/': regexStr += "/"; break;
            default:
                if (std::isalnum(c)) regexStr += c;
                else regexStr += "\\" + std::string(1, c);
                break;
            }
        }
        return std::regex("^" + regexStr + "$");
    }

    bool shouldIgnore(const std::string& path, const std::vector<std::regex>& patterns)
    {
        for (const auto& regex : patterns)
        {
            if (std::regex_match(path, regex)) return true;
        }
        return false;
    }
};

static int getNowIntTime()
{
    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());
    return static_cast<int>(duration.count());
}

int main(int argc, const char** argv)
{
    Encoding::initEnvcode();
    auto args = Encoding::getUTF8Vector(argc, argv);
    std::filesystem::path workPath = std::filesystem::path(args[0]).parent_path();
    std::filesystem::path targetConfig = workPath / "TARGET_CONFIG.json";
    try
    {
        if (!std::filesystem::exists(targetConfig))
        {
            throw std::runtime_error("找不到目标配置文件: "+ targetConfig.string());
        }

        std::ifstream ifs(targetConfig);
        nlohmann::json config = nlohmann::json::parse(ifs, nullptr, true, true);
        std::string cookie = config.at("cookie");
        std::string modId = config.at("modId");
        std::string targetPath = config.at("targetPath");
        std::vector<std::string> ignore;
        if (config.contains("ignore"))
        {
            const auto& _jsonIgnore = config["ignore"];
            if (_jsonIgnore.is_array())
            {
                // 遍历数组并处理
                for (const auto& item : _jsonIgnore)
                {
                    ignore.push_back(item);
                }
            }
        }
        else
        {
            std::cout << "未找到ignore声明 建议配置, 避免敏感信息泄漏\n";
        }
        std::cout << "ZIP打包中: " << targetPath << "\n";
        MemoryZipCompressor zip;
        auto zipBytes = zip.compressToMemory(targetPath, ignore);
        if (!zip.zipSuccess)
        {
            showToast(L"压缩错误", L"文件打包期间出现错误, 已终止工作");
            return 0;
        }
        NeAutoKit::NeteaseUser user{ cookie };
        std::cout << "正在获取MOD信息..\n";
        auto modData = user.getModifyModData(modId);
        std::string modName = modData.getModName();
        std::cout << "MOD名称: " << modName << "\n";
        std::cout << "正在提交更新文件..\n";
        modData.setResFile(user.putFileBytes(
            std::string(reinterpret_cast<char*>(zipBytes.data()), zipBytes.size()),
            std::to_string(getNowIntTime()) + ".zip"
        ));
        std::cout << "正在更新MOD信息...\n";
        if (!user.updateModData(modData))
        {
            throw std::runtime_error("MOD资料更新失败: " + modData.args.dump());
        }
        std::cout << "正在提交MOD到自测环境\n";
        if (!user.selfTestMode(modId))
        {
            throw std::runtime_error("MOD自测提交失败");
        }
        std::cout << "自测提交成功 进入消息循环检查状态\n";
        while (1)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1500));
            auto nowModData = user.getModifyModData(modId);
            auto state = nowModData.getState();
            if (state == "init")
            {
                throw std::runtime_error("MOD自测未通过 请前往开发者平台检查原因");
            }
            else if(nowModData.getStateCanTest())
            {
                showToast(L"项目ID: "+Encoding::convertToWide(modId.data()), L"已发布至自测环境 可上线测试");
                break;
            }
            else
            {
                std::cout << state << "\n";
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "[Error] " << e.what() << std::endl;
        showToast(L"运行时异常", L"⚠️自动化处理期间遇到错误, 已暂停进程");
        system("pause");
    }

    return 0;
}
