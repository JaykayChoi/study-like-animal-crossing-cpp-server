// ----------------------------------------------------------------------------
// COPYRIGHT (C)2017 BY MOTIF CO., LTD. ALL RIGHTS RESERVED.
// ----------------------------------------------------------------------------
import * as crypto from 'crypto';

// ----------------------------------------------------------------------------
// Constants.
// ----------------------------------------------------------------------------
const Const = {
  Algo: 'aes-128-cbc',
  CurveName: 'secp224r1',
  AesKeySize: 16,

  IV: Buffer.from([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15]),
};

// ----------------------------------------------------------------------------
// BotCryptoContext object.
// ----------------------------------------------------------------------------
interface BotCryptoContext {
  ecdh: crypto.ECDH;
  publicKey: Buffer;
  secret: Buffer;
  aesKey: Buffer;

  init(): void;
  encrypt(buf: Buffer): Buffer;
  decrypt(buf: Buffer): Buffer;
  computeSecret(peerPublicKey: Buffer): void;
}

class BotCryptoContext {
  constructor() {
    this.ecdh = null;
    this.publicKey = null;
    this.secret = null;
    this.aesKey = null;
  }
}

BotCryptoContext.prototype.init = function () {
  this.ecdh = crypto.createECDH(Const.CurveName);
  this.publicKey = this.ecdh.generateKeys();
};

BotCryptoContext.prototype.encrypt = function (buf) {
  console.debug('encrypting buffer', { buf: buf.toString('hex') });

  const cipher = crypto.createCipheriv(Const.Algo, this.aesKey, Const.IV);
  let encrypted = (cipher.update as any)(buf, 'hex', 'hex');
  encrypted += cipher.final('hex');

  console.debug('encrypted buffer', { buf: encrypted });
  return Buffer.from(encrypted, 'hex');
};

BotCryptoContext.prototype.decrypt = function (buf) {
  console.debug('decrypting buffer', { buf: buf.toString('hex') });

  const decipher = crypto.createDecipheriv(Const.Algo, this.aesKey, Const.IV);
  let decrypted = (decipher.update as any)(buf, 'hex', 'hex');
  decrypted += decipher.final('hex');

  console.debug('decrypted buffer', { buf: decrypted });
  return Buffer.from(decrypted, 'hex');
};

BotCryptoContext.prototype.computeSecret = function (peerPublicKey) {
  this.secret = this.ecdh.computeSecret(peerPublicKey);
  this.aesKey = this.secret.slice(0, Const.AesKeySize);
};

export default BotCryptoContext;
