// N-API port (node-addon-api) of the former NAN addon: one binary per platform works across Node majors.
#include <napi.h>
#include <sys/msg.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <string>
#include <vector>

#ifndef __linux__
struct msgbuf {
  long mtype;
  char mtext[1];
};
#endif

namespace {

// Same limit as MessageQueue.MSGMAX in lib/queue.js.
const double kMsgMax = 4052;

std::string ErrorText(const std::string &message, int errcode) {
  return message + ": " + strerror(errcode);
}

// msgsnd on a worker thread. Copies the payload so the JS Buffer may be collected while the call blocks.
class SendWorker : public Napi::AsyncWorker {
 public:
  SendWorker(Napi::Function cb, int id, const char *data, size_t length, long type, int flags)
      : Napi::AsyncWorker(cb), id(id), type(type), flags(flags), ret(0), error(0),
        message(sizeof(long) + length), dataLength(length) {
    ((struct msgbuf *)message.data())->mtype = type;
    if (length > 0) memcpy(((struct msgbuf *)message.data())->mtext, data, length);
  }

  void Execute() override {
    ret = msgsnd(id, message.data(), dataLength, flags);
    error = errno;
  }

  void OnOK() override {
    Napi::Env env = Env();
    Napi::HandleScope scope(env);
    if (ret == -1) {
      Callback().Call({Napi::Error::New(env, ErrorText("Failed to send message", error)).Value()});
    } else {
      Callback().Call({env.Null()});
    }
  }

 private:
  int id;
  long type;
  int flags;
  int ret;
  int error;
  std::vector<char> message;
  size_t dataLength;
};

// msgrcv on a worker thread.
class ReceiveWorker : public Napi::AsyncWorker {
 public:
  ReceiveWorker(Napi::Function cb, int id, size_t bufferLength, long type, int flags)
      : Napi::AsyncWorker(cb), id(id), bufferLength(bufferLength), type(type), flags(flags),
        received(0), error(0), message(sizeof(long) + bufferLength) {}

  void Execute() override {
    received = msgrcv(id, message.data(), bufferLength, type, flags);
    error = errno;
  }

  void OnOK() override {
    Napi::Env env = Env();
    Napi::HandleScope scope(env);
    if (received == (ssize_t)-1) {
      Callback().Call({Napi::Error::New(env, ErrorText("Failed to receive message", error)).Value(), env.Null()});
    } else {
      Callback().Call({env.Null(), Napi::Buffer<char>::Copy(env, ((struct msgbuf *)message.data())->mtext, (size_t)received)});
    }
  }

 private:
  int id;
  size_t bufferLength;
  long type;
  int flags;
  ssize_t received;
  int error;
  std::vector<char> message;
};

bool CheckArgs(const Napi::CallbackInfo &info, size_t count) {
  if (info.Length() < count) {
    Napi::TypeError::New(info.Env(), "Wrong number of arguments").ThrowAsJavaScriptException();
    return false;
  }
  return true;
}

// get(key, flags) -> queue id; throws on failure.
Napi::Value Get(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (!CheckArgs(info, 2)) return env.Undefined();
  key_t key = (key_t)info[0].ToNumber().Int32Value();
  int flags = info[1].ToNumber().Int32Value();
  int queue = msgget(key, flags);
  if (queue == -1) {
    Napi::Error::New(env, ErrorText("Failed to get queue", errno)).ThrowAsJavaScriptException();
    return env.Undefined();
  }
  return Napi::Number::New(env, queue);
}

// snd(id, buffer, type, flags, callback(err))
Napi::Value Send(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (!CheckArgs(info, 5)) return env.Undefined();
  if (!info[1].IsBuffer() || !info[4].IsFunction()) {
    Napi::TypeError::New(env, "snd(id, buffer, type, flags, callback)").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  Napi::Buffer<char> buf = info[1].As<Napi::Buffer<char>>();
  (new SendWorker(info[4].As<Napi::Function>(), info[0].ToNumber().Int32Value(), buf.Data(), buf.Length(),
                  (long)info[2].ToNumber().Int32Value(), info[3].ToNumber().Int32Value()))
      ->Queue();
  return env.Undefined();
}

// rcv(id, bufferLength, type, flags, callback(err, data))
Napi::Value Receive(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (!CheckArgs(info, 5)) return env.Undefined();
  if (!info[4].IsFunction()) {
    Napi::TypeError::New(env, "rcv(id, bufferLength, type, flags, callback)").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  double length = info[1].ToNumber().DoubleValue();
  if (!(length >= 0 && length <= kMsgMax)) {
    Napi::TypeError::New(env, "bufferLength must be between 0 and MSGMAX").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  (new ReceiveWorker(info[4].As<Napi::Function>(), info[0].ToNumber().Int32Value(),
                     (size_t)length, (long)info[2].ToNumber().Int32Value(),
                     info[3].ToNumber().Int32Value()))
      ->Queue();
  return env.Undefined();
}

// ctl(id, cmd) -> Buffer with the raw msqid_ds on IPC_STAT success, else msgctl's return value.
// IPC_SET is rejected with a TypeError: it would write a zeroed msqid_ds (uid/gid/mode/qbytes reset to 0).
Napi::Value Control(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (!CheckArgs(info, 2)) return env.Undefined();
  int id = info[0].ToNumber().Int32Value();
  int cmd = info[1].ToNumber().Int32Value();

  if (cmd == IPC_SET) {
    Napi::TypeError::New(env, "IPC_SET is not supported").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  if (cmd != IPC_STAT) {
    return Napi::Number::New(env, msgctl(id, cmd, nullptr));
  }

  msqid_ds buf;
  memset(&buf, 0, sizeof(buf));
  int ret = msgctl(id, cmd, &buf);
  if (ret == 0) return Napi::Buffer<char>::Copy(env, (const char *)&buf, sizeof(buf));
  return Napi::Number::New(env, ret);
}

// close(id) -> true; throws on failure (removes the queue).
Napi::Value Close(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (!CheckArgs(info, 1)) return env.Undefined();
  int id = info[0].ToNumber().Int32Value();
  if (msgctl(id, IPC_RMID, nullptr) != 0) {
    Napi::Error::New(env, ErrorText("Failed to close queue", errno)).ThrowAsJavaScriptException();
    return env.Undefined();
  }
  return Napi::Boolean::New(env, true);
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set("get", Napi::Function::New(env, Get));
  exports.Set("snd", Napi::Function::New(env, Send));
  exports.Set("rcv", Napi::Function::New(env, Receive));
  exports.Set("ctl", Napi::Function::New(env, Control));
  exports.Set("close", Napi::Function::New(env, Close));
  return exports;
}

}  // namespace

NODE_API_MODULE(msg, Init)
