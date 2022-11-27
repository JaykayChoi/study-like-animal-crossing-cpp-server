// ----------------------------------------------------------------------------
// COPYRIGHT (C)2017 BY MOTIF CO., LTD. ALL RIGHTS RESERVED.
// ----------------------------------------------------------------------------

import botConf from './botConf';
import BotClient from './botClient';
import { Const as ConnectionConst } from './botConnection';

export let sendMap: Map<number, any> = new Map<number, any>();

//----------------------------------------------------------
// request senders
//----------------------------------------------------------
// export const sendTownEnterBuilding = (bot: BotClient) => {
//   const body = {
//     buildingType: BUILDING_TYPE[botConf.destinationBuildingType],
//     religionType: 0,
//   };

//   const packet = {
//     seqNum: 0,
//     type: proto.Town.ENTER_BUILDING,
//     body: JSON.stringify(body),
//   };

//   bot.getConn().sendJsonPacket(packet, NaN);
// };

function buildTownMovePacketBodyBuffer(packetType: number, moveState: any): Buffer {
  const buf = Buffer.allocUnsafe(ConnectionConst.TownMoveTxPacketSize);

  // Write move state.
  let pos = 0;
  buf.writeInt32LE(packetType, pos);
  pos += 4;
  buf.writeInt32LE(moveState.x, pos);
  pos += 4;
  buf.writeInt32LE(moveState.y, pos);
  pos += 4;
  buf.writeInt32LE(moveState.degrees, pos);
  pos += 4;
  buf.writeInt32LE(moveState.speed, pos);

  return buf;
}

//----------------------------------------------------------
export const sendTownMove = (bot: BotClient, moveState: any) => {
  // const packetType = proto.Town.MOVE_CS;
  // const buf = buildTownMovePacketBodyBuffer(packetType, moveState);
  // bot.getConn().sendBinaryPacket(proto.Town.MOVE_CS, buf);
};

//----------------------------------------------------------
export const sendSetPoint = (bot: BotClient, cmsId: number, amount: number) => {
  // const body = {
  //   cmsId,
  //   amount,
  // };

  // const packet = {
  //   seqNum: 0,
  //   type: proto.Dev.SET_POINT,
  //   body: JSON.stringify(body),
  // };

  // bot.getConn().sendJsonPacket(packet, NaN);
};

//----------------------------------------------------------
// regist request senders
// 하나의 ActionType 에서 여러 패킷의 단계를 밟는 경우 sendMap 에 해당 패킷의 send함수를 등록하여야 한다.
//----------------------------------------------------------
// sendMap[proto.Town.ENTER] = sendTownEnter;
// sendMap[proto.Town.LOAD_COMPLETE] = sendTownLoadComplete;
// sendMap[proto.Town.ENTER_BUILDING] = sendTownEnterBuilding;
// sendMap[proto.Town.DEPART_DEPART] = sendTownDepartDepart;
// sendMap[proto.Ocean.ENTER] = sendOceanEnter;
// sendMap[proto.Ocean.LOAD_COMPLETE] = sendOceanLoadComplete;
// sendMap[proto.Ocean.ARRIVE] = sendOceanArrive;

// sendMap[proto.Admin.TELEPORT_TOWN] = sendCheatTeleportTown;
//----------------------------------------------------------
