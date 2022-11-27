// ----------------------------------------------------------------------------
// COPYRIGHT (C)2017 BY MOTIF CO., LTD. ALL RIGHTS RESERVED.
// ----------------------------------------------------------------------------

import BotConnection from './botConnection';
import { State as ConnectionState } from './botConnection';
import botConf from './botConf';
import * as botConst from './const';
import _ from 'lodash';
import { BehaviorTree } from 'behaviortree';
import {
  LoginPhase,
  BehaviorActionType,
  BehaviorRequestResultType,
  BotMode,
  ScenarioType,
  scenarioTypeToString,
  BotMoveState,
  PacketType,
} from './botEnums';
import * as reqSenders from './botReqSenders';
import { vec2 } from 'gl-matrix';
import { SUCCESS, RUNNING, FAILURE } from 'behaviortree';

import * as scenarioInfiniteLoopReconnect from './botScenarios/botScenarioInfiniteLoopReconnect';
import * as scenarioTownRandomMove from './botScenarios/botScenarioTownRandomMove';

// ----------------------------------------------------------------------------
// Constants.
// ----------------------------------------------------------------------------
class BehaviorRequest {
  private actionType: BehaviorActionType = 0;
  private requiredPacketTypes: number[] = [];
  private result: BehaviorRequestResultType = 0;

  constructor() {}

  getRequiredPacketTypes() {
    return this.requiredPacketTypes;
  }
  setRequiredPacketTypes(values: number[]) {
    this.requiredPacketTypes = values;
  }

  get getActionType() {
    return this.actionType;
  }
  set setActionType(val: BehaviorActionType) {
    this.actionType = val;
  }

  get getResult() {
    return this.result;
  }
  set setResult(val: BehaviorRequestResultType) {
    this.result = val;
  }

  reset() {
    this.setActionType = BehaviorActionType.None;
    this.setResult = BehaviorRequestResultType.Success;
    this.requiredPacketTypes = [];
  }
}

interface BehaviorStat {
  waitCount: number;
  waitMax: number;
}

export class BotClient {
  private pubId: string;
  private userName: string; // dev계정에서는 sessionToken
  private _userId: number = 0;
  private _conn: BotConnection;
  private botMode: BotMode;
  private loginPhase: LoginPhase;
  private moveStates: any[];
  private curMoveStateIndex: number;
  private behaviorStat: BehaviorStat;
  private curRequest: BehaviorRequest = new BehaviorRequest();
  private scenarioBehaviorTree: BehaviorTree;
  private _moveState: BotMoveState = BotMoveState.MoveStopped;
  private lastTickTimeInMs: number = 0;

  private _isScenarioInitiated: boolean = false;
  private _lastLogoutTimeUtc: number = 0;
  private _townMovePacketRecvCount: number = 0;

  constructor(idx: number) {
    // User info.
    const pubIdInt = botConf.pubIdStart + idx;
    this.pubId = pubIdInt.toString();
    this.userName = botConf.userNamePrefix + this.pubId;

    // Socket.
    this._conn = new BotConnection(this.pubId, this.userName);

    // State.
    this.botMode = BotMode.LOGIN_MODE;
    this.loginPhase = LoginPhase.UNAUTHORIZED;

    // behavior tree setting
    this.behaviorStat = {
      waitCount: 0,
      waitMax: 0,
    };

    this.initScenario();

    // Move state.
    this.resetMoveState();
  }

  public getConn(): BotConnection {
    return this._conn;
  }
  public setConn(value: BotConnection) {
    this._conn = value;
  }

  public getMoveState(): BotMoveState {
    return this._moveState;
  }

  public setMoveState(value: BotMoveState) {
    this._moveState = value;
  }

  public getLogoutTimeUtc(): number {
    return this._lastLogoutTimeUtc;
  }
  public setLastLogoutTimeUtc(value: number) {
    this._lastLogoutTimeUtc = value;
  }

  //----------------------------------------------------------
  resetBehaviorStat() {
    this.behaviorStat.waitCount = 0;
    this.behaviorStat.waitMax = 0;
    this._isScenarioInitiated = false;

    this.resetMoveState();
  }

  //----------------------------------------------------------
  private resetMoveState() {
    this.curMoveStateIndex = 0;
    this.moveStates = [];
    this.moveStates.push({
      x: -678000,
      y: -185800,
      degrees: 0,
      speed: 3,
    });
    this.moveStates.push({
      x: -650500,
      y: -183100,
      degrees: 15,
      speed: 800,
    });
    this.moveStates.push({
      x: -617700,
      y: -174100,
      degrees: 0,
      speed: 0,
    });
    this.moveStates.push({
      x: -619400,
      y: -174200,
      degrees: -177,
      speed: 800,
    });
    this.moveStates.push({
      x: -683900,
      y: -178500,
      degrees: 180,
      speed: 783,
    });
    this.moveStates.push({
      x: -671800,
      y: -177500,
      degrees: -3,
      speed: 761,
    });
    this.moveStates.push({
      x: -650600,
      y: -190000,
      degrees: 0,
      speed: 0,
    });

    this._townMovePacketRecvCount = 0;

    // ocean move data
    this._moveState = BotMoveState.MoveStopped;
  }

  //----------------------------------------------------------
  getBehaviorStat(): BehaviorStat {
    return this.behaviorStat;
  }

  //----------------------------------------------------------
  getBehaviorRequestResult(): BehaviorRequestResultType {
    return this.curRequest.getResult;
  }

  //----------------------------------------------------------
  isDisconnected(): boolean {
    return this._conn.isDisconnected();
  }

  //----------------------------------------------------------
  close(): void {
    if (this._conn) {
      this._conn.disconnect();
      this.setLastLogoutTimeUtc(new Date().getTime() / 1000);
    }
  }

  //----------------------------------------------------------
  reconnect(): void {
    console.info('reconnect..................................', { pubId: this.pubId });
    this.botMode = BotMode.LOGIN_MODE;
    this._conn.setState(ConnectionState.INITIALIZED);
    this.loginPhase = LoginPhase.UNAUTHORIZED;
    this._conn.clearPacketQueue();

    this.initScenario();

    this.resetBehaviorStat();
  }

  initScenario() {
    console.info(`applying [${botConf.scenarioTypeName}] ...`);

    switch (ScenarioType[botConf.scenarioTypeName]) {
      case ScenarioType.None:
        break;
      case ScenarioType.InfiniteLoopReconnect:
        this.scenarioBehaviorTree = scenarioInfiniteLoopReconnect.createScenario(this);
        break;
      case ScenarioType.TownRandomMove:
        this.scenarioBehaviorTree = scenarioTownRandomMove.createScenario(this);
        break;

      default:
        console.warn(`not found [${botConf.scenarioTypeName}] ...applying default scenario`);
        break;
    }
  }

  //----------------------------------------------------------
  tick(): void {
    if (this._conn.isDisconnected()) {
      return;
    }

    try {
      switch (this.botMode) {
        case BotMode.LOGIN_MODE:
          this._tickLoginMode();
          break;
        case BotMode.SCENARIO_MODE:
          this._tickScenarioMode();
          break;
        default:
          // invalid mode
          break;
      }
    } catch (err) {
      console.error('tick exception', {
        userId: this._userId,
        err,
      });
      if (!this._conn.isDisconnected()) {
        this.close();
      }
      return;
    }
  }

  //----------------------------------------------------------
  // Login Mode Processes
  //----------------------------------------------------------
  private _tickLoginMode() {
    //handle login mode packets
    this.handleLoginModePacket();

    switch (this.loginPhase) {
      case LoginPhase.UNAUTHORIZED:
        this.tickOnUnauthorized();
        break;
      case LoginPhase.AUTHRIZING:
        break;
      case LoginPhase.AUTHORIZED:
        this.tickOnAuthorized();
        break;
      case LoginPhase.LOGGING_IN:
        this.tickOnLoggingIn();
        break;

      case LoginPhase.AFTER_LOGGED_IN:
        this.tickOnAfterLoggedIn();
        break;
      case LoginPhase.MAP_LOADING:
        break;
      case LoginPhase.MAP_LOADING_COMPLETE:
        // end of login mode. changing to scenario mode
        this.botMode = BotMode.SCENARIO_MODE;
        break;

      default:
        break;
    }
  }

  //----------------------------------------------------------
  // botClient의 LoginMode용 패킷핸들러는 이곳에 추가.
  private handleLoginModePacket() {
    const packet = this._conn.popPacket();
    if (!packet || !packet.type) {
      return;
    }
    if (!this.updateSyncLoginMode(packet)) {
      this.close();
      return;
    }

    switch (packet.type) {
      case PacketType.Hello:
        this.onRecvHello(packet);
        break;
      case PacketType.Login:
        this.onRecvLogin(packet);
        break;
      case PacketType.EnterWorld:
        this.onRecvEnterWorld(packet);
        break;

      default:
        console.info('handlePacket unhandled packet', {
          userId: this._userId,
          packetType: packet.type,
        });
        break;
    }
  }

  //----------------------------------------------------------
  // 항상 최신의 sync data를 업데이트 받아놓는다.
  private updateSyncLoginMode(packet): boolean {
    try {
      // todo
      return true;
    } catch (err) {
      console.error('updateSyncLoginMode error', {
        userId: this._userId,
        err,
      });
      return false;
    }
  }

  //----------------------------------------------------------
  private tickOnUnauthorized(): void {
    // todo
    this.loginPhase = LoginPhase.AUTHORIZED;
  }
  //----------------------------------------------------------
  private tickOnAuthorized(): void {
    this.loginPhase = LoginPhase.LOGGING_IN;
    this._conn.reconnect();
  }

  //----------------------------------------------------------
  private tickOnLoggingIn(): void {
    console.info('logged-in', { pubId: this.pubId });

    this.loginPhase = LoginPhase.AFTER_LOGGED_IN;
  }

  //----------------------------------------------------------
  // 로그인완료 시점. 이곳에서 필요한 분기를 시킨다.
  //----------------------------------------------------------
  private tickOnAfterLoggedIn(): void {
    // todo
  }

  //----------------------------------------------------------
  // login handlers
  //---------------------------------------------------
  private onRecvHello(packet): void {
    // First take the double quotes from both ends of the 'body'.
    const peerPublicKey = packet.body.replace(/"/g, '');
    this._conn.getCryptoCtx.computeSecret(Buffer.from(peerPublicKey, 'hex'));

    // Now send login.
    const body = {
      accountId: this.userName,
    };

    const loginPacket = {
      type: PacketType.Login,
      seqNum: 0,
      body: JSON.stringify(body),
    };

    this._conn.sendJsonPacket(loginPacket);
  }

  //----------------------------------------------------------
  private onRecvLogin(packet): void {
    const resBody = JSON.parse(packet.body);
    if (resBody.kickReason) {
      console.info('onRecvEnterWorld kicked from server', {
        userId: this._userId,
      });
      this.close();
      return;
    }

    // const user = resBody.sync.add.user;
    // this.pubId = user.pubId ? user.pubId : this.pubId;
    // this._userId = user.userId ? user.userId : 0;
    // this._conn.setUserId(this._userId);

    console.info('onRecvLogin', {});

    const body = {
      accountId: this.userName,
    };

    const enterWorldPacket = {
      type: PacketType.EnterWorld,
      seqNum: 0,
      body: JSON.stringify(body),
    };

    this._conn.sendJsonPacket(enterWorldPacket);
  }

  //----------------------------------------------------------
  private onRecvEnterWorld(packet): void {
    const resBody = JSON.parse(packet.body);

    // const user = resBody.sync.add.user;
    // this.pubId = user.pubId ? user.pubId : this.pubId;
    // this._userId = user.userId ? user.userId : 0;
    // this._conn.setUserId(this._userId);

    this._userId = resBody.userId;

    console.info('onRecvEnterWorld', {
      userId: this._userId,
    });

    // TODO
  }
  //----------------------------------------------------------
  // Scenario Mode 함수들
  //----------------------------------------------------------
  private _tickScenarioMode() {
    //handle scenario mode packets
    this.handleScenarioModePacket();

    this.tickOnPlaySecenario();
  }

  //----------------------------------------------------------
  // botClient의 ScenarioMode용 패킷핸들러는 이곳에 추가.
  private handleScenarioModePacket() {
    for (let k = 0; k < 5; k++) {
      const packet = this._conn.popPacket();
      if (!packet) {
        return;
      }
      if (!packet.type) {
        continue;
      }

      let ret = true;
      try {
        if (packet.body) {
          const resBody = JSON.parse(packet.body);
          if (!this.updateSyncScenarioMode(resBody)) {
            this.close();
            return;
          }

          // update latest request's state
          if (resBody.errCode) {
            ret = 0 == resBody.errCode ? true : false;
          }
        }
      } catch (error) {
        console.error('handleScenarioModePacket failed', {
          userId: this._userId,
          error: error.message,
        });
        this.close();
        return;
      }

      this.updateBehaviorRequest(packet.type, ret);

      // add individual packet handlers here
      switch (
        packet.type
        // todo
      ) {
      }
    }
  }

  //----------------------------------------------------------
  // 항상 최신의 sync data를 업데이트 받아놓는다.
  private updateSyncScenarioMode(resBody): boolean {
    // todo
    if (resBody.errCode) {
      console.error('received error', {
        errCode: resBody.errCode,
        errMessage: resBody.errMessage ? resBody.errMessage : null,
      });
      return false;
    }
    return true;
  }

  //----------------------------------------------------------
  private updateBehaviorRequest(packetType: number, isSuccess: boolean) {
    let requiredPacketTypes = this.curRequest.getRequiredPacketTypes();
    if (0 == requiredPacketTypes.length) {
      return;
    }
    let found: boolean = false;
    let k = 0;
    for (; k < requiredPacketTypes.length; k++) {
      if (packetType == requiredPacketTypes[k]) {
        found = true;
        break;
      }
    }
    if (!found) {
      return;
    }

    if (!isSuccess) {
      //anyone fails all fail
      this.curRequest.setResult = BehaviorRequestResultType.Failure;

      console.error('failed request', { userId: this._userId, packetType });
    } else {
      //if last packet is success, it's request is complete
      if (k == requiredPacketTypes.length - 1) {
        this.curRequest.setResult = BehaviorRequestResultType.Success;
      } else {
        //send next request packet
        const nextPacketType = requiredPacketTypes[k + 1];
        if (!nextPacketType) {
          return;
        }
        const func = reqSenders.sendMap[nextPacketType];
        if (func) {
          console.info('next request', {
            userId: this._userId,
            nextPacketType,
            func,
          });

          func(this);
        }
      }
    }
  }

  //----------------------------------------------------------
  private tickOnPlaySecenario(): void {
    // [todo] act based on gamestate
    this.scenarioBehaviorTree.step();

    // console.info('tickOnPlaySecenario', {
    //   userId: this._userId,
    //   lastResult: this.scenarioBehaviorTree.lastResult,
    //   lastRundData: this.scenarioBehaviorTree.lastRundData,
    // });

    if (
      this.scenarioBehaviorTree.lastResult === SUCCESS ||
      this.scenarioBehaviorTree.lastResult === FAILURE
    ) {
      this.curRequest.reset();
    }

    //console.log(this.scenarioBehaviorTree.lastRundData);
  }

  //----------------------------------------------------------
  //----------------------------------------------------------
  // BehaviorTree 함수들
  //----------------------------------------------------------
  //----------------------------------------------------------
  private _btWaitSet(count: number) {
    this.behaviorStat.waitCount = 0;
    this.behaviorStat.waitMax = count;
  }

  //----------------------------------------------------------
  btTickWait(count: number) {
    if (this.behaviorStat.waitCount < 0) {
      this._btWaitSet(count);
      return 0;
    } else {
      this.behaviorStat.waitCount++;
      if (this.behaviorStat.waitCount >= this.behaviorStat.waitMax) {
        this.behaviorStat.waitCount = -1;
        return 1;
      }
      return 0;
    }
  }

  //----------------------------------------------------------
  btTickWaitRandom(maxCount: number) {
    const val = Math.floor(Math.random() * maxCount) + 1;
    if (this.behaviorStat.waitCount < 0) {
      this._btWaitSet(val);
      return 0;
    } else {
      this.behaviorStat.waitCount++;
      if (this.behaviorStat.waitCount >= this.behaviorStat.waitMax) {
        this.behaviorStat.waitCount = -1;
        return 1;
      }
      return 0;
    }
  }

  //----------------------------------------------------------
  btReconnect() {
    console.info('btReconnect', { userId: this._userId });
    this.reconnect();
  }
  //----------------------------------------------------------
  btIsMoveStateStopped() {
    console.info('btIsMoveStateStopped', { userId: this._userId });
    return BotMoveState.MoveStopped === this.getMoveState();
  }

  //----------------------------------------------------------
  btIsScenarioInitiated(): boolean {
    return this._isScenarioInitiated;
  }

  //----------------------------------------------------------
  btSetScenarioInitiated() {
    this._isScenarioInitiated = true;
    console.info('btSetScenarioInitiated fin', { userId: this._userId });
    return BehaviorRequestResultType.Success;
  }

  //----------------------------------------------------------
  private _btStartCheatSetPointDucat(): BehaviorRequestResultType {
    console.info('_btStartCheatSetPointDucat', { userId: this._userId });

    // todo
    this.curRequest.setRequiredPacketTypes([9001]);
    this.curRequest.setResult = BehaviorRequestResultType.Pending;
    this.curRequest.setActionType = BehaviorActionType.CheatSetPointDucat;

    // todo
    reqSenders.sendSetPoint(this, 10001, botConf.point.ducatAmount);
    return BehaviorRequestResultType.Pending;
  }

  //----------------------------------------------------------
  btCheatSetPointDucat(): BehaviorRequestResultType {
    if (BehaviorActionType.CheatSetPointDucat != this.curRequest.getActionType) {
      return this._btStartCheatSetPointDucat();
    } else {
      const ret = this.getBehaviorRequestResult();
      if (BehaviorRequestResultType.Success !== ret) {
        return ret;
      }

      console.info('btCheatSetPointDucat fin', { userId: this._userId });
      return ret;
    }
  }

  //----------------------------------------------------------
  btLogout(): BehaviorRequestResultType {
    console.info('btLogout fin', {
      userId: this._userId,
      reconnectWaitTimeUtc: botConf.reconnectWaitTimeSec,
    });

    //접속을 종료시키므로 더이상 behaviorTree 스텝도 진행되지 않는다.
    this.close();

    return BehaviorRequestResultType.Failure;
  }
}

export default BotClient;
