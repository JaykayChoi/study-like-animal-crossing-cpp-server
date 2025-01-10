// -------------------------------------------------------------------------------------------------
// COPYRIGHT (C)2017 BY MOTIF CO., LTD. ALL RIGHTS RESERVED.
// -------------------------------------------------------------------------------------------------

import { BehaviorTree, Sequence, Selector, decorators } from 'behaviortree';
const { InvertDecorator, AlwaysSucceedDecorator } = decorators;

const tree = new Selector({
  nodes: [
    new Sequence({
      nodes: [
        new InvertDecorator({ node: 'isScenarioInitiated' }),
        'setNextDestinationTown',
        'setScenarioInitiated',
      ],
    }),
    new Sequence({
      nodes: ['isInOceanLoading', 'MakeOceanLoadComplete'],
    }),
    new Sequence({
      nodes: [new InvertDecorator({ node: 'isPointBluegemSufficient' }), 'cheatSetPointBluegem'],
    }),
    new Sequence({
      nodes: [new InvertDecorator({ node: 'isPointDucatSufficient' }), 'cheatSetPointDucat'],
    }),

    new Sequence({
      nodes: [
        'isInOcean',
        new Selector({
          nodes: [
            new Sequence({
              nodes: ['isWrecked', 'resurrect', 'cheatTeleportTown'],
            }),
            new Sequence({
              nodes: ['cheatTeleportTown'],
            }),
          ],
        }), // end of Selector1
      ],
    }),
    new Sequence({
      nodes: [
        'isInTown',
        new Selector({
          nodes: [
            new Sequence({
              nodes: [new InvertDecorator({ node: 'isInDestTown' }), 'cheatTeleportTown'],
            }),
            new Sequence({
              nodes: ['isInDestTown', 'townRandomMove'],
            }),
          ],
        }), // end of Selector1
      ],
    }),
  ],
}); // end of Selector0

export function createScenario(bot) {
  const bTree = new BehaviorTree({
    tree: tree,
    blackboard: bot,
  });
  return bTree;
}
