// ----------------------------------------------------------------------------
// COPYRIGHT (C)2017 BY MOTIF CO., LTD. ALL RIGHTS RESERVED.
// ----------------------------------------------------------------------------

import JSON5 from 'json5';
import _ from 'lodash';
import path from 'path';
import fs from 'fs';

import BotClient from './botClient';
import botConf from './botConf';
import * as botConst from './const';
import { registerBehaviorNodes } from './botScenarios/botBehaviorCommonNode';

// -------------------------------------------------------------------------------------------------
// // undefined 참조등으로 예외를 catch 하지 못하면 호출
// -------------------------------------------------------------------------------------------------
process.on('uncaughtException', (err) => {
  console.error('uncaught Exception', {
    msg: err.message,
    stack: err.stack,
  });
  // 위에 error로그가 파일기록이 비동기라서 약 1초간의 딜레이를 준 후 종료 시킨다.
  setTimeout(() => {
    process.exit(1);
  }, 1000);
});

// -------------------------------------------------------------------------------------------------
// Promise의 then 에서 예외를 catch 하지 못하면 호출
// -------------------------------------------------------------------------------------------------
process.on('unhandledRejection', (err: Error) => {
  console.error('unhandled Rejection', {
    msg: err.message,
    stack: err.stack,
  });
  // 위에 error로그가 파일기록이 비동기라서 약 1초간의 딜레이를 준 후 종료 시킨다.
  setTimeout(() => {
    process.exit(1);
  }, 1000);
});

// ----------------------------------------------------------------------------
// Module variables.
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Variables.
// ----------------------------------------------------------------------------

let stopping = false;

const allClients: BotClient[] = [];

// ----------------------------------------------------------------------------
// Private functions.
// ----------------------------------------------------------------------------

function stopImpl() {
  // Wait for all clients to terminate.
  setTimeout(() => {
    console.info('exit');
    process.exit(0);
  }, botConf.numBots * 50);
}

function tick() {
  if (stopping) {
    stopImpl();
    return;
  }

  allClients.forEach((bc) => {
    setTimeout(() => {
      if (stopping) {
        return;
      }
      if (bc.isDisconnected()) {
        if (bc.getLogoutTimeUtc() + botConf.reconnectWaitTimeSec > new Date().getTime() / 1000) {
          return;
        }
        bc.reconnect();
      } else {
        bc.tick();
      }
    }, (Math.random() * botConf.tickIntervalMsec) / 2);
  });

  setTimeout(() => {
    tick();
  }, botConf.tickIntervalMsec);
}

function load(cfgname: string): Promise<any> {
  const configFolder = 'config';
  const configFilePath = path.join(configFolder, cfgname);

  const cfgData = fs.readFileSync(configFilePath, 'utf8');

  console.info('config loaded successfully.', { cfgData });

  return JSON5.parse(cfgData);
}

// ----------------------------------------------------------------------------
// Public functions.
// ----------------------------------------------------------------------------

export const start = async (cfgname: string = 'botClientApp_sample.json5') => {
  // load config
  console.info('loading config...', {
    cfgname,
  });

  const loadedData = await load(cfgname);
  if (!loadedData) {
    throw new Error('Failed to load config!');
  }
  _.merge(botConf, loadedData.common);

  // merge scenario configurations
  const scenarioConf = loadedData[botConf.scenarioTypeName];
  if (scenarioConf) {
    _.merge(botConf, scenarioConf);
  }

  console.log('Config: ', botConf);

  //register behaviorTree nodes
  registerBehaviorNodes();

  // Create clients.
  console.info('spawning clients ...', { numBots: botConf.numBots });
  for (let i = 0; i < botConf.numBots; ++i) {
    const bc = new BotClient(i);
    allClients.push(bc);
  }

  // Start ticking.
  tick();
};

export const stop = () => {
  // Close all clients.
  console.info('closing clients ...');
  stopping = true;
  allClients.forEach((bc) => {
    bc.close();
  });
};
