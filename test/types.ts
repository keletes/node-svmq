// Type test, compiled by `yarn test:types` (never run). Mirrors how the Keletes services import svmq.
import MessageQueue from '..';
import MessageQueueRequire = require('..');

const queue: MessageQueue = new MessageQueue(31337);
const same: MessageQueue = MessageQueue.open(31337, 0o666);
new MessageQueueRequire(1, 0o666, {});

queue.push('text');
queue.push(Buffer.from('x'), (err) => err?.message);
queue.push('text', { type: 2, flags: 2048 }, (err?: Error | null) => void err);

queue.pop((err, data) => {
  if (err) throw err;
  data.toString();
});
queue.pop({ type: 3 }, (err: Error | null, data: Buffer) => void (err ?? data.length));

queue.on('data', (data) => data.toString('utf8'));
queue.on('error', (err) => err.message);

const closed: boolean = queue.close((err, ok) => void (err ?? ok));
const flags: number = MessageQueue.formatFlags(0o666);
const max: number = MessageQueue.MSGMAX + MessageQueue.IPC_CREATE + flags;
const id: number = MessageQueue.msg.get(1, 950);
MessageQueue.msg.snd(id, Buffer.alloc(1), 1, 0, (err) => void err);
const key: number = same.key + max + Number(closed);

// @ts-expect-error key must be a number
new MessageQueue('31337');
// @ts-expect-error pop needs a callback
queue.pop({ type: 1 });

export { key };
