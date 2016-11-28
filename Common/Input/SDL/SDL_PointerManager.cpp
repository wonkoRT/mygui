/*!
	@file
	@author		Albert Semenov
	@date		1/2009
*/

#include "Precompiled.h"
#include "SDL_PointerManager.h"
#include <MyGUI.h>

#include "ResourceSDLPointer.cpp"

#ifdef MYGUI_OGRE_21
#include "OgreWindowEventUtilities.h"
#endif

namespace input
{

	PointerManager::PointerManager() :
		mManagerPointer(true),
		mCursor(nullptr)
#ifdef MYGUI_OGRE_21
		, mSdlWindow(0)
		, mMouseInWindow(true)
		, mWindowHasFocus(true)
#endif
	{
	}

	PointerManager::~PointerManager()
	{
		SDL_FreeCursor(mCursor);
	}

	void PointerManager::createPointerManager(SDL_Window* sdlWindow)
	{
		mSdlWindow = sdlWindow;

		MyGUI::PointerManager& manager = MyGUI::PointerManager::getInstance();
		manager.setVisible(false);
		manager.eventChangeMousePointer += MyGUI::newDelegate(this, &PointerManager::notifyChangeMousePointer);

		std::string resourceCategory = MyGUI::ResourceManager::getInstance().getCategoryName();
		MyGUI::FactoryManager::getInstance().registerFactory<ResourceSDLPointer>(resourceCategory);
	}

	void PointerManager::destroyPointerManager()
	{
		std::string resourceCategory = MyGUI::ResourceManager::getInstance().getCategoryName();
		MyGUI::FactoryManager::getInstance().unregisterFactory<ResourceSDLPointer>(resourceCategory);

		MyGUI::PointerManager& manager = MyGUI::PointerManager::getInstance();
		manager.eventChangeMousePointer -= MyGUI::newDelegate(this, &PointerManager::notifyChangeMousePointer);
	}

	void PointerManager::setPointerVisible(bool _value)
	{
		SDL_ShowCursor(static_cast<int>(_value));
	}

	void PointerManager::notifyChangeMousePointer(const std::string& _name)
	{
		if (mManagerPointer)
		{
			setPointer(_name);
		}
	}

	void PointerManager::setPointerName(const std::string& _name)
	{
		mManagerPointer = false;
		setPointer(_name);
	}

	void PointerManager::updateSDLPointer(SDL_SystemCursor _newCursor)
	{
		SDL_FreeCursor(mCursor);
		mCursor = SDL_CreateSystemCursor(_newCursor);
		SDL_SetCursor(mCursor);
	}

	void PointerManager::setPointer(const std::string& _name)
	{
		MapPointer::iterator iter = mMapPointer.find(_name);
		if (iter != mMapPointer.end())
		{
			updateSDLPointer(iter->second);
		}
		else
		{
			MyGUI::IResource* resource_generic = MyGUI::ResourceManager::getInstance().getByName(_name, false);
			if (resource_generic != nullptr)
			{
				ResourceSDLPointer* resource = resource_generic->castType<ResourceSDLPointer>(false);
				if (resource != nullptr)
				{
					mMapPointer[_name] = resource->getPointerType();
					updateSDLPointer(resource->getPointerType());
				}
			}
		}
	}

	void PointerManager::loadPointerResources()
	{
		MyGUI::ResourceManager::getInstance().load("PointersSDL.xml");
	}

#ifdef MYGUI_OGRE_21
	void PointerManager::handleSdlPointerEvents(const SDL_Event& evt)
	{
		switch (evt.type)
		{
		case SDL_WINDOWEVENT:
			handleWindowEvent(evt);
			break;
		}
	}
	void PointerManager::handleWindowEvent(const SDL_Event& evt)
	{
		switch (evt.window.event)
		{
		case SDL_WINDOWEVENT_ENTER:
			mMouseInWindow = true;
			updateMouseSettings();
			break;
		case SDL_WINDOWEVENT_LEAVE:
			mMouseInWindow = false;
			updateMouseSettings();
			break;
		case SDL_WINDOWEVENT_FOCUS_GAINED:
			mWindowHasFocus = true;
			updateMouseSettings();
			break;
		case SDL_WINDOWEVENT_FOCUS_LOST:
			mWindowHasFocus = false;
			updateMouseSettings();
			break;
		}
	}
	void PointerManager::updateMouseSettings()
	{
		// 2do
		bool mGrabPointer = false && mMouseInWindow && mWindowHasFocus;
		SDL_SetWindowGrab(mSdlWindow, mGrabPointer ? SDL_TRUE : SDL_FALSE);
	}
#endif

} // namespace input
