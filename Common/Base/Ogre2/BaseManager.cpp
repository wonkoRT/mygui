/*!
	@file
	@author		Albert Semenov
	@date		08/2008
*/

#include "Precompiled.h"
#include "BaseManager.h"

#include <stdexcept>

#include <MyGUI_Ogre2Platform.h>

#include <MyGUI_Ogre2RenderManager.h>

#include <Compositor/OgreCompositorManager2.h>
#include <Compositor/OgreCompositorNodeDef.h>
#include <Compositor/Pass/OgreCompositorPass.h>
#include <Compositor/Pass/OgreCompositorPassDef.h>
#include <Compositor/Pass/PassScene/OgreCompositorPassSceneDef.h>
#include <Compositor/Pass/PassClear/OgreCompositorPassClearDef.h>
#include <Compositor/OgreCompositorWorkspaceDef.h>
#ifndef MYGUI_OGRE_21
#include <Hlms/Unlit/OgreHlmsUnlit.h>
#include <Hlms/Pbs/OgreHlmsPbs.h>
#else
#include <Hlms/Unlit/include/OgreHlmsUnlit.h>
#include <Hlms/Pbs/include/OgreHlmsPbs.h>

#include <SDL_syswm.h>
#endif
#include <OgreHlmsManager.h>
#include <OgreFrameStats.h>

#if MYGUI_PLATFORM == MYGUI_PLATFORM_WIN32
#	include <windows.h>
#elif MYGUI_PLATFORM == MYGUI_PLATFORM_LINUX
#	include <X11/Xlib.h>
#	include <X11/Xutil.h>
#	include <X11/Xatom.h>
#endif

namespace base
{

#if MYGUI_PLATFORM == MYGUI_PLATFORM_APPLE
#include <CoreFoundation/CoreFoundation.h>
	// This function will locate the path to our application on OS X,
	// unlike windows you can not rely on the curent working directory
	// for locating your configuration files and resources.
	std::string macBundlePath()
	{
		char path[1024];
		CFBundleRef mainBundle = CFBundleGetMainBundle();
		assert(mainBundle);
		CFURLRef mainBundleURL = CFBundleCopyBundleURL(mainBundle);
		assert(mainBundleURL);
		CFStringRef cfStringRef = CFURLCopyFileSystemPath( mainBundleURL, kCFURLPOSIXPathStyle);
		assert(cfStringRef);
		CFStringGetCString(cfStringRef, path, 1024, kCFStringEncodingASCII);
		CFRelease(mainBundleURL);
		CFRelease(cfStringRef);
		return std::string(path);
	}
#endif

	BaseManager::BaseManager() :
		mGUI(nullptr),
		mPlatform(nullptr),
		mRoot(nullptr),
		mCamera(nullptr),
		mSceneManager(nullptr),
		mWindow(nullptr),
		mExit(false),
		mPluginCfgName("plugins.cfg"),
		mResourceXMLName("resources.xml"),
		mResourceFileName("MyGUI_Core.xml")
#ifdef MYGUI_OGRE_21
		, mSdlWindow(0),
		w_(0),
		h_(0)
#endif
	{
		#if MYGUI_PLATFORM == MYGUI_PLATFORM_APPLE
			mResourcePath = macBundlePath() + "/Contents/Resources/";
		#else
			mResourcePath = "";
		#endif
	}

	BaseManager::~BaseManager()
	{
	}

	bool BaseManager::create(int _width, int _height)
	{
		Ogre::String pluginsPath;

		#ifndef OGRE_STATIC_LIB
			pluginsPath = mResourcePath + mPluginCfgName;
		#endif

		mRoot = new Ogre::Root(pluginsPath, mResourcePath + "ogre.cfg", mResourcePath + "Ogre.log");

		setupResources();

		// попробуем завестись на дефолтных
		if (!mRoot->restoreConfig())
		{
			// ничего не получилось, покажем диалог
			if (!mRoot->showConfigDialog()) return false;
		}

#ifndef MYGUI_OGRE_21
		mWindow = mRoot->initialise(true);


		// вытаскиваем дискриптор окна
		size_t handle = getWindowHandle();

	#if MYGUI_PLATFORM == MYGUI_PLATFORM_WIN32
		char buf[MAX_PATH];
		::GetModuleFileNameA(0, (LPCH)&buf, MAX_PATH);
		HINSTANCE instance = ::GetModuleHandleA(buf);
		HICON hIconSmall = static_cast<HICON>(LoadImage(instance, MAKEINTRESOURCE(1001), IMAGE_ICON, 32, 32, LR_DEFAULTSIZE));
		HICON hIconBig = static_cast<HICON>(LoadImage(instance, MAKEINTRESOURCE(1001), IMAGE_ICON, 256, 256, LR_DEFAULTSIZE));
		if (hIconSmall)
			::SendMessageA((HWND)handle, WM_SETICON, 0, (LPARAM)hIconSmall);
		if (hIconBig)
			::SendMessageA((HWND)handle, WM_SETICON, 1, (LPARAM)hIconBig);
	#endif
#else
		mRoot->initialise(false);

		std::string windowTitle = "asdfasdf";

		Ogre::ConfigOptionMap& cfgOpts = mRoot->getRenderSystem()->getConfigOptions();

		int width = 1280;
		int height = 720;

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
		{
			Ogre::Vector2 screenRes = iOSUtils::getScreenResolutionInPoints();
			width = static_cast<int>(screenRes.x);
			height = static_cast<int>(screenRes.y);
		}
#endif

		Ogre::ConfigOptionMap::iterator opt = cfgOpts.find("Video Mode");
		if (opt != cfgOpts.end())
		{
			//Ignore leading space
			const Ogre::String::size_type start = opt->second.currentValue.find_first_of("012356789");
			//Get the width and height
			Ogre::String::size_type widthEnd = opt->second.currentValue.find(' ', start);
			// we know that the height starts 3 characters after the width and goes until the next space
			Ogre::String::size_type heightEnd = opt->second.currentValue.find(' ', widthEnd + 3);
			// Now we can parse out the values
			w_ = Ogre::StringConverter::parseInt(opt->second.currentValue.substr(0, widthEnd));
			h_ = Ogre::StringConverter::parseInt(opt->second.currentValue.substr(
				widthEnd + 3, heightEnd));
		}

		Ogre::NameValuePairList params;
		bool fullscreen = Ogre::StringConverter::parseBool(cfgOpts["Full Screen"].currentValue);
//#if OGRE_USE_SDL2
		int screen = 0;
		int posX = SDL_WINDOWPOS_CENTERED_DISPLAY(screen);
		int posY = SDL_WINDOWPOS_CENTERED_DISPLAY(screen);

		if (fullscreen)
		{
			posX = SDL_WINDOWPOS_UNDEFINED_DISPLAY(screen);
			posY = SDL_WINDOWPOS_UNDEFINED_DISPLAY(screen);
		}


		mSdlWindow = SDL_CreateWindow(
			windowTitle.c_str(),    // window title
			posX,               // initial x position
			posY,               // initial y position
			w_,              // width, in pixels
			h_,             // height, in pixels
			SDL_WINDOW_SHOWN
			| (fullscreen ? SDL_WINDOW_FULLSCREEN : 0) | SDL_WINDOW_RESIZABLE);

		//Get the native whnd
		SDL_SysWMinfo wmInfo;
		SDL_VERSION(&wmInfo.version);

		if (SDL_GetWindowWMInfo(mSdlWindow, &wmInfo) == SDL_FALSE)
		{
			OGRE_EXCEPT(Ogre::Exception::ERR_INTERNAL_ERROR,
				"Couldn't get WM Info! (SDL2)",
				"GraphicsSystem::initialize");
		}

		Ogre::String winHandle;
		switch (wmInfo.subsystem)
		{
#ifdef WIN32
		case SDL_SYSWM_WINDOWS:
			// Windows code
			winHandle = Ogre::StringConverter::toString((uintptr_t)wmInfo.info.win.window);
			break;
#elif __MACOSX__
		case SDL_SYSWM_COCOA:
			//required to make OGRE play nice with our window
			params.insert(std::make_pair("macAPI", "cocoa"));
			params.insert(std::make_pair("macAPICocoaUseNSView", "true"));

			winHandle = Ogre::StringConverter::toString(WindowContentViewHandle(wmInfo));
			break;
#else
		case SDL_SYSWM_X11:
			winHandle = Ogre::StringConverter::toString((uintptr_t)wmInfo.info.x11.window);
			break;
#endif
		default:
			OGRE_EXCEPT(Ogre::Exception::ERR_NOT_IMPLEMENTED,
				"Unexpected WM! (SDL2)",
				"GraphicsSystem::initialize");
			break;
		}

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		params.insert(std::make_pair("externalWindowHandle", winHandle));
#else
		params.insert(std::make_pair("parentWindowHandle", winHandle));
#endif
//#endif

		params.insert(std::make_pair("title", windowTitle));
		params.insert(std::make_pair("gamma", "true"));
		params.insert(std::make_pair("FSAA", cfgOpts["FSAA"].currentValue));
		params.insert(std::make_pair("vsync", cfgOpts["VSync"].currentValue));

		mWindow = Ogre::Root::getSingleton().createRenderWindow(windowTitle, w_, h_,
			fullscreen, &params);
#endif


		const size_t numThreads = std::max<int>(1, Ogre::PlatformInformation::getNumLogicalCores());
		Ogre::InstancingThreadedCullingMethod threadedCullingMethod = Ogre::INSTANCING_CULLING_SINGLETHREAD;
		if(numThreads > 1)
			threadedCullingMethod = Ogre::INSTANCING_CULLING_THREADED;
		mSceneManager = Ogre::Root::getSingleton().createSceneManager(Ogre::ST_GENERIC, numThreads, threadedCullingMethod);

		mCamera = mSceneManager->createCamera("BaseCamera");
		mCamera->setNearClipDistance(5);
		mCamera->setPosition(400, 400, 400);
		mCamera->lookAt(0, 150, 0);

		// Set default mipmap level (NB some APIs ignore this)
		Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(5);

		mSceneManager->setAmbientLight(Ogre::ColourValue::White, Ogre::ColourValue::White, Ogre::Vector3::UNIT_Y);
		Ogre::SceneNode* lightNode = mSceneManager->getRootSceneNode()->createChildSceneNode();
		Ogre::Light* light = mSceneManager->createLight();
		lightNode->attachObject(light);
		light->setType(Ogre::Light::LT_DIRECTIONAL);
		Ogre::Vector3 vec(-0.3f, -0.3f, -0.3f);
		vec.normalise();
		light->setDirection(vec);

		// Load resources
		Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();

		mRoot->addFrameListener(this);
		Ogre::WindowEventUtilities::addWindowEventListener(mWindow, this);

		registerHlms();

		createGui();

#ifndef MYGUI_OGRE_21
		createInput(handle);

		createPointerManager(handle);
#else
		createInput(mSdlWindow);

		createPointerManager(mSdlWindow);

		MyGUI::Gui::getInstance().eventFrameStart += MyGUI::newDelegate(this, &BaseManager::frameEvent);
#endif

		createScene();

#ifndef MYGUI_OGRE_21
		windowResized(mWindow);
#else
		windowResized(w_, h_);
#endif

		return true;
	}

	void BaseManager::run()
	{
		// инициализируем все рендер таргеты
		mRoot->getRenderSystem()->_initRenderTargets();

		// крутимся бесконечно
		while (true)
		{
			Ogre::WindowEventUtilities::messagePump();

			if (mWindow->isActive() == false)
			{
				mWindow->setActive(true);
#if MYGUI_PLATFORM == MYGUI_PLATFORM_WIN32
				::Sleep(50);
#endif
			}
			if (!mRoot->renderOneFrame())
				break;

// выставляем слип, чтобы другие потоки не стопорились
#if MYGUI_PLATFORM == MYGUI_PLATFORM_WIN32
			::Sleep(0);
#endif

		};
	}

	void BaseManager::destroy()
	{
		destroyScene();

		destroyPointerManager();

		destroyInput();

		destroyGui();

		// очищаем сцену
		if (mSceneManager)
		{
			mSceneManager->clearScene();
			mSceneManager->destroyAllCameras();
			mSceneManager = nullptr;
			mCamera = nullptr;
		}

		if (mWindow)
		{
			mWindow->destroy();
			mWindow = nullptr;
		}

		if (mRoot)
		{
			Ogre::RenderWindow* window = mRoot->getAutoCreatedWindow();
			if (window)
				window->removeAllViewports();
			delete mRoot;
			mRoot = nullptr;
		}
	}

	void BaseManager::createGui()
	{
		mPlatform = new MyGUI::Ogre2Platform();
		mPlatform->initialise(mWindow, mSceneManager, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);

		//setup workspace
		//NB this can also been done via a script using the following syntax
		/*
		pass custom MYGUI
		{
		}
		*/
		const Ogre::String workspaceName = "scene workspace";
		const Ogre::IdString workspaceNameHash = workspaceName;

		Ogre::CompositorManager2* pCompositorManager = Ogre::Root::getSingleton().getCompositorManager2();
		Ogre::CompositorNodeDef *nodeDef = pCompositorManager->addNodeDefinition("myworkspace");
		//Input texture
		nodeDef->addTextureSourceName("WindowRT", 0, Ogre::TextureDefinitionBase::TEXTURE_INPUT);
		nodeDef->setNumTargetPass(1);
		{
			Ogre::CompositorTargetDef *targetDef = nodeDef->addTargetPass("WindowRT");
			targetDef->setNumPasses(3);
			{
				{
					Ogre::CompositorPassClearDef* passClear = static_cast<Ogre::CompositorPassClearDef*>
					(targetDef->addPass(Ogre::PASS_CLEAR));
					Ogre::CompositorPassSceneDef *passScene = static_cast<Ogre::CompositorPassSceneDef*>
						(targetDef->addPass(Ogre::PASS_SCENE));
					passScene->mShadowNode = Ogre::IdString();

					// For the MyGUI pass
					targetDef->addPass(Ogre::PASS_CUSTOM, MyGUI::OgreCompositorPassProvider::mPassId);
				}
			}
		}
		Ogre::CompositorWorkspaceDef *workDef = pCompositorManager->addWorkspaceDefinition(workspaceName);
#ifndef MYGUI_OGRE_21
		workDef->connectOutput(nodeDef->getName(), 0);
#else
		workDef->connectExternal(0, nodeDef->getName(), 0);
#endif

		pCompositorManager->addWorkspace(mSceneManager, mWindow, mCamera, workspaceNameHash, true);

		mGUI = new MyGUI::Gui();
		mGUI->initialise(mResourceFileName);
	}

	void BaseManager::destroyGui()
	{
		if (mGUI)
		{
			mGUI->shutdown();
			delete mGUI;
			mGUI = nullptr;
		}

		if (mPlatform)
		{
			mPlatform->shutdown();
			delete mPlatform;
			mPlatform = nullptr;
		}
	}

	void BaseManager::setWindowMaximized(bool _value)
	{
#ifndef MYGUI_OGRE_21
#if MYGUI_PLATFORM == MYGUI_PLATFORM_WIN32
		if (_value)
		{
			size_t handle = getWindowHandle();
			::ShowWindow((HWND)handle, SW_SHOWMAXIMIZED);
		}
#endif
#else
		SDL_MaximizeWindow(mSdlWindow);
#endif
	}

	bool BaseManager::getWindowMaximized()
	{
#ifndef MYGUI_OGRE_21
		bool result = false;
#if MYGUI_PLATFORM == MYGUI_PLATFORM_WIN32
		size_t handle = getWindowHandle();
		result = ::IsZoomed((HWND)handle) != 0;
#endif
		return result;
#else
		return (SDL_GetWindowFlags(mSdlWindow) & SDL_WINDOW_MAXIMIZED) != 0;
#endif
	}

	void BaseManager::setWindowCoord(const MyGUI::IntCoord& _value)
	{
#if MYGUI_PLATFORM == MYGUI_PLATFORM_WIN32
		if (_value.empty())
			return;

		MyGUI::IntCoord coord = _value;
		if (coord.left < 0)
			coord.left = 0;
		if (coord.top < 0)
			coord.top = 0;
		if (coord.width < 640)
			coord.width = 640;
		if (coord.height < 480)
			coord.height = 480;
		if (coord.width > GetSystemMetrics(SM_CXSCREEN))
			coord.width = GetSystemMetrics(SM_CXSCREEN);
		if (coord.height > GetSystemMetrics(SM_CYSCREEN))
			coord.height = GetSystemMetrics(SM_CYSCREEN);
		if (coord.right() > GetSystemMetrics(SM_CXSCREEN))
			coord.left = GetSystemMetrics(SM_CXSCREEN) - coord.width;
		if (coord.bottom() > GetSystemMetrics(SM_CYSCREEN))
			coord.top = GetSystemMetrics(SM_CYSCREEN) - coord.height;

#ifndef MYGUI_OGRE_21
		size_t handle = getWindowHandle();
		::MoveWindow((HWND)handle, coord.left, coord.top, coord.width, coord.height, true);
#else
		SDL_SetWindowPosition(mSdlWindow, coord.left, coord.top);
		SDL_SetWindowSize(mSdlWindow, coord.width, coord.height);
#endif
#endif
	}

	MyGUI::IntCoord BaseManager::getWindowCoord()
	{
		MyGUI::IntCoord result;
#if MYGUI_PLATFORM == MYGUI_PLATFORM_WIN32
#ifndef MYGUI_OGRE_21
		size_t handle = getWindowHandle();
		::RECT rect;
		::GetWindowRect((HWND)handle, &rect);
		result.left = rect.left;
		result.top = rect.top;
		result.width = rect.right - rect.left;
		result.height = rect.bottom - rect.top;
#else
		int x, y, w, h;
		SDL_GetWindowPosition(mSdlWindow, &x, &y);
		SDL_GetWindowSize(mSdlWindow, &w, &h);
		result.left = x;
		result.top = y;
		result.width = w;
		result.height = h;
#endif
#endif
		return result;
	}

	void BaseManager::setupResources()
	{
		MyGUI::xml::Document doc;

		if (!doc.open(mResourceXMLName))
			doc.getLastError();

		MyGUI::xml::ElementPtr root = doc.getRoot();
		if (root == nullptr || root->getName() != "Paths")
			return;

		MyGUI::xml::ElementEnumerator node = root->getElementEnumerator();
		while (node.next())
		{
			if (node->getName() == "Path")
			{
				if (node->findAttribute("root") != "")
				{
					bool rootAttr = MyGUI::utility::parseBool(node->findAttribute("root"));
					if (rootAttr)
						mRootMedia = node->getContent();
				}
				addResourceLocation(node->getContent());
			}
		}

		addResourceLocation(getRootMedia() + "/Common/Base");
	}

	void BaseManager::registerHlms()
	{
		Ogre::String shaderSyntax = "/GLSL";
		if (mRoot->getRenderSystem()->getName() == "Direct3D11 Rendering Subsystem")
			shaderSyntax = "/HLSL";

		Ogre::Archive *archiveCommon = Ogre::ArchiveManager::getSingletonPtr()->load(
			getRootMedia() + "/OgreHlms" + "/Common" + shaderSyntax,
			"FileSystem", true);

		Ogre::ArchiveVec libraryCommon;
		libraryCommon.push_back(archiveCommon);

		Ogre::Archive *archiveUnlit = Ogre::ArchiveManager::getSingletonPtr()->load(
			getRootMedia() + "/OgreHlms" + "/Unlit" + shaderSyntax,
			"FileSystem", true);

		Ogre::HlmsUnlit *hlmsUnlit = OGRE_NEW Ogre::HlmsUnlit(archiveUnlit, &libraryCommon);
		Ogre::Root::getSingleton().getHlmsManager()->registerHlms(hlmsUnlit);
	}

#ifdef MYGUI_OGRE_21
	void BaseManager::frameEvent(float _time)
	{
		Ogre::WindowEventUtilities::messagePump();

		SDL_Event evt = { 0 };
		while (SDL_PollEvent(&evt))
		{
			handleSdlEvents(evt);
		}
	}
	void BaseManager::handleSdlEvents(const SDL_Event& evt)
	{
		// hack
		static SDL_Keycode lastSym;

		switch (evt.type)
		{
		case SDL_MOUSEMOTION:
		case SDL_MOUSEWHEEL:
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		case SDL_KEYDOWN:
		case SDL_KEYUP:
		case SDL_TEXTINPUT:
		case SDL_JOYAXISMOTION:
		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
		case SDL_JOYDEVICEADDED:
		case SDL_JOYDEVICEREMOVED:
			handleSdlInputEvents(evt);
			break;
		case SDL_WINDOWEVENT:
			handleSdlInputEvents(evt);
			handleSdlPointerEvents(evt);
			handleWindowEvent(evt);
			break;
		}
	}
	void BaseManager::handleWindowEvent(const SDL_Event& evt)
	{
		switch (evt.window.event)
		{
		case SDL_WINDOWEVENT_SIZE_CHANGED:
			int w, h;
			SDL_GetWindowSize(mSdlWindow, &w, &h);
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
			mWindow->resize(w, h);
#else
			windowResized(w, h);
			mWindow->windowMovedOrResized();
#endif
			break;
		case SDL_WINDOWEVENT_RESIZED:
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
			mWindow->resize(evt.window.data1, evt.window.data2);
#else
			windowResized(evt.window.data1, evt.window.data2);
			mWindow->windowMovedOrResized();
#endif
			break;
		case SDL_WINDOWEVENT_CLOSE:
			windowClosed(mWindow);
			break;
		case SDL_WINDOWEVENT_SHOWN:
			mWindow->setVisible(true);
			break;
		case SDL_WINDOWEVENT_HIDDEN:
			mWindow->setVisible(false);
			break;
		}
	}
#endif

	bool BaseManager::frameStarted(const Ogre::FrameEvent& evt)
	{
		if (mExit)
			return false;

		if (!mGUI)
			return true;

		captureInput();

		return true;
	}

	bool BaseManager::frameEnded(const Ogre::FrameEvent& evt)
	{
		return true;
	}

#ifndef MYGUI_OGRE_21
	void BaseManager::windowResized(Ogre::RenderWindow* _rw)
	{
		int width = (int)_rw->getWidth();
		int height = (int)_rw->getHeight();

		// при удалении окна может вызываться этот метод
		if (mCamera)
		{
			mCamera->setAspectRatio((float)width / (float)height);

			setInputViewSize(width, height);
		}
	}
#else
	void BaseManager::windowResized(int w, int h)
	{
		mPlatform->getRenderManagerPtr()->windowResized(w, h);
		if (mCamera)
		{
			mCamera->setAspectRatio((float)w / (float)h);
			setInputViewSize(w, h);
		}
	}
#endif

	void BaseManager::windowClosed(Ogre::RenderWindow* _rw)
	{
		mExit = true;
		destroyInput();
	}

#ifndef MYGUI_OGRE_21
	size_t BaseManager::getWindowHandle()
	{
		size_t handle = 0;
		mWindow->getCustomAttribute("WINDOW", &handle);
		return handle;
	}
#endif

	void BaseManager::setWindowCaption(const std::wstring& _text)
	{
	#if MYGUI_PLATFORM == MYGUI_PLATFORM_WIN32
#ifndef MYGUI_OGRE_21
		::SetWindowTextW((HWND)getWindowHandle(), _text.c_str());
#else
		// 2do
		SDL_SetWindowTitle(mSdlWindow, MyGUI::UString(_text).asUTF8_c_str());
#endif
	#elif MYGUI_PLATFORM == MYGUI_PLATFORM_LINUX
		Display* xDisplay = nullptr;
		unsigned long windowHandle = 0;
		mWindow->getCustomAttribute("XDISPLAY", &xDisplay);
		mWindow->getCustomAttribute("WINDOW", &windowHandle);
		Window win = (Window)windowHandle;

		XTextProperty windowName;
		windowName.value    = (unsigned char *)(MyGUI::UString(_text).asUTF8_c_str());
		windowName.encoding = XA_STRING;
		windowName.format   = 8;
		windowName.nitems   = strlen((char *)(windowName.value));
		XSetWMName(xDisplay, win, &windowName);
	#endif
	}

	void BaseManager::prepare()
	{
	}

	void BaseManager::addResourceLocation(const std::string& _name, const std::string& _group, const std::string& _type, bool _recursive)
	{
		#if MYGUI_PLATFORM == MYGUI_PLATFORM_APPLE
			// OS X does not set the working directory relative to the app, In order to make things portable on OS X we need to provide the loading with it's own bundle path location
			Ogre::ResourceGroupManager::getSingleton().addResourceLocation(Ogre::String(macBundlePath() + "/" + _name), _type, _group, _recursive);
		#else
			Ogre::ResourceGroupManager::getSingleton().addResourceLocation(_name, _type, _group, _recursive);
		#endif
	}

	void BaseManager::addResourceLocation(const std::string& _name, bool _recursive)
	{
		addResourceLocation(_name, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, "FileSystem", false);
	}

	void BaseManager::injectMouseMove(int _absx, int _absy, int _absz)
	{
		if (!mGUI)
			return;

		MyGUI::InputManager::getInstance().injectMouseMove(_absx, _absy, _absz);
	}

	void BaseManager::injectMousePress(int _absx, int _absy, MyGUI::MouseButton _id)
	{
		if (!mGUI)
			return;

		MyGUI::InputManager::getInstance().injectMousePress(_absx, _absy, _id);
	}

	void BaseManager::injectMouseRelease(int _absx, int _absy, MyGUI::MouseButton _id)
	{
		if (!mGUI)
			return;

		MyGUI::InputManager::getInstance().injectMouseRelease(_absx, _absy, _id);
	}

	void BaseManager::injectKeyPress(MyGUI::KeyCode _key, MyGUI::Char _text)
	{
		if (!mGUI)
			return;

		if (_key == MyGUI::KeyCode::Escape)
		{
			mExit = true;
			return;
		}
		else if (_key == MyGUI::KeyCode::SysRq)
		{
			makeScreenShot();
			return;
		}

		// change polygon mode
		// TODO: polygon mode require changes in platform
		//else if (_key == MyGUI::KeyCode::F5)
		//{
		//	getCamera()->setPolygonMode(Ogre::PM_SOLID);
		//}
		//else if (_key == MyGUI::KeyCode::F6)
		//{
		//	getCamera()->setPolygonMode(Ogre::PM_WIREFRAME);
		//}
		//else if (_key == MyGUI::KeyCode::F7)
		//{
		//	getCamera()->setPolygonMode(Ogre::PM_POINTS);
		//}
#if OGRE_VERSION >= MYGUI_DEFINE_VERSION(1, 7, 0) && OGRE_NO_VIEWPORT_ORIENTATIONMODE == 0
		else if (_key == MyGUI::KeyCode::F1)
		{
			mWindow->getViewport(0)->setOrientationMode(Ogre::OR_DEGREE_0, false);
			mPlatform->getRenderManagerPtr()->setRenderWindow(mWindow);
		}
		else if (_key == MyGUI::KeyCode::F2)
		{
			mWindow->getViewport(0)->setOrientationMode(Ogre::OR_DEGREE_90, false);
			mPlatform->getRenderManagerPtr()->setRenderWindow(mWindow);
		}
		else if (_key == MyGUI::KeyCode::F3)
		{
			mWindow->getViewport(0)->setOrientationMode(Ogre::OR_DEGREE_180, false);
			mPlatform->getRenderManagerPtr()->setRenderWindow(mWindow);
		}
		else if (_key == MyGUI::KeyCode::F4)
		{
			mWindow->getViewport(0)->setOrientationMode(Ogre::OR_DEGREE_270, false);
			mPlatform->getRenderManagerPtr()->setRenderWindow(mWindow);
		}
#endif

		MyGUI::InputManager::getInstance().injectKeyPress(_key, _text);
	}

	void BaseManager::injectKeyRelease(MyGUI::KeyCode _key)
	{
		if (!mGUI)
			return;

		MyGUI::InputManager::getInstance().injectKeyRelease(_key);
	}

	void BaseManager::quit()
	{
		mExit = true;
	}

	const std::string& BaseManager::getRootMedia()
	{
		return mRootMedia;
	}

	void BaseManager::setResourceFilename(const std::string& _flename)
	{
		mResourceFileName = _flename;
	}

	Ogre::SceneManager* BaseManager::getSceneManager()
	{
		return mSceneManager;
	}

	Ogre::Camera* BaseManager::getCamera()
	{
		return mCamera;
	}

	void BaseManager::makeScreenShot()
	{
		std::ifstream stream;
		std::string file;
		do
		{
			stream.close();
			static size_t num = 0;
			const size_t max_shot = 100;
			if (num == max_shot)
			{
				MYGUI_LOG(Info, "The limit of screenshots is exceeded : " << max_shot);
				return;
			}
			file = MyGUI::utility::toString("screenshot_", ++num, ".png");
			stream.open(file.c_str());
		}
		while (stream.is_open());
		mWindow->writeContentsToFile(file);
	}

	MyGUI::MapString BaseManager::getStatistic()
	{
		MyGUI::MapString result;

		try
		{
			const Ogre::FrameStats* stats = mRoot->getFrameStats();
			result["FPS"] = MyGUI::utility::toString(stats->getFps());
			const Ogre::RenderTarget::FrameStats rtStats = mWindow->getStatistics();
			result["triangle"] = MyGUI::utility::toString(rtStats.triangleCount);
			result["batch"] = MyGUI::utility::toString(rtStats.batchCount);
			result["batch gui"] = MyGUI::utility::toString(MyGUI::Ogre2RenderManager::getInstance().getBatchCount());
		}
		catch (...)
		{
			MYGUI_LOG(Warning, "Error get statistics");
		}

		return result;
	}

} // namespace base
