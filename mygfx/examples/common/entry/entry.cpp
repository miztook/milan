#include <bx/bx.h>
#include <bx/file.h>
#include <bx/sort.h>
#include <mygfx/mygfx.h>

#include <time.h>

#include "entry_p.h"

namespace entry
{
	static uint32_t s_debug = MYGFX_DEBUG_NONE;
	static uint32_t s_reset = MYGFX_RESET_NONE;
	static uint32_t s_width = ENTRY_DEFAULT_WIDTH;
	static uint32_t s_height = ENTRY_DEFAULT_HEIGHT;
	static bool s_exit = false;

	static bx::FileReaderI* s_fileReader = NULL;
	static bx::FileWriterI* s_fileWriter = NULL;

	extern bx::AllocatorI* getDefaultAllocator();
	bx::AllocatorI* g_allocator = getDefaultAllocator();

	typedef bx::StringT<&g_allocator> String;

	static String s_currentDir;

	class FileReader : public bx::FileReader
	{
		typedef bx::FileReader super;

	public:
		virtual bool open(const bx::FilePath& _filePath, bx::Error* _err) override
		{
			String filePath(s_currentDir);
			filePath.append(_filePath.get());
			return super::open(filePath.getPtr(), _err);
		}
	};

	class FileWriter : public bx::FileWriter
	{
		typedef bx::FileWriter super;

	public:
		virtual bool open(const bx::FilePath& _filePath, bool _append, bx::Error* _err) override
		{
			String filePath(s_currentDir);
			filePath.append(_filePath.get());
			return super::open(filePath.getPtr(), _append, _err);
		}
	};

	void setCurrentDir(const char* _dir)
	{
		s_currentDir.set(_dir);
	}

#if ENTRY_CONFIG_IMPLEMENT_DEFAULT_ALLOCATOR
	bx::AllocatorI* getDefaultAllocator()
	{
		BX_PRAGMA_DIAGNOSTIC_PUSH();
		BX_PRAGMA_DIAGNOSTIC_IGNORED_MSVC(4459); // warning C4459: declaration of 's_allocator' hides global declaration
		BX_PRAGMA_DIAGNOSTIC_IGNORED_CLANG_GCC("-Wshadow");
		static bx::DefaultAllocator s_allocator;
		return &s_allocator;
		BX_PRAGMA_DIAGNOSTIC_POP();
	}
#endif // ENTRY_CONFIG_IMPLEMENT_DEFAULT_ALLOCATOR

	static const char* s_keyName[] =
	{
		"None",
		"Esc",
		"Return",
		"Tab",
		"Space",
		"Backspace",
		"Up",
		"Down",
		"Left",
		"Right",
		"Insert",
		"Delete",
		"Home",
		"End",
		"PageUp",
		"PageDown",
		"Print",
		"Plus",
		"Minus",
		"LeftBracket",
		"RightBracket",
		"Semicolon",
		"Quote",
		"Comma",
		"Period",
		"Slash",
		"Backslash",
		"Tilde",
		"F1",
		"F2",
		"F3",
		"F4",
		"F5",
		"F6",
		"F7",
		"F8",
		"F9",
		"F10",
		"F11",
		"F12",
		"NumPad0",
		"NumPad1",
		"NumPad2",
		"NumPad3",
		"NumPad4",
		"NumPad5",
		"NumPad6",
		"NumPad7",
		"NumPad8",
		"NumPad9",
		"Key0",
		"Key1",
		"Key2",
		"Key3",
		"Key4",
		"Key5",
		"Key6",
		"Key7",
		"Key8",
		"Key9",
		"KeyA",
		"KeyB",
		"KeyC",
		"KeyD",
		"KeyE",
		"KeyF",
		"KeyG",
		"KeyH",
		"KeyI",
		"KeyJ",
		"KeyK",
		"KeyL",
		"KeyM",
		"KeyN",
		"KeyO",
		"KeyP",
		"KeyQ",
		"KeyR",
		"KeyS",
		"KeyT",
		"KeyU",
		"KeyV",
		"KeyW",
		"KeyX",
		"KeyY",
		"KeyZ",
		"GamepadA",
		"GamepadB",
		"GamepadX",
		"GamepadY",
		"GamepadThumbL",
		"GamepadThumbR",
		"GamepadShoulderL",
		"GamepadShoulderR",
		"GamepadUp",
		"GamepadDown",
		"GamepadLeft",
		"GamepadRight",
		"GamepadBack",
		"GamepadStart",
		"GamepadGuide",
	};
	BX_STATIC_ASSERT(Key::Count == BX_COUNTOF(s_keyName));

	const char* getName(Key::Enum _key)
	{
		BX_CHECK(_key < Key::Count, "Invalid key %d.", _key);
		return s_keyName[_key];
	}

	static AppI*    s_currentApp = NULL;
	static AppI*    s_apps = NULL;
	static uint32_t s_numApps = 0;

	static char s_restartArgs[1024] = { '\0' };

	static AppI* getCurrentApp(AppI* _set = NULL)
	{
		if (NULL != _set)
		{
			s_currentApp = _set;
		}
		else if (NULL == s_currentApp)
		{
			s_currentApp = getFirstApp();
		}

		return s_currentApp;
	}

	static AppI* getNextWrap(AppI* _app)
	{
		AppI* next = _app->getNext();
		if (NULL != next)
		{
			return next;
		}

		return getFirstApp();
	}


	AppI::AppI(const char* _name, const char* _description)
	{
		m_name = _name;
		m_description = _description;
		m_next = s_apps;

		s_apps = this;
		s_numApps++;
	}

	AppI::~AppI()
	{
		for (AppI* prev = NULL, *app = s_apps, *next = app->getNext()
			; NULL != app
			; prev = app, app = next, next = app->getNext())
		{
			if (app == this)
			{
				if (NULL != prev)
				{
					prev->m_next = next;
				}
				else
				{
					s_apps = next;
				}

				--s_numApps;

				break;
			}
		}
	}

	const char* AppI::getName() const
	{
		return m_name;
	}

	const char* AppI::getDescription() const
	{
		return m_description;
	}

	AppI* AppI::getNext()
	{
		return m_next;
	}

	AppI* getFirstApp()
	{
		return s_apps;
	}

	uint32_t getNumApps()
	{
		return s_numApps;
	}

	int runApp(AppI* _app, int _argc, const char* const* _argv)
	{
		_app->init(_argc, _argv, s_width, s_height);
		mygfx::frame();

		WindowHandle defaultWindow = { 0 };
		setWindowSize(defaultWindow, s_width, s_height);

		while (_app->update())
		{
			if (0 != bx::strLen(s_restartArgs))
			{
				break;
			}
		}

		return _app->shutdown();
	}

	static int32_t sortApp(const void* _lhs, const void* _rhs)
	{
		const AppI* lhs = *(const AppI**)_lhs;
		const AppI* rhs = *(const AppI**)_rhs;

		return bx::strCmpI(lhs->getName(), rhs->getName());
	}

	static void sortApps()
	{
		if (2 > s_numApps)
		{
			return;
		}

		AppI** apps = (AppI**)BX_ALLOC(g_allocator, s_numApps * sizeof(AppI*));

		uint32_t ii = 0;
		for (AppI* app = getFirstApp(); NULL != app; app = app->getNext())
		{
			apps[ii++] = app;
		}
		bx::quickSort(apps, s_numApps, sizeof(AppI*), sortApp);

		s_apps = apps[0];
		for (ii = 1; ii < s_numApps; ++ii)
		{
			AppI* app = apps[ii - 1];
			app->m_next = apps[ii];
		}
		apps[s_numApps - 1]->m_next = NULL;

		BX_FREE(g_allocator, apps);
	}

	int main(int _argc, const char* const* _argv)
	{
		//DBG(BX_COMPILER_NAME " / " BX_CPU_NAME " / " BX_ARCH_NAME " / " BX_PLATFORM_NAME);

		s_fileReader = BX_NEW(g_allocator, FileReader);
		s_fileWriter = BX_NEW(g_allocator, FileWriter);

		entry::WindowHandle defaultWindow = { 0 };

		bx::FilePath fp(_argv[0]);
		char title[bx::kMaxFilePath];
		bx::strCopy(title, BX_COUNTOF(title), fp.getBaseName());

		entry::setWindowTitle(defaultWindow, title);
		setWindowSize(defaultWindow, ENTRY_DEFAULT_WIDTH, ENTRY_DEFAULT_HEIGHT);

		sortApps();

		const char* find = "";
		if (1 < _argc)
		{
			find = _argv[_argc - 1];
		}

	restart:
		AppI* selected = NULL;

		for (AppI* app = getFirstApp(); NULL != app; app = app->getNext())
		{
			if (NULL == selected
				&& !bx::strFindI(app->getName(), find).isEmpty())
			{
				selected = app;
			}
		}

		int32_t result = bx::kExitSuccess;
		s_restartArgs[0] = '\0';
		if (0 == s_numApps)
		{
			result = ::_main_(_argc, (char**)_argv);
		}
		else
		{
			result = runApp(getCurrentApp(selected), _argc, _argv);
		}

		if (0 != bx::strLen(s_restartArgs))
		{
			find = s_restartArgs;
			goto restart;
		}

		setCurrentDir("");

		BX_DELETE(g_allocator, s_fileReader);
		s_fileReader = NULL;

		BX_DELETE(g_allocator, s_fileWriter);
		s_fileWriter = NULL;

		return result;
	}

	bx::FileReaderI* getFileReader()
	{
		return s_fileReader;
	}

	bx::FileWriterI* getFileWriter()
	{
		return s_fileWriter;
	}

	bx::AllocatorI* getAllocator()
	{
		if (NULL == g_allocator)
		{
			g_allocator = getDefaultAllocator();
		}

		return g_allocator;
	}
}