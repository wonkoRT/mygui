/*!
	@file
	@author		Albert Semenov
	@date		11/2009
*/

#ifndef POINTER_MANAGER_H_
#define POINTER_MANAGER_H_

#include <string>
#include <SDL.h>

namespace input
{

	class PointerManager
	{
	public:
		PointerManager();
		virtual ~PointerManager();

		void createPointerManager(SDL_Window* sdlWindow);
		void destroyPointerManager();

		void setPointerVisible(bool _value);
		void setPointerName(const std::string& _name);

		void loadPointerResources();

	private:
		void notifyChangeMousePointer(const std::string& _name);
		void updateSDLPointer(SDL_SystemCursor _newCursor);
		bool isMouseInClient();
		void setPointer(const std::string& _name);

	private:
		typedef std::map<std::string, SDL_SystemCursor> MapPointer;
		MapPointer mMapPointer;
		bool mManagerPointer;
		SDL_Cursor* mCursor;


#ifdef MYGUI_OGRE_21
	private:
		SDL_Window* mSdlWindow;
		bool        mMouseInWindow;
		bool        mWindowHasFocus;
	private:
		void handleWindowEvent(const SDL_Event& evt);
		void updateMouseSettings();
	protected:
		void handleSdlPointerEvents(const SDL_Event& evt);
#endif
	};

} // namespace input

#endif // POINTER_MANAGER_H_
