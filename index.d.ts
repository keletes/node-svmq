/// <reference types="node" />
import { EventEmitter } from 'events';

declare class MessageQueue extends EventEmitter {
  /** Maximum message data size in bytes (hardcoded, 4052). */
  static readonly MSGMAX: number;
  /** `IPC_CREAT` flag (512). */
  static readonly IPC_CREATE: number;
  /** Converts the permission number given to the constructor into `msgget` flags. */
  static formatFlags(perms: number): number;
  /** Same as `new MessageQueue(key, perms)`. */
  static open(key: number, perms?: number): MessageQueue;
  /** Raw bindings to msgget/msgsnd/msgrcv/msgctl. Not recommended. */
  static readonly msg: MessageQueue.NativeBindings;

  /**
   * Opens (or creates) the queue `key`. Throws when `key` is not a number, `perms` is not a number
   * or the queue cannot be opened. Use `ftok` first when you have a path.
   */
  constructor(key: number, perms?: number, options?: object);

  readonly key: number;
  readonly perms: number;
  readonly flags: number;
  readonly options: object;
  /** System V queue id returned by msgget. */
  readonly id: number;
  readonly closed: boolean;

  /**
   * Pushes a message. Strings are converted to a Buffer. Never throws: validation and system errors
   * (their text is only the strerror message, e.g. "Resource temporarily unavailable" for EAGAIN)
   * are passed to `callback`.
   */
  push(data: Buffer | string, callback?: MessageQueue.PushCallback): void;
  push(data: Buffer | string, options: MessageQueue.PushOptions, callback?: MessageQueue.PushCallback): void;

  /**
   * Pops the next message (blocks a libuv thread until one is available). Do not use together with
   * the `'data'` event.
   */
  pop(callback: MessageQueue.PopCallback): void;
  pop(options: MessageQueue.PopOptions | undefined, callback: MessageQueue.PopCallback): void;

  /** Removes the queue from the system (IPC_RMID). Returns whether it was closed. */
  close(callback?: MessageQueue.CloseCallback): boolean;

  /** Starts a receive loop on the first `'data'` listener. */
  on(event: 'data', listener: (data: Buffer) => void): this;
  on(event: 'error', listener: (err: Error) => void): this;
  on(event: string | symbol, listener: (...args: any[]) => void): this;
  once(event: 'data', listener: (data: Buffer) => void): this;
  once(event: 'error', listener: (err: Error) => void): this;
  once(event: string | symbol, listener: (...args: any[]) => void): this;
  addListener(event: 'data', listener: (data: Buffer) => void): this;
  addListener(event: 'error', listener: (err: Error) => void): this;
  addListener(event: string | symbol, listener: (...args: any[]) => void): this;
}

declare namespace MessageQueue {
  interface PushOptions {
    /** Message type, a positive integer. Default 1. */
    type?: number;
    /** msgsnd flags, e.g. 2048 (IPC_NOWAIT on Linux) to fail with EAGAIN instead of blocking. Default 0. */
    flags?: number;
  }

  interface PopOptions {
    /** 0 (default): first message; > 0: first message of that type; < 0: lowest type <= |type|. */
    type?: number;
    /** msgrcv flags. Default 0. */
    flags?: number;
  }

  type PushCallback = (err?: Error | null) => void;
  type PopCallback = (err: Error | null, data: Buffer) => void;
  type CloseCallback = (err: Error | null, closed: boolean) => void;

  interface NativeBindings {
    /** msgget; throws on failure. */
    get(key: number, flags: number): number;
    /** msgsnd on a worker thread. */
    snd(id: number, data: Buffer, type: number, flags: number, callback: (err: Error | null) => void): void;
    /** msgrcv on a worker thread. */
    rcv(id: number, bufferLength: number, type: number, flags: number, callback: (err: Error | null, data: Buffer | null) => void): void;
    /** msgctl; returns the raw msqid_ds Buffer for IPC_STAT on success, otherwise msgctl's return value. Throws a TypeError for IPC_SET (unsupported: it would zero the queue's uid/gid/mode/qbytes). */
    ctl(id: number, cmd: number): Buffer | number;
    /** msgctl(IPC_RMID); throws on failure. */
    close(id: number): true;
  }
}

export = MessageQueue;
