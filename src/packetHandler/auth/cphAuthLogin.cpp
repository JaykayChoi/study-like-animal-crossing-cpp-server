﻿#include "cphAuthLogin.h"
#include "../../clientHandler.h"
#include "../../serverManager.h"
#include "../../sql/authDb.h"
#include "json/json.h"

void CphAuthLogin::Exec(
    PacketHandler* packetHandler, int seqNum, const Json::Value& bodyRoot)
{
    ClientHandler* client = packetHandler->GetClientHandler();
    if (!bodyRoot.isObject())
    {
        client->PacketError(
            static_cast<uint32>(EPacketType::AUTH_LOGIN), "invalid-req-body");
        return;
    }

    std::string accountId = bodyRoot.get("accountId", "").asString();
    if (accountId.empty())
    {
        client->PacketError(
            static_cast<uint32>(EPacketType::AUTH_LOGIN), "invalid-req-body");
        return;
    }

    int curTimeUtc = (int)std::time(0);

    AuthDb* authDb = ServerManager::Get().GetAuthDb();

    // TEMP
    if (!authDb)
    {
        return;
    }

    AuthDb::LoginResult loginResult = authDb->Login(accountId, curTimeUtc);

    if (!loginResult.errMsg.empty())
    {
        // TODO 에러 처리.
    }

    // TODO enterWorldToken 발급.

    // TODO kick old connection

    client->OnLogin(loginResult.bIsNewUser);
}