#include <iostream>

#include <v8.h>
#include <libplatform/libplatform.h>
#include <v8pp/context.hpp>

/**
* Program entry point
*
* @return 0 if the program completed successfully
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
        v8pp::context context;

        const std::string source = "2 + 6";

        v8::Isolate* isolate = context.isolate();

        v8::HandleScope scope(isolate);
        v8::TryCatch tryCatch;
        v8::Handle<v8::Value> result = context.run_script(source);
        if(tryCatch.HasCaught())
        {
            const std::string msg = v8pp::from_v8<std::string>(isolate, tryCatch.Exception()->ToString());
            throw std::runtime_error(msg);
        }

        const int scriptResult = v8pp::from_v8<int>(isolate, result);

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
