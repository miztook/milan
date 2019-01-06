#include "mygfx/mygfx.h"

#pragma comment(lib, "mygfx.lib")
#pragma comment(lib, "example-common.lib")

#include <bx/uint32_t.h>
#include "common.h"
#include "bgfx_utils.h"


class ExampleHelloWorld : public entry::AppI
{
public:
	ExampleHelloWorld(const char* _name, const char* _description)
		: entry::AppI(_name, _description)
	{
	}

	void init(int32_t _argc, const char* const* _argv, uint32_t _width, uint32_t _height) override
	{
	
	}

	virtual int shutdown() override
	{
		return 0;
	}

	bool update() override
	{
		return false;
	}
};

int _main_(int _argc, char** _argv)
{
	ExampleHelloWorld app("00-helloworld", "Initialization and debug text.");
	return entry::runApp(&app, _argc, _argv);
}
