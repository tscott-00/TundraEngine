#include "window.h"

#include "events_state.h"

#define sdl_error(func) if (func < 0) { printf("SDL Error: %s\n", SDL_GetError()); }
#define sdl_nullptr_error(func) if (func == nullptr) { printf("SDL Error: %s\n", SDL_GetError()); }

Window::Window(const std::string& title)
{
	sdl_error(SDL_Init(SDL_INIT_EVERYTHING));

	sdl_error(SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8));
	sdl_error(SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8));
	sdl_error(SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8));
	sdl_error(SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8));
	sdl_error(SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32));
	sdl_error(SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1));
	sdl_error(SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE));
	sdl_error(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4));
	sdl_error(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3));
#if defined _DEBUG
	sdl_error(SDL_GL_SetAttribute(SDL_GLattr::SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG));
#endif

	window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, EventsState::windowWidth, EventsState::windowHeight, SDL_WINDOW_OPENGL);
	sdl_nullptr_error(window)

	/*int iconWidth, iconHeight, iconNumComponents;
	std::string iconFileName = "./res/icon_small.png";
	unsigned char* imageData = stbi_load(iconFileName.c_str(), &iconWidth, &iconHeight, &iconNumComponents, 4);
	SDL_Surface* icon;
	icon = SDL_CreateRGBSurfaceFrom(imageData, iconWidth, iconHeight, 32, iconWidth * 4, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
	SDL_SetWindowIcon(window, icon);
	stbi_image_free(imageData);
	SDL_FreeSurface(icon);*/

	glContext = SDL_GL_CreateContext(window);
	sdl_nullptr_error(glContext)
	GLenum status = glewInit();
	if (status != GLEW_OK)
		std::cerr << "Glew failed to initialize!" << std::endl;

#if defined _DEBUG
	glEnable(GL_DEBUG_OUTPUT);
	// Enable everything but notifications
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW | GL_DEBUG_SEVERITY_MEDIUM | GL_DEBUG_SEVERITY_HIGH, 0, nullptr, GL_TRUE);
	glDebugMessageCallback(MessageCallback, 0);
#endif
	//if (useVsync)
	sdl_error(SDL_GL_SetSwapInterval(1));
	//else
	//	SDL_GL_SetSwapInterval(0);

	// Configure opengl somewhere
	// Update and Render ( Just uses active FB ( Render for render engine which is seperate object, game objects with renderable setup will get added to renderer ) )
}

Window::~Window()
{
	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

void Window::Update()
{
	SDL_GL_SwapWindow(window);
}