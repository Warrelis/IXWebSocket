/*
 *  IXSentryClient.cpp
 *  Author: Benjamin Sergeant
 *  Copyright (c) 2019 Machine Zone. All rights reserved.
 */

#include "IXSentryClient.h"

#include <chrono>
#include <iostream>

#include <ixwebsocket/IXWebSocketHttpHeaders.h>


namespace ix
{
    SentryClient::SentryClient(const std::string& dsn) :
        _dsn(dsn),
        _validDsn(false),
        _luaFrameRegex("\t([^/]+):([0-9]+): in function '([^/]+)'")
    {
        const std::regex dsnRegex("(http[s]?)://([^:]+):([^@]+)@([^/]+)/([0-9]+)");
        std::smatch group;

        if (std::regex_match(dsn, group, dsnRegex) && group.size() == 6)
        {
            _validDsn = true;

            const auto scheme = group.str(1);
            const auto host = group.str(4);
            const auto project_id = group.str(5);
            _url = scheme + "://" + host + "/api/" + project_id + "/store/";

            _publicKey = group.str(2);
            _secretKey = group.str(3);
        }
    }

    int64_t SentryClient::getTimestamp()
    {
        const auto tp = std::chrono::system_clock::now();
        const auto dur = tp.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::seconds>(dur).count();
    }

    std::string SentryClient::getIso8601()
    {
        std::time_t now;
        std::time(&now);
        char buf[sizeof("2011-10-08T07:07:09Z")];
        std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&now));
        return buf;
    }

    std::string SentryClient::computeAuthHeader()
    {
        std::string securityHeader("Sentry sentry_version=5");
        securityHeader += ",sentry_client=ws/1.0.0";
        securityHeader += ",sentry_timestamp=" + std::to_string(SentryClient::getTimestamp());
        securityHeader += ",sentry_key=" + _publicKey;
        securityHeader += ",sentry_secret=" + _secretKey;

        return securityHeader;
    }

    Json::Value SentryClient::parseLuaStackTrace(const std::string& stack)
    {
        Json::Value frames;

        // Split by lines
        std::string line;
        std::stringstream tokenStream(stack);

        std::stringstream ss;
        std::smatch group;

        while (std::getline(tokenStream, line))
        {
            //	MapScene.lua:2169: in function 'singleCB'
            if (std::regex_match(line, group, _luaFrameRegex))
            {
                const auto fileName = group.str(1);
                const auto linenoStr = group.str(2);
                const auto function = group.str(3);

                ss << linenoStr;
                uint64_t lineno;
                ss >> lineno;

                Json::Value frame;
                frame["lineno"] = Json::UInt64(lineno);
                frame["filename"] = fileName;
                frame["function"] = function;

                frames.append(frame);
            }
        }

        return frames;
    }

    std::string parseExceptionName(const std::string& stack)
    {
        // Split by lines
        std::string line;
        std::stringstream tokenStream(stack);

        // Extract the first line
        std::getline(tokenStream, line);

        return line;
    }

    std::string SentryClient::computePayload(const Json::Value& msg)
    {
        Json::Value payload;
        payload["platform"] = "python";
        payload["sdk"]["name"] = "ws";
        payload["sdk"]["version"] = "1.0.0";
        payload["timestamp"] = SentryClient::getIso8601();

        bool isNoisyTypes = msg["id"].asString() == "game_noisytypes_id";

        std::string stackTraceFieldName = isNoisyTypes ? "traceback" : "stack";
        std::string stack(msg["data"][stackTraceFieldName].asString());

        Json::Value exception;
        exception["stacktrace"]["frames"] = parseLuaStackTrace(stack);
        exception["value"] = isNoisyTypes ? parseExceptionName(stack) : msg["data"]["message"];

        payload["exception"].append(exception);

        Json::Value extra;
        extra["cobra_event"] = msg;

        exception["extra"] = extra;

        return _jsonWriter.write(payload);
    }

    bool SentryClient::send(const Json::Value& msg,
                            bool verbose)
    {
        HttpRequestArgs args;
        args.extraHeaders["X-Sentry-Auth"] = SentryClient::computeAuthHeader();
        args.connectTimeout = 60;
        args.transferTimeout = 5 * 60;
        args.followRedirects = true;
        args.verbose = verbose;
        args.logger = [](const std::string& msg)
        {
            std::cout << msg;
        };

        std::string body = computePayload(msg);
        HttpResponse response = _httpClient.post(_url, body, args);

        if (verbose)
        {
            for (auto it : response.headers)
            {
                std::cerr << it.first << ": " << it.second << std::endl;
            }

            std::cerr << "Upload size: " << response.uploadSize << std::endl;
            std::cerr << "Download size: " << response.downloadSize << std::endl;

            std::cerr << "Status: " << response.statusCode << std::endl;
            if (response.errorCode != HttpErrorCode::Ok)
            {
                std::cerr << "error message: " << response.errorMsg << std::endl;
            }

            if (response.headers["Content-Type"] != "application/octet-stream")
            {
                std::cerr << "payload: " << response.payload << std::endl;
            }
        }

        return response.statusCode == 200;
    }
} // namespace ix
