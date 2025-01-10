// ----------------------------------------------------------------------------
// COPYRIGHT (C)2017 BY MOTIF CO., LTD. ALL RIGHTS RESERVED.
// ----------------------------------------------------------------------------

'use strict';

import minimist from 'minimist';
import * as mysql from 'promise-mysql';
import { promisify } from 'util';
import fs from 'fs';
import path from 'path';
import { using } from 'bluebird';

const procedureNameFilter = /create .*procedure `(.*)`/i;

function getConnection(connectionOption) {
  return mysql.createConnection(connectionOption).disposer((connection) => {
    connection.end();
  });
}

async function apply(connectionOption, databaseName, files) {
  console.log(`Connecting to '${connectionOption.host}:${connectionOption.port}'`);
  return using(getConnection(connectionOption), async (connection) => {
    const rows = await connection.query(`
      SELECT ROUTINE_NAME
        FROM INFORMATION_SCHEMA.ROUTINES
       WHERE ROUTINE_SCHEMA = '${databaseName}'
         AND ROUTINE_TYPE = 'PROCEDURE';
    `);

    if (rows.length === 0) {
      console.log(`No procedure in '${databaseName}' database`);
    } else {
      console.log(`Dropping procedures for '${databaseName}' database`);
      const dropQueries = rows
        .map((row) => {
          const procedureName = row.ROUTINE_NAME;
          return `DROP PROCEDURE \`${databaseName}\`.\`${procedureName}\``;
        })
        .join(';');
      await connection.query(dropQueries);
    }

    for (const filename of files) {
      const createProcedure = await promisify(fs.readFile)(filename, { encoding: 'utf-8' });

      const matched = createProcedure.match(procedureNameFilter);
      const procedureName = matched[1];
      console.log(`Creating procedure '${procedureName}'`);
      await connection.query(createProcedure);
    }
  });
}

function main() {
  const argv = minimist(process.argv.slice(2));

  const configPath = argv.config ;
  console.log(`Loading config from '${configPath}'`);

  // --config 로 설정 파일 지정
  const config = JSON.parse(fs.readFileSync(configPath, { encoding: 'utf-8' }));

  // --env 로 설정 파일 내의 환경 지정
  const environment = argv.env;
  const connectionOption = config[environment];

  const databaseName = connectionOption.database;

  const inputDirectory = argv.input || 'procedures';

  const fileFilter = /.*\.sql/;
  const files = fs
    .readdirSync(inputDirectory)
    .filter((file) => fileFilter.test(file))
    .map((file) => path.join(inputDirectory, file));

  return apply(connectionOption, databaseName, files);
}

main()
  .then(() => {
    console.log('Finished');
    process.exit(0);
  })
  .catch((err) => {
    console.log(err.message);
    console.error(err.stack);
    process.exit(-1);
  });
