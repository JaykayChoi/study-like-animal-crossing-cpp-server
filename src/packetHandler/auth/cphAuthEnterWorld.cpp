#include "cphAuthEnterWorld.h"
#include "../../clientHandler.h"
#include "../../serverInstance.h"
#include "../../sql/authDb.h"
#include "../../sql/userDb.h"
#include "json/json.h"

void CphAuthEnterWorld::Exec(
    PacketHandler* packetHandler, int seqNum, const Json::Value& bodyRoot)
{
    ClientHandler* client = packetHandler->GetClientHandler();
    if (!bodyRoot.isObject())
    {
        client->PacketError(
            static_cast<uint32>(EPacketType::AUTH_ENTER_WORLD), "invalid-req-body");
        return;
    }

    // TODO enter world 에서는 accountId 대신에 enterWorldToken 을 사용.
    std::string accountId = bodyRoot.get("accountId", "").asString();
    if (accountId.empty())
    {
        client->PacketError(
            static_cast<uint32>(EPacketType::AUTH_ENTER_WORLD), "invalid-req-body");
        return;
    }

    // TODO auth server 분리하여 userId 얻어와야 됨. 임시로 여기서 직접 auth db 사용.

    // TODO worldId
    AuthDb* authDb = ServerInstance::Get().GetAuthDb();
    AuthDb::EnterWorldResult authEnterWorldResult = authDb->EnterWorld(accountId, "1");

    if (!authEnterWorldResult.errMsg.empty())
    {
        // TODO 에러 처리.
    }

    int curTimeUtc = (int)std::time(0);
    UserDb* userDb = ServerInstance::Get().GetUserDb();
    UserDb::EnterWorldResult userEnterWorldResult = userDb->EnterWorld(
        authEnterWorldResult.userId, authEnterWorldResult.pubId, curTimeUtc);

    if (!userEnterWorldResult.errMsg.empty())
    {
        // TODO 에러 처리.
    }

    client->OnEnterWorld(seqNum, userEnterWorldResult);
}