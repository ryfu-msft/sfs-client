// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "sfsclient/SFSClient.h"

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <time.h>
#include <vector>

using namespace SFS;

namespace
{
void DisplayUsage()
{
    std::cout
        << "Usage: SFSClientTool --productName <name> [options]" << std::endl
        << std::endl
        << "Required:" << std::endl
        << "  --productName <name>\t\tName of the product to be retrieved" << std::endl
        << std::endl
        << "Options:" << std::endl
        << "  -h, --help\t\t\tDisplay this help message" << std::endl
        << "  --accountId <id>\t\tAccount ID of the SFS service, used to identify the caller" << std::endl
        << "  --instanceId <id>\t\tA custom SFS instance ID" << std::endl
        << "  --namespace <ns>\t\tA custom SFS namespace" << std::endl
        << "  --customUrl <url>\t\tA custom URL for the SFS service. Library must have been built with SFS_ENABLE_OVERRIDES"
        << std::endl
        << std::endl
        << "Example:" << std::endl
        << "  SFSClientTool --productName \"Microsoft.WindowsStore_12011.1001.1.0_x64__8wekyb3d8bbwe\" --accountId test"
        << std::endl;
}

void DisplayHelp()
{
    std::cout << "SFSClient Tool" << std::endl
              << "Copyright (c) Microsoft Corporation. All rights reserved." << std::endl
              << std::endl
              << "Use to interact with the SFS service through the SFS Client library." << std::endl
              << std::endl;
    DisplayUsage();
}

const std::string c_boldRedStart = "\033[1;31m";
const std::string c_cyanStart = "\033[0;36m";
const std::string c_darkGreyStart = "\033[0;90m";
const std::string c_colorEnd = "\033[0m";

void PrintError(std::string_view message)
{
    std::cout << c_boldRedStart << message << c_colorEnd << std::endl;
}

void PrintLog(std::string_view message)
{
    std::cout << c_cyanStart << message << c_colorEnd << std::endl;
}

struct Settings
{
    bool displayHelp{true};
    std::string productName;
    std::string accountId;
    std::string instanceId;
    std::string nameSpace;
    std::string customUrl;
};

int ParseArguments(const std::vector<std::string_view>& args, Settings& settings)
{
    settings = {};
    settings.displayHelp = args.size() == 1;

    const size_t argsSize = args.size();
    auto validateArg =
        [&argsSize](const size_t index, const std::string& switchName, const std::string_view& argValue) -> bool {
        if (argsSize <= index + 1)
        {
            PrintError("Missing argument of --" + switchName + ".");
            return false;
        }
        if (!argValue.empty())
        {
            PrintError("--" + switchName + " can only be specified once.");
            return false;
        }
        return true;
    };

    for (size_t i = 1; i < args.size(); ++i)
    {
        if (args[i].compare("-h") == 0 || args[i].compare("--help") == 0)
        {
            settings.displayHelp = true;
        }
        else if (args[i].compare("--productName") == 0)
        {
            if (!validateArg(i, "productName", settings.productName))
            {
                return 1;
            }
            settings.productName = args[++i];
        }
        else if (args[i].compare("--accountId") == 0)
        {
            if (!validateArg(i, "accountId", settings.accountId))
            {
                return 1;
            }
            settings.accountId = args[++i];
        }
        else if (args[i].compare("--instanceId") == 0)
        {
            if (!validateArg(i, "instanceId", settings.instanceId))
            {
                return 1;
            }
            settings.instanceId = args[++i];
        }
        else if (args[i].compare("--namespace") == 0)
        {
            if (!validateArg(i, "namespace", settings.nameSpace))
            {
                return 1;
            }
            settings.nameSpace = args[++i];
        }
        else if (args[i].compare("--customUrl") == 0)
        {
            if (!validateArg(i, "customUrl", settings.customUrl))
            {
                return 1;
            }
            settings.customUrl = args[++i];
        }
        else
        {
            PrintError("Unknown option " + std::string(args[i]) + ".\n");
            return 1;
        }
    }
    return 0;
}

constexpr std::string_view ToString(HashType type)
{
    switch (type)
    {
    case HashType::Sha1:
        return "Sha1";
    case HashType::Sha256:
        return "Sha256";
    }
    return "";
}

void DisplayResults(const std::unique_ptr<Content>& content)
{
    if (!content)
    {
        std::cout << "No results found." << std::endl;
        return;
    }

    auto comma = [](size_t count, size_t size) -> std::string { return count == size - 1 ? "" : ","; };

    PrintLog("Content found:");
    std::cout << "{" << std::endl;
    std::cout << R"(  "Content": {)" << std::endl;
    std::cout << R"(    "Namespace": ")" << content->GetContentId().GetNameSpace() << R"(",)" << std::endl;
    std::cout << R"(    "Name": ")" << content->GetContentId().GetName() << R"(",)" << std::endl;
    std::cout << R"(    "Version": ")" << content->GetContentId().GetVersion() << R"(",)" << std::endl;

    if (content->GetFiles().size() == 0)
    {
        std::cout << R"(    "Files": [])" << std::endl;
    }
    else
    {
        std::cout << R"(    "Files": [)" << std::endl;

        size_t fileCount = 0;
        for (const auto& file : content->GetFiles())
        {
            std::cout << R"(      {)" << std::endl;
            std::cout << R"(        "FileId": ")" << file->GetFileId() << R"(",)" << std::endl;
            std::cout << R"(        "Url": ")" << file->GetUrl() << R"(",)" << std::endl;
            std::cout << R"(        "SizeInBytes": )" << file->GetSizeInBytes() << R"(,)" << std::endl;
            std::cout << R"(        "Hashes": {)" << std::endl;
            size_t hashCount = 0;
            for (const auto& hash : file->GetHashes())
            {
                std::cout << "          \"" << ToString(hash.first) << R"(": ")" << hash.second << '"'
                          << comma(hashCount++, file->GetHashes().size()) << std::endl;
            }
            std::cout << R"(        })" << std::endl;
            std::cout << R"(      })" << comma(fileCount++, content->GetFiles().size()) << std::endl;
        }
        std::cout << R"(    ])" << std::endl;
    }
    std::cout << R"(  })" << std::endl;
    std::cout << R"(})" << std::endl;
}

void LogResult(const SFS::Result& result)
{
    std::cout << "  Result code: " << ToString(result.GetCode());
    if (!result.GetMessage().empty())
    {
        std::cout << ". Message: " << result.GetMessage();
    }
    std::cout << std::endl;
}

std::string TimestampToString(std::chrono::time_point<std::chrono::system_clock> time)
{
    using namespace std::chrono;

    // get number of milliseconds for the current second
    // (remainder after division into seconds)
    auto ms = duration_cast<milliseconds>(time.time_since_epoch()) % 1000;

    auto timer = system_clock::to_time_t(time);

    std::stringstream timeStream;
    struct tm gmTime;
#ifdef _WIN32
    gmtime_s(&gmTime, &timer); // gmtime_s is the safe version of gmtime, not available on Linux
#else
    gmTime = (*std::gmtime(&timer));
#endif
    timeStream << std::put_time(&gmTime, "%F %X"); // yyyy-mm-dd HH:MM:SS
    timeStream << '.' << std::setfill('0') << std::setw(3) << ms.count();

    return timeStream.str();
}

void LoggingCallback(const SFS::LogData& logData)
{
    std::cout << c_darkGreyStart << "Log: " << TimestampToString(logData.time) << " [" << ToString(logData.severity)
              << "]"
              << " " << std::filesystem::path(logData.file).filename().string() << ":" << logData.line << " "
              << logData.message << c_colorEnd << std::endl;
}

bool SetEnv(const std::string& varName, const std::string& value)
{
    if (varName.empty() || value.empty())
    {
        return false;
    }
#ifdef _WIN32
    return _putenv_s(varName.c_str(), value.c_str()) == 0;
#else
    return setenv(varName.c_str(), value.c_str(), 1 /*overwrite*/) == 0;
#endif
}
} // namespace

int main(int argc, char* argv[])
{
    Settings settings;
    if (ParseArguments(std::vector<std::string_view>(argv, argv + argc), settings) != 0)
    {
        DisplayUsage();
        return 1;
    }

    if (settings.displayHelp || settings.productName.empty())
    {
        DisplayHelp();
        return 0;
    }

    if (!settings.customUrl.empty())
    {
        SetEnv("SFS_TEST_OVERRIDE_BASE_URL", settings.customUrl);
        PrintLog("Using custom URL: " + settings.customUrl);
        PrintLog("Note that the library must have been built with SFS_ENABLE_OVERRIDES to use a custom URL.");
    }

    // Initialize SFSClient
    PrintLog("Initializing SFSClient with accountId: " + settings.accountId +
             (settings.instanceId.empty() ? "" : ", instanceId: " + settings.instanceId) +
             (settings.nameSpace.empty() ? "" : ", namespace: " + settings.nameSpace));
    std::unique_ptr<SFSClient> sfsClient;
    auto result =
        SFSClient::Make({settings.accountId, settings.instanceId, settings.nameSpace, LoggingCallback}, sfsClient);
    if (!result)
    {
        PrintError("Failed to initialize SFSClient.");
        LogResult(result);
        return result.GetCode();
    }

    // Perform operations using SFSClient
    PrintLog("Getting latest download info for product: " + settings.productName);
    std::unique_ptr<Content> content;
    result = sfsClient->GetLatestDownloadInfo(settings.productName, content);
    if (!result)
    {
        PrintError("Failed to get latest download info.");
        LogResult(result);
        return result.GetCode();
    }

    // Display results
    DisplayResults(content);

    return 0;
}
