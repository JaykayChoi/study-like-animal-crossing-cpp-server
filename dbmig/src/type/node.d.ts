// -------------------------------------------------------------------------------------------------
// COPYRIGHT (C)2017 BY MOTIF CO., LTD. ALL RIGHTS RESERVED.
// -------------------------------------------------------------------------------------------------

declare module NodeJS {
  interface Process {
    name: string;
  }
}

declare type JsonLike = { [key: string]: any };
declare type JsonString = string;
declare type JsonEmpty = {};

//
// delegates
//
declare type Action<T> = (arg: T) => void;
declare type Func<T, TResult> = (arg: T) => TResult;
