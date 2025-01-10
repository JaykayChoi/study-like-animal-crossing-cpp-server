// ----------------------------------------------------------------------------
// COPYRIGHT (C)2017 BY MOTIF CO., LTD. ALL RIGHTS RESERVED.
// ----------------------------------------------------------------------------
import minimist from 'minimist';
import * as mysql from 'promise-mysql';
import { promisify } from 'util';
import fs from 'fs';
import path from 'path';
import mkdirp from 'mkdirp';
import { using } from 'bluebird';

function getConnection(connectionOption) {
  return mysql.createConnection(connectionOption).disposer((connection) => {
    connection.end();
  });
}

function extract(connectionOption, databaseName, outputDirectory) {
  console.log(`Connecting to '${connectionOption.host}:${connectionOption.port}'`);
  return using(getConnection(connectionOption), async (connection) => {
    console.log(`Querying procedures for '${databaseName}' database`);
    const rows = await connection.query(`
      SELECT ROUTINE_NAME
        FROM INFORMATION_SCHEMA.ROUTINES
       WHERE ROUTINE_SCHEMA = '${databaseName}'
         AND ROUTINE_TYPE = 'PROCEDURE';
    `);

    console.log(`Making output directory '${outputDirectory}'`);
    await promisify(mkdirp)(outputDirectory);

    for (const row of rows) {
      const procedureName = row.ROUTINE_NAME;
      console.log(`Querying create procedure '${databaseName}.${procedureName}'`);
      const createProcedureRows = await connection.query(`
        SHOW CREATE PROCEDURE ${databaseName}.${procedureName};
      `);
      const createProcedureRow = createProcedureRows[0];

      console.log(`Writing procedure file '${outputDirectory}/${procedureName}.sql'`);
      await promisify(fs.writeFile)(
        `${outputDirectory}/${procedureName}.sql`,
        createProcedureRow['Create Procedure']
      );
    }
  });
}

function main() {
  const argv = minimist(process.argv.slice(2));

  const configPath = argv.config || path.join('config', 'default.json');
  console.log(`Loading config from '${configPath}'`);

  // --config 로 설정 파일 지정
  const config = JSON.parse(fs.readFileSync(configPath, { encoding: 'utf-8' }));

  // --env 로 설정 파일 내의 환경 지정
  const environment = argv.env || process.env.NODE_ENV || 'development';
  const connectionOption = config[environment];

  const databaseName = connectionOption.database;

  const outputDirectory = argv.output || 'procedures';

  return extract(connectionOption, databaseName, outputDirectory);
}

main()
  .then(() => {
    console.log('Finished');
    process.exit(0);
  })
  .catch((err) => {
    console.error(err.message);
    process.exit(-1);
  });
