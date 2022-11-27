// ----------------------------------------------------------------------------
// COPYRIGHT (C)2017 BY MOTIF CO., LTD. ALL RIGHTS RESERVED.
// ----------------------------------------------------------------------------

import minimist from 'minimist';
import * as childProcess from 'child_process';
import fs from 'fs';
import * as JSON5 from 'json5';
import _ from 'lodash';

const tmpConfigPath = 'config/.database.migrate.tmp.json';

function exec(cmd: string) {
  console.log(cmd);
  const output = childProcess.execSync(cmd);
  console.log(output.toString());
}

function migrateEnv(config: any, target: string, direction: string) {
  if (!target || target === 'table') {
    _.forOwn(config, (_cfg, env) => {
      if (env.includes('user')) {
        const cmd = `db-migrate ${direction}:user --config ${tmpConfigPath} --env ${env}`;
        exec(cmd);
      } else {
        const cmd = `db-migrate ${direction}:${env} --config ${tmpConfigPath} --env ${env}`;
        exec(cmd);
      }
    });
  }

  if (!target || target === 'procedure') {
    _.forOwn(config, (_cfg, env) => {
      if (env.includes('user')) {
        const cmd =
          `node -r source-map-support/register dist/spapply ` +
          `--config ${tmpConfigPath} --env ${env} --input procedures/user`;
        exec(cmd);
      } else {
        const cmd =
          `node -r source-map-support/register dist/spapply ` +
          `--config ${tmpConfigPath} --env ${env} --input procedures/${env}`;
        exec(cmd);
      }
    });
  }
}

function loadConfigFile(filePath: string) {
  const file = fs.readFileSync(filePath, 'utf8');
  try {
    const split = filePath.split('.');
    const extension = split[split.length - 1];
    const data = extension === 'json5' ? JSON5.parse(file) : JSON.parse(file);
    return data;
  } catch (err) {
    throw new Error(`parse is failed '${filePath}': ${err.message}`);
  }
}

function writeConfigFile(filePath: string, data: string) {
  try {
    fs.writeFileSync(filePath, data);
  } catch (err) {
    throw new Error(`write is failed '${filePath}': ${err.message}`);
  }
}

function main() {
  // parse options.
  const argv = minimist(process.argv.slice(2));
  const env = argv.env; // 정의되어 있지 않은 경우 모든 환경 migrate. 입력 받을 수 있는 환경의 종류는 environments 변수 참고.
  const target = argv.target; // table or procedure. 정의되어 있지 않은 경우 둘 다.
  const direction = argv.direction || 'up'; // up or down

  // service_layout 에서 user db sharding, user, password 을 얻어온다.
  const layout = loadConfigFile('../service_layout/local.json5');
  const configData: any = {};
  if (!env || env === 'auth') {
    configData.auth = layout.sharedConfig.mysqlAuthDb;
  }
  if (!env || env === 'world') {
    configData.world = layout.world.worlds[0].mysqlWorldDb;
  }
  if (!env || env === 'user') {
    const mysqlUserDb = layout.world.worlds[0].mysqlUserDb;
    const shards = mysqlUserDb.shards;
    delete mysqlUserDb.shards;
    for (const shard of shards) {
      configData[shard.sqlCfg.database] = _.cloneDeep(mysqlUserDb.sqlDefaultCfg);
      _.merge(configData[shard.sqlCfg.database], shard.sqlCfg);
    }
  }

  writeConfigFile(tmpConfigPath, JSON.stringify(configData));

  try {
    migrateEnv(configData, target, direction);
  } catch (err) {
    fs.unlinkSync(tmpConfigPath);
    throw new Error(`migrate is failed. err: ${err.message}`);
  }

  fs.unlinkSync(tmpConfigPath);
}

main();
