// -------------------------------------------------------------------------------------------------
// COPYRIGHT (C)2017 BY MOTIF CO., LTD. ALL RIGHTS RESERVED.
// -------------------------------------------------------------------------------------------------

//--------------------------------------------
// https://www.npmjs.com/package/behaviortree
//--------------------------------------------

import {
  BehaviorTree,
  Sequence,
  Selector,
  InvertDecorator,
  Task,
  SUCCESS,
  RUNNING,
  FAILURE,
} from 'behaviortree';
import BotClient from '../botClient';
import { BehaviorRequestResultType } from '../botEnums';

function convertResultToGenuineType(ret: BehaviorRequestResultType): SUCCESS | RUNNING | FAILURE {
  switch (ret) {
    case BehaviorRequestResultType.Pending:
      return RUNNING;
    case BehaviorRequestResultType.Success:
      return SUCCESS;
    case BehaviorRequestResultType.Failure:
      return FAILURE;
    default:
      return FAILURE;
  }
}

export function registerBehaviorNodes() {
  //--------------------------------------------
  BehaviorTree.register(
    'tickWait3',
    new Task({
      run: function (bot: BotClient) {
        return convertResultToGenuineType(bot.btTickWait(3));
      },
    })
  );

  //--------------------------------------------
  BehaviorTree.register(
    'tickWait10',
    new Task({
      run: function (bot: BotClient) {
        return convertResultToGenuineType(bot.btTickWait(10));
      },
    })
  );

  //--------------------------------------------
  BehaviorTree.register(
    'tickWaitRandom10',
    new Task({
      run: function (bot: BotClient) {
        return convertResultToGenuineType(bot.btTickWaitRandom(10));
      },
    })
  );

  //--------------------------------------------
  BehaviorTree.register(
    'isScenarioInitiated',
    new Task({
      run: function (bot: BotClient) {
        if (bot.btIsScenarioInitiated()) {
          return SUCCESS;
        }
        return FAILURE;
      },
    })
  );

  //--------------------------------------------
  BehaviorTree.register(
    'isMoveStateStopped',
    new Task({
      run: function (bot: BotClient) {
        if (bot.btIsMoveStateStopped()) {
          return SUCCESS;
        }
        return FAILURE;
      },
    })
  );

  //--------------------------------------------
  BehaviorTree.register(
    'reconnect',
    new Task({
      run: function (bot: BotClient) {
        bot.btReconnect();
        return SUCCESS;
      },
    })
  );

  //--------------------------------------------
  BehaviorTree.register(
    'cheatSetPointDucat',
    new Task({
      run: function (bot: BotClient) {
        return convertResultToGenuineType(bot.btCheatSetPointDucat());
      },
    })
  );

  //--------------------------------------------
  BehaviorTree.register(
    'setScenarioInitiated',
    new Task({
      run: function (bot: BotClient) {
        return convertResultToGenuineType(bot.btSetScenarioInitiated());
      },
    })
  );

  //--------------------------------------------
  BehaviorTree.register(
    'logout',
    new Task({
      run: function (bot: BotClient) {
        return convertResultToGenuineType(bot.btLogout());
      },
    })
  );
}
