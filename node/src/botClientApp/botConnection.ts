// ----------------------------------------------------------------------------
// COPYRIGHT (C)2017 BY MOTIF CO., LTD. ALL RIGHTS RESERVED.
// ----------------------------------------------------------------------------
import * as net from 'net';
import Queue from 'queue-fifo';
import { SmartBuffer } from 'smart-buffer';
import _ from 'lodash';
import zlib from 'zlib';
import { promisify } from 'util';

import BotCryptoContext from './botCryptoContext';
import { PacketType } from './botEnums';

// ----------------------------------------------------------------------------
// Constants.
// ----------------------------------------------------------------------------

export enum PayloadFlag {
  None = 0,
  Encrypt = 1,
  Compress = 1 << 1,
  Binary = 1 << 2,

  Logout = 1 << 7,
}

export enum Const {
  HeaderLen = 4,
  RecvBufSize = 16384 * 2,

  BinaryPacketHeadLen = 4 * 1,
  BinaryPayloadFlags = PayloadFlag.Binary,

  TownMoveTxPacketSize = BinaryPacketHeadLen + 4 * 4, // TOWN_MOVE_TX_PACKET_SIZE in NetCommon.h
  TownMoveRxPacketSize = BinaryPacketHeadLen + 4 * 5, // TOWN_MOVE_RX_PACKET_SIZE in NetCommon.h

  OceanMoveTxPacketSize = BinaryPacketHeadLen + 4 * 3 + 8 * 2, // OCEAN_MOVE_TX_PACKET_SIZE in NetCommon.h
  OceanNetUserMoveRxPacketSize = BinaryPacketHeadLen + 4 * 3 + 8 * 2, // OCEAN_NET_USER_MOVE_RX_PACKET_SIZE in NetCommon.h
}

export enum State {
  INITIALIZED = 0,
  CONNECTING = 1,
  CONNECTED = 2,
  EXCHANGING_KEYS = 3,
  LOGGING_IN = 4,
  LOGGED_IN = 5,
  USER_NAME_SET = 6,
  FIRST_MATE_SET = 7,
  CHANGE_COMPANY_JOB = 8,

  DISCONNECTED = 10,
  ERROR = 100,
}

interface Payload {
  flags: number;
  buffer: Buffer;
}

export const compress = promisify(zlib.deflate);
export const uncompress = promisify(zlib.inflate);

// ----------------------------------------------------------------------------
// BotConnection object.
// ----------------------------------------------------------------------------
class BotConnection {
  private pubId: string;
  private userName: string;
  private _userId: number = 0;
  private sock: net.Socket;
  private recvBuf: Buffer;
  private bytesReceived: number;
  private cryptoCtx: BotCryptoContext;
  private state: State;
  private packetQueue: Queue<any>;
  private lobbydHost: string;
  private lobbydPort: number;

  constructor(inPubId: string, userName: string) {
    // User.
    this.pubId = inPubId;
    this.userName = userName;

    // Socket.
    this.sock = null;
    this.recvBuf = Buffer.allocUnsafe(Const.RecvBufSize);
    this.bytesReceived = 0;
    this.setLobbydAddr('localhost', 20100);

    // Encryption.
    this.cryptoCtx = new BotCryptoContext();

    // State.
    this.state = State.INITIALIZED;

    // Received packet queue.
    this.packetQueue = new Queue();
  }

  get getCryptoCtx() {
    return this.cryptoCtx;
  }

  public getUserId(): number {
    return this._userId;
  }
  public setUserId(value: number) {
    this._userId = value;
  }

  connect(): void {
    if (this.sock) {
      return;
    }

    this.sock = net.connect({ host: this.lobbydHost, port: this.lobbydPort }, () => {
      console.info('connected to lobbyd', {
        pubId: this.pubId,
      });

      this.setState(State.CONNECTED);

      this.onConnected();

      this.sock.on('data', (data) => {
        console.debug('SOCK EVT data', { pubId: this.pubId });
        this.onRecv(data);
      });

      this.sock.on('end', () => {
        console.debug('SOCK EVT end', { pubId: this.pubId });
        this.disconnect();
      });

      this.sock.on('error', (err) => {
        console.debug('SOCK EVT error', { pubId: this.pubId, error: err.message });
        this.disconnect();
      });

      this.sock.on('close', () => {
        console.debug('SOCK EVT close', { pubId: this.pubId });
        this.disconnect();
      });
    });
  }

  setLobbydAddr(addr: string, port: number) {
    this.lobbydHost = addr;
    this.lobbydPort = port;
  }

  disconnect(): void {
    if (this.state === State.DISCONNECTED) {
      return;
    }
    console.info('bot-con disconnect');

    if (this.sock) {
      this.sock.destroy();
      this.sock = null;
    }

    this.setState(State.DISCONNECTED);
  }

  reconnect(): void {
    this.disconnect();
    this.state = State.INITIALIZED;
    this.connect();
  }

  isLoginProcessCompleted(): boolean {
    return this.state === State.CHANGE_COMPANY_JOB;
  }

  isDisconnected(): boolean {
    return this.state === State.DISCONNECTED;
  }

  clearPacketQueue(): void {
    this.packetQueue.clear();
  }

  popPacket() {
    return this.packetQueue.dequeue();
  }

  sendJsonPacket(packet, payloadFlags = PayloadFlag.Compress | PayloadFlag.Encrypt): Promise<any> {
    const packetJson = JSON.stringify(packet);
    const packetBuf = Buffer.from(packetJson, 'utf-8');

    console.info('sending packet', {
      userId: this._userId,
      packetType: packet.type,
      packetJson,
    });

    return Promise.resolve()
      .then(() => {
        if (payloadFlags & PayloadFlag.Compress) {
          return compress(packetBuf);
        }

        return packetBuf;
      })
      .then((buf: Buffer) => {
        if (payloadFlags & PayloadFlag.Encrypt) {
          return this.cryptoCtx.encrypt(buf);
        }

        return buf;
      })
      .then((buf) => {
        const packetSize = buf.byteLength;
        const sendBuf = Buffer.alloc(Const.HeaderLen + packetSize);

        const firstByte = (packetSize & 0xff00) >> 8;
        const secondByte = packetSize & 0x00ff;

        sendBuf[0] = firstByte;
        sendBuf[1] = secondByte;
        sendBuf[2] = payloadFlags;

        sendBuf.fill(buf, Const.HeaderLen);

        return this.sock.write(sendBuf);
      })
      .catch((err) => {
        console.error('failed to send', {
          error: err.message,
          packetType: packet.type,
        });

        if (!this.isDisconnected()) {
          this.setState(State.ERROR);
        }
      });
  }

  sendBinaryPacket(packetType: number, bufBody: Buffer): Promise<any> {
    return Promise.resolve()
      .then(() => {
        const packetSize = bufBody.byteLength;
        const firstByte = (packetSize & 0xff00) >> 8;
        const secondByte = packetSize & 0x00ff;

        const smartBuffer = SmartBuffer.fromOptions({
          size: 4096,
        });
        smartBuffer.writeUInt8(firstByte);
        smartBuffer.writeUInt8(secondByte);
        smartBuffer.writeUInt8(Const.BinaryPayloadFlags);
        smartBuffer.writeUInt8(0);

        // binary packet
        smartBuffer.writeBuffer(bufBody);
        const sendBuf = smartBuffer.toBuffer();

        return this.sock.write(sendBuf);
      })
      .catch((err) => {
        console.error('failed to send', {
          error: err.message,
          packetType,
        });

        if (!this.isDisconnected()) {
          this.setState(State.ERROR);
        }
      });
  }

  private onConnected(): void {
    // Send hello.
    this.setState(State.EXCHANGING_KEYS);

    this.cryptoCtx.init();

    const body = {
      publicKey: this.cryptoCtx.publicKey.toString('hex'),
    };

    const packet = {
      type: PacketType.Hello,
      seqNum: 0,
      body: JSON.stringify(body),
    };

    this.sendJsonPacket(packet, PayloadFlag.None);
  }

  // private onRecv(data) {
  //   // Old code.
  //   // const payload = this.readPayload(data);
  //   // if (!payload) {
  //   //   return;
  //   // }
  //   // return this.processPayload(payload);

  //   const payloads = this.readPayloads(data);
  //   if (payloads.length === 0) {
  //     return;
  //   }

  //   return (Promise as any).reduce(
  //     payloads,
  //     (_accum, payload) => {
  //       return this.processPayload(payload);
  //     },
  //     null
  //   );
  // }
  private onRecv(data) {
    // Old code.
    // const payload = this.readPayload(data);
    // if (!payload) {
    //   return;
    // }
    // return this.processPayload(payload);

    const payloads = this.readPayloads(data);
    if (payloads.length === 0) {
      return;
    }

    payloads.forEach((payload) => {
      return this.processPayload(payload);
    });
  }

  private processPayload(payload) {
    return this.parsePacket(payload)
      .then((packet) => {
        if (!packet) {
          // 현재 binary packet들은 처리안하고 있음
          return;
        }

        return this.packetQueue.enqueue(packet);

        /////////////////////////////
        // below codes moved to botClient
      })
      .catch((err) => {
        console.error('onRecv failed', {
          pubId: this.pubId,
          userId: this._userId,
          error: err.message,
        });

        console.error(err.stack);

        this.disconnect();
      });
  }

  private calcFreeRecvBufSize(): number {
    const totalSize = this.recvBuf.length;
    if (this.bytesReceived >= totalSize) {
      return 0;
    }
    return totalSize - this.bytesReceived;
  }

  private readPayloads(buf): Payload[] {
    const freeSize = this.calcFreeRecvBufSize();
    if (freeSize < buf.byteLength) {
      console.error('readPayload recvBuff overflow', {
        userId: this._userId,
        incommingBytesLength: buf.byteLength,
        freeSize,
      });
      this.disconnect();
      return [];
    }

    // First copy everything into the recv buffer.
    buf.copy(this.recvBuf, this.bytesReceived, 0, buf.byteLength);
    this.bytesReceived += buf.byteLength;

    // Read as many payloads as we can.
    const payloads = [];
    while (true) {
      if (this.bytesReceived < Const.HeaderLen) {
        // Need to read more.
        break;
      }

      const firstByte = this.recvBuf[0];
      const secondByte = this.recvBuf[1];
      const payloadFlags = this.recvBuf[2];

      let payloadSize = 0;
      payloadSize |= (firstByte & 0xff) << 8;
      payloadSize |= secondByte & 0xff;

      if (this.bytesReceived < Const.HeaderLen + payloadSize) {
        // Need to read more.
        break;
      }

      // We have enough to read to make a packet.
      // Copy the payload to separate buffer,
      // and discard the data read from the recv buffer.
      const numBytesToDiscard = Const.HeaderLen + payloadSize;
      const numBytesRemaining = this.bytesReceived - numBytesToDiscard;

      const payloadBuffer = Buffer.alloc(payloadSize);
      this.recvBuf.copy(payloadBuffer, 0, Const.HeaderLen, numBytesToDiscard);
      this.recvBuf.copy(this.recvBuf, 0, numBytesToDiscard, numBytesToDiscard + numBytesRemaining);
      this.bytesReceived = numBytesRemaining;

      // console.debug('[TEMP] readPayload', {
      //   pubId: this.pubId,
      //   bytesReceived: this.bytesReceived,
      // });

      // payloads.push(Buffer.from(payloadBuffer.buffer, 4));
      // const offset = (payloadFlags & PayloadFlag.Compress) ? 4 : 0;
      const payload = {
        flags: payloadFlags,
        buffer: null,
      };

      if (payloadFlags & PayloadFlag.Compress) {
        // + 4 for 'uncompressed size' of the payload.
        payload.buffer = Buffer.from(payloadBuffer.buffer, 4);
      } else {
        payload.buffer = payloadBuffer;
      }

      payloads.push(payload);
    }

    return payloads;
  }

  private parsePacket(payload): Promise<any> {
    const flags = payload.flags;
    const payloadBuf = payload.buffer;

    return Promise.resolve()
      .then(() => {
        // Decrypt if necessary.
        if (flags & PayloadFlag.Encrypt) {
          return this.cryptoCtx.decrypt(payloadBuf);
        }

        return payloadBuf;
      })
      .then((buf) => {
        // Unzip.
        if (flags & PayloadFlag.Compress) {
          return uncompress(buf);
        }

        return buf;
      })
      .then((buf) => {
        // Return parsed packet.
        if (flags & PayloadFlag.Binary) {
          return this.parseBinaryPacket(buf);
        }

        return JSON.parse(buf);
      });
  }

  private parseBinaryPacket(buf: Buffer): any {
    // todo
  }

  public setState(newState: State): void {
    // console.debug('state change', {
    //   oldState: this.state,
    //   newState,
    // });

    this.state = newState;
  }
}

function parseMoveInTown(buf): any {
  let pos = 4;
  const userId = buf.readInt32LE(pos);
  pos += 4;
  const x = buf.readInt32LE(pos);
  pos += 4;
  const y = buf.readInt32LE(pos);
  pos += 4;
  const degrees = buf.readInt32LE(pos);
  pos += 4;
  const speed = buf.readInt32LE(pos);

  const packet = {
    seqNum: 0,
    userId,
    x,
    y,
    degrees,
    speed,
  };

  // console.verbose('received move-in-town - ', JSON.stringify(packet));

  return packet;
}

export default BotConnection;
