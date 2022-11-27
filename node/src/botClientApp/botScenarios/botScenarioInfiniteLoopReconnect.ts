// -------------------------------------------------------------------------------------------------
// COPYRIGHT (C)2017 BY MOTIF CO., LTD. ALL RIGHTS RESERVED.
// -------------------------------------------------------------------------------------------------

//--------------------------------------------
// https://www.npmjs.com/package/behaviortree
//--------------------------------------------

import { BehaviorTree, Sequence, Selector, Task, SUCCESS, RUNNING, FAILURE } from 'behaviortree';

const tree = new Selector({
  nodes: [new Sequence({ nodes: ['tickWait3', 'tickWait3', 'reconnect'] })],
});

export function createScenario(bot) {
  const bTree = new BehaviorTree({
    tree: tree,
    blackboard: bot,
  });
  return bTree;
}
