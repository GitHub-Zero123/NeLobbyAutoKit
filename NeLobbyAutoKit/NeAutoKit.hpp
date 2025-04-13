#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <string>
#include <memory>
#include <vector>
#include <filesystem>
#include "libs/Utils/json.hpp"
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "libs/Utils/httplib.h"

// By Zero123
namespace NeAutoKit
{
    using json = nlohmann::json;

    inline bool _checkOk(const std::string& str, const std::string& targetKey="status", const std::string& targteValue="ok")
    {
        try
        {
            auto jo = json::parse(str);
            if (jo.contains(targetKey))
            {
                return jo[targetKey] == targteValue;
            }
        }
        catch (const std::exception& e) {}
        return false;
    }

    //===========================================
    // PutFileInfo：处理文件返回的数据解析
    //===========================================
    class PutFileInfo
    {
    public:
        std::string htmlData;
        std::string xNtesSignature;
        std::string fileName;
        json joData;
        std::string argsU8;

        PutFileInfo(const std::string& htmlData, const std::string& xNtesSignature, const std::string& fileName = "")
            : htmlData(htmlData), xNtesSignature(xNtesSignature), fileName(fileName)
        {
            // 正则表达式提取<textarea>中的内容并解析为json
            static const std::regex re("<textarea>([\\s\\S]*?)</textarea>");
            std::smatch match;
            if (std::regex_search(htmlData, match, re))
            {
                try
                {
                    argsU8 = match[1].str();
                    joData = json::parse(argsU8);
                }
                catch (const json::parse_error& e)
                {
                    std::cerr << "json解析错误: " << e.what() << std::endl;
                }
            }
        }
    };

    //===========================================
    // ModINFO：用于处理/修改 MOD 数据
    //===========================================
    class ModINFO
    {
    public:
        std::string modId;
        json args;  // 存放MOD数据

        // 构造函数：对传入的字典数据进行处理
        ModINFO(const json& dic, const std::string& modId = "")
            : modId(modId), args(replaceNoneInJson(dic))
        {
            // 网易发力了 读取的数据和写入的MOD信息结构不一样 甚至类型也不一样 null/""/[]/{} 混用

            // 如果存在 author_info，则删除
            if (args.contains("author_info"))
            {
                args.erase("author_info");
            }
            // 针对特定 key 为空的情况做处理
            static const std::vector<std::string> mayBeNullKeys = { "subject_id", "commercial_time", "openbeta_time" };
            for (const auto& key : mayBeNullKeys)
            {
                if (!args.contains(key) || args[key].is_null() || (args[key].is_string() && args[key].get<std::string>().empty()))
                {
                    args[key] = nullptr;  // 赋值为null
                }
            }

            // 如果为 "" 则删除：pri_effect_type, sub_effect_type
            static const std::vector<std::string> removeKeys = { "pri_effect_type", "sub_effect_type" };
            for (const auto& key : removeKeys)
            {
                if (args.contains(key) && (args[key].is_null() || (args[key].is_string() && args[key].get<std::string>().empty())))
                {
                    args.erase(key);
                }
            }

            // DLC 数据处理，如果不存在 dlc_info 则不处理
            if (args.contains("dlc_info"))
            {
                auto& dlcInfo = args["dlc_info"];
                if (!dlcInfo.contains("master") || dlcInfo["master"].is_null() || !dlcInfo["master"].is_object() || dlcInfo["master"].empty())
                {
                    dlcInfo["master"] = json::object();
                }

                // 如果 slave_list 不存在 或 值为 null 或 不是数组 或是空数组，则设为空数组
                if (!dlcInfo.contains("slave_list") || dlcInfo["slave_list"].is_null() || !dlcInfo["slave_list"].is_array() || dlcInfo["slave_list"].empty())
                {
                    dlcInfo["slave_list"] = json::array();
                }
            }
        }

        // 递归替换 json 中的 null 值为空字符串
        static json replaceNoneInJson(json obj)
        {
            if (obj.is_object())
            {
                for (auto it = obj.begin(); it != obj.end(); ++it)
                {
                    it.value() = replaceNoneInJson(it.value());
                }
            }
            else if (obj.is_array())
            {
                for (auto& el : obj)
                {
                    el = replaceNoneInJson(el);
                }
            }
            else if (obj.is_null())
            {
                return "";
            }
            return obj;
        }

        // 获取完整数据引用
        json& getDatasRef()
        {
            return args;
        }

        // 获取完整数据
        json getDatas() const
        {
            return args;
        }

        // 获取MOD名称
        std::string getModName() const
        {
            if (args.contains("item_name"))
            {
                return args["item_name"].get<std::string>();
            }
            return "";
        }

        // 设置MOD名称
        void setModName(const std::string& name)
        {
            args["item_name"] = name;
        }

        // 获取MODID
        std::string getItemId() const
        {
            if (args.contains("item_id"))
            {
                return args["item_id"].get<std::string>();
            }
            return "";
        }

        // 获取更新日志
        std::string getUpdateSummary() const
        {
            if (args.contains("update_summary"))
            {
                return args["update_summary"].get<std::string>();
            }
            return "";
        }

        // 设置更新日志
        void setUpdateSummary(const std::string& summary)
        {
            args["update_summary"] = summary;
        }

        // 设置 MOD ZIP文件（默认下标0）
        void setResFile(const PutFileInfo& fileRes, size_t index = 0)
        {
            if (!args.contains("res") || !args["res"].is_array() || args["res"].size() <= index)
            {
                std::cerr << "资源列表不存在或下标越界" << std::endl;
                return;
            }
            auto& res = args["res"][index];
            // 构造 resUrl 对象
            json resUrl;
            resUrl["body"] = fileRes.argsU8;
            resUrl["file_type"] = "zip_package";
            resUrl["sign"] = fileRes.xNtesSignature;
            res["res_url"] = std::move(resUrl);
            if (!fileRes.fileName.empty())
            {
                res["res_name"] = fileRes.fileName;
            }
            for (const auto& key : { "res_info", "res_id", "cdn_info", "cdn_url"})
            {
                if (res.contains(key))
                {
                    res.erase(key);
                }
            }
        }

        // 获取MOD状态 (审核中/未审核/自测中/...)
        std::string getState() const
        {
            if (args.contains("status"))
            {
                return args["status"].get<std::string>();
            }
            return "";
        }

        // 获取MOD当前状态是否可以测试(已上线/自测完毕)
        bool getStateCanTest() const
        {
            std::string state = getState();
            return (state == "self_test" || state == "online");
        }
    };

    //===========================================
    // NeteaseUser: 模拟网易开发者用户的平台操作
    //===========================================
    class NeteaseUser
    {
    public:
        std::string cookie;
        httplib::Headers baseHeader;

        NeteaseUser(const std::string& cookie) : cookie(cookie)
        {
            baseHeader.insert({ "cookie", cookie });
        }

        // [需要异常处理] 获取MOD数据(只读)
        json getModData(const std::string& modId) const
        {
            std::string host = "mc-launcher.webapp.163.com";
            std::string path = "/items/categories/pe/" + modId;
            httplib::SSLClient cli(host.c_str());
            auto res = cli.Get(path.c_str(), baseHeader);
            if (res && res->status == 200)
            {
                return json::parse(res->body);
            }
            else
            {
                throw std::runtime_error("GET 请求失败: " + (res ? std::to_string(res->status) : "null response"));
            }
            return json::object();
        }

        // [需要异常处理] 获取可修改的MOD数据 生成 ModINFO 对象
        ModINFO getModifyModData(const std::string& modId) const
        {
            json data = getModData(modId);
            if (data.contains("data"))
            {
                return ModINFO(data["data"], modId);
            }
            throw std::runtime_error("无法获取 MOD 数据");
        }

        // 更新MOD数据
        bool updateModData(const ModINFO& modInfo) const
        {
            if (modInfo.modId.empty())
            {
                throw std::runtime_error("ModID不能为空");
            }
            std::string modId = modInfo.modId;
            std::string host = "mc-launcher.webapp.163.com";
            std::string path = "/items/categories/pe/" + modId + "/update";
            httplib::SSLClient cli(host.c_str());
            auto res = cli.Post(path.c_str(), baseHeader, modInfo.getDatas().dump(), "application/json");
            return (res && res->status == 200) && _checkOk(res->body);
        }

        // 从文件路径加载PutFileInfo对象
        PutFileInfo putFile(const std::filesystem::path& filePath, const std::string& fileName = "default.zip") const
        {
            std::ifstream ifs(filePath, std::ios::binary);
            if (!ifs)
            {
                throw std::runtime_error("无法打开文件: " + filePath.string());
            }
            std::stringstream buffer;
            buffer << ifs.rdbuf();
            std::string fileBytes = buffer.str();
            return putFileBytes(fileBytes, fileName);
        }

        // 直接使用字节构造PutFileInfo对象
        PutFileInfo putFileBytes(const std::string& fileBytes, const std::string& fileName = "default.zip") const
        {
            // 获取文件token
            std::string host1 = "mc-launcher.webapp.163.com";
            std::string path1 = "/filepicker/file_token?file_type=zip_package&secure=true";
            httplib::SSLClient cli1(host1.c_str());
            auto res1 = cli1.Get(path1.c_str(), baseHeader);
            if (!(res1 && res1->status == 200))
            {
                throw std::runtime_error("获取 token 失败");
            }
            json tokenJson = json::parse(res1->body);
            std::string token = tokenJson["data"]["token"];

            // 上传到wy服务器
            std::string host2 = "pfp.ps.netease.com";
            std::string path2 = "/x19s/file/new/";
            httplib::SSLClient cli2(host2.c_str());

            httplib::MultipartFormDataItems items =
            {
                { "Authorization", token, "", ""},
                { "fpfile", fileBytes, fileName, "application/x-zip-compressed" }
            };

            auto res2 = cli2.Post(path2.c_str(), items);

            if (!(res2 && res2->status == 200))
            {
                throw std::runtime_error("文件上传失败");
            }
            //if (_checkJson(res2->body))
            //{
            //    throw std::runtime_error(res2->body);
            //}
            // 构造 PutFileInfo 对象 x-ntes-signature 用于后续的校验工作
            std::string signature;
            auto it = res2->headers.find("x-ntes-signature");
            if (it != res2->headers.end())
            {
                signature = it->second;
            }
            else
            {
                throw std::runtime_error("响应缺少x-ntes-signature头");
            }
            return PutFileInfo(res2->body, signature, fileName);
        }

        // 发起MOD自测请求
        bool selfTestMode(const std::string& modId, bool passCheck = true) const
        {
            std::string host = "mc-launcher.webapp.163.com";
            std::string path = "/items/categories/pe/" + modId + "/self-test-apply";
            httplib::SSLClient cli(host.c_str());
            json body;
            body["self_test_pass_check"] = passCheck;
            auto res = cli.Put(path.c_str(), baseHeader, body.dump(), "application/json");
            return (res && res->status == 200 && _checkOk(res->body));
        }
    };
}