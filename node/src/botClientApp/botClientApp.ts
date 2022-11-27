// ----------------------------------------------------------------------------
// COPYRIGHT (C)2017 BY MOTIF CO., LTD. ALL RIGHTS RESERVED.
// ----------------------------------------------------------------------------
// Set process title.
process.name = 'botclientApp';

const cfgName = process.argv[2];

// Start server.
import * as botLogic from './botLogic';
botLogic.start(cfgName);

['SIGTERM', 'SIGINT', 'SIGHUP', 'SIGQUIT'].forEach((signal: NodeJS.Signals) => {
  process.on(signal, botLogic.stop);
});
