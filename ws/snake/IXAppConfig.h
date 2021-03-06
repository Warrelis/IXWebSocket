/*
 *  IXAppConfig.h
 *  Author: Benjamin Sergeant
 *  Copyright (c) 2019 Machine Zone, Inc. All rights reserved.
 */

#pragma once

#include "nlohmann/json.hpp"
#include <string>
#include <vector>

namespace snake
{
    struct AppConfig
    {
        // Server
        std::string hostname;
        int port;

        // Redis
        std::vector<std::string> redisHosts;
        int redisPort;
        std::string redisPassword;

        // AppKeys
        nlohmann::json apps;

        // Misc
        bool verbose;
    };

    bool isAppKeyValid(const AppConfig& appConfig, std::string appkey);

    std::string getRoleSecret(const AppConfig& appConfig, std::string appkey, std::string role);

    std::string generateNonce();

    void dumpConfig(const AppConfig& appConfig);
} // namespace snake
