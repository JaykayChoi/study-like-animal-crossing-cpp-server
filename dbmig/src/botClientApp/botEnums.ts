export enum BotMode {
  LOGIN_MODE = 0,
  SCENARIO_MODE = 1,
}

export enum LoginPhase {
  UNAUTHORIZED = 0,
  AUTHRIZING = 1,
  AUTHORIZED = 2,
  LOGGING_IN = 3,
  AFTER_LOGGED_IN = 5,
  MAP_LOADING = 6,
  MAP_LOADING_COMPLETE = 7, // OBSOLETE
  DEPARTING = 8, // OBSOLETE
}

export enum PacketType {
  Hello = 101,
  Login = 102,
  EnterWorld = 103,
}

// 비동기적인 동작상태를 정의
export enum BehaviorActionType {
  None = 0,

  // common
  WaitTimeout = 11,
  SelectNation = 12,
  RepairShipsInTown = 13,
  DraftSailorInTown = 14,

  // town
  ChangeToTown = 101,
  EnterBuilding = 102,
  GoverGetSyncData = 103,
  GoverInvest = 104,
  TownRandomMove = 105,

  // ocean
  ChangeToOcean = 201,
  MakeOceanLoadComplete = 202,
  OceanMoveToTargetPos = 203,
  AutoSailingToDestination = 204,
  BeginAutoSailing = 205,
  EndAutoSailing = 206,
  Resurrect = 207,

  // ocean battle
  HandleEncount = 301,
  BattleResume = 302,
  EnterBattle = 303,
  ApplySimpleBattleEnd = 304,
  HandleBattleReward = 305,

  // cheat
  CheatTeleportTown = 401,
  CheatTeleportToLocation = 402,
  CheatSetPointDucat = 403,
  CheatSetPointBluegem = 404,
}

export enum BehaviorRequestResultType {
  Pending = 0,
  Success = 1,
  Failure = 2,
}

export enum ScenarioType {
  None = 0,
  InfiniteLoopReconnect = 1,
  TownRandomMove = 2,
}

export enum BotMoveState {
  MoveStopped = 0,
  Moving = 1,
}

export const scenarioTypeToString = (type: number) => {
  if (ScenarioType[type] !== undefined) {
    return `${ScenarioType[type]}`;
  } else {
    return `Unknown.${type}`;
  }
};
