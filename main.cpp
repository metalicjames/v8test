#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include <v8.h>
#include <libplatform/libplatform.h>
#include <v8pp/context.hpp>
#include <v8pp/class.hpp>
#include <v8pp/module.hpp>

#include <cryptokernel/crypto.h>

/**
* Uses the address of a local variable to determine the stack top now.
* Given a size, returns an address that is that far from the current
* top of stack.
*
* @param size the size of the stack limit in bits
* @return a pointer to the address of the bottom of the stack
*/
uint32_t* ComputeStackLimit(uint32_t size) {
  uint32_t* answer = &size - (size / sizeof(size));
  // If the size is very large and the stack is very near the bottom of
  // memory then the calculation above may wrap around and give an address
  // that is above the (downwards-growing) stack.  In that case we return
  // a very low address.
  if(answer > &size)
  {
      return reinterpret_cast<uint32_t*>(sizeof(size));
  }

  return answer;
}

/**
* Program entry point
*
* @return 0 if the program completed successfully
* @throw runtime_error if there was a problem executing the script
*/
int main(int argc, char* argv[])
{
    v8::V8::InitializeICU();
    v8::V8::InitializeExternalStartupData(argv[0]);
	std::unique_ptr<v8::Platform> platform(v8::platform::CreateDefaultPlatform());
	v8::V8::InitializePlatform(platform.get());
	v8::V8::Initialize();

	try
	{
        const std::string source = "var cryptoLib = new CK.Crypto(true); cryptoLib.getPublicKey() + cryptoLib.getPrivateKey();";

        v8::Isolate::CreateParams params;
        params.constraints.set_stack_limit(ComputeStackLimit(20000));
        params.constraints.set_max_executable_size(3);

        std::unique_ptr<v8::ArrayBuffer::Allocator> arrayBuffer(v8::ArrayBuffer::Allocator::NewDefaultAllocator());
        params.array_buffer_allocator = arrayBuffer.get();

        v8::Isolate* isolate = v8::Isolate::New(params);
        isolate->Enter();

        v8pp::context context(isolate, arrayBuffer.get());
        v8::HandleScope scope(context.isolate());

        v8pp::class_<CryptoKernel::Crypto> cryptoClass(context.isolate());
        cryptoClass.ctor<const bool>();
        cryptoClass.set("getPublicKey", &CryptoKernel::Crypto::getPublicKey);
        cryptoClass.set("getPrivateKey", &CryptoKernel::Crypto::getPrivateKey);

        v8pp::module CK(context.isolate());

        CK.set("Crypto", cryptoClass);

        context.isolate()->GetCurrentContext()->Global()->Set(v8::String::NewFromUtf8(context.isolate(), "CK"), CK.new_instance());

        v8::TryCatch tryCatch(context.isolate());

        v8::Handle<v8::Value> result = context.run_script(source);
        if(tryCatch.HasCaught())
        {
            const std::string msg = v8pp::from_v8<std::string>(context.isolate(), tryCatch.Exception()->ToString());
            throw std::runtime_error(msg);
        }

        const std::string scriptResult = v8pp::from_v8<std::string>(context.isolate(), result);

        std::cout << scriptResult;
	}
	catch(std::exception& e)
	{
        throw std::runtime_error(e.what());
	}

    v8::V8::Dispose();
	v8::V8::ShutdownPlatform();

    return 0;
}
