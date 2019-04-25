// addon.cc
#include <node.h>
#include "ftdi_context_wrapper.h"

namespace demo {

using v8::Local;
using v8::Object;

void InitAll(Local<Object> exports) {
  FtdiContextWrapper::Init(exports);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, InitAll)

}  // namespace demo
