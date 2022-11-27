class BotConf {
  [key: string]: any;

  authd: string;
  scenarioTypeName: string;
  numBots: number;
  pubIdStart: number;
  userNamePrefix: string;
  tickIntervalMsec: number;

  constructor() {}
}

let botConf: BotConf = new BotConf();

export default botConf;
