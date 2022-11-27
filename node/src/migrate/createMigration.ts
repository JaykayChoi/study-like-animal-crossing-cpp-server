// ----------------------------------------------------------------------------
// COPYRIGHT (C)2017 BY MOTIF CO., LTD. ALL RIGHTS RESERVED.
// ----------------------------------------------------------------------------

import * as childProcess from 'child_process';

const migrationName = process.argv[2];
let env = process.argv[3];

if (!migrationName) {
  console.log('Invalid migration name!');
  process.exit(1);
}

if (!env) {
  env = 'user';
}

const cmd =
  `./node_modules/.bin/db-migrate create:${env} ${migrationName}` +
  ` --config config/database.migrate.json --env ${env} --sql-file`;

console.log(cmd);

const output = childProcess.execSync(cmd, {
  env: process.env,
  maxBuffer: 10000 * 2014,
});

console.log(output.toString());
