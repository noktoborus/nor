/* vim: ft=c ff=unix fenc=utf-8
 * file: main.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include <X11/Xlib.h>

/* error check */
#define ec___(CALL, EXPR) \
	fprintf (stderr, "FAIL CALL: [" __FILE__ ":%u] %s `%s' <- %s\n", __LINE__, #CALL, #EXPR, __func__)

#define ec_ec(CALL, EXPR) \
	{\
		if (!(EXPR))\
		{\
			ec___ (CALL, EXPR);\
		}\
	}

#define ec_if(CALL, EXPR) \
	if (!(EXPR))\
	{\
		ec___ (CALL, EXPR);\
	}\
	else

struct kzSurfShape_t
{
	unsigned int wx;
	unsigned int wy;
};

struct kzSurfKey_t
{
	unsigned char device;
	unsigned int key;
};

struct kzSurfMotion_t
{
	unsigned char device;
	unsigned int x;
	unsigned int y;
};

/*
 * struct kzSurfKey_t *key;
 * struct kzSurfQueue_t q;
 *
 **** get:
 *	if (q.off.keyf < q.off.keyl || q.key_last > q.off.keyf)
 *	{
 *		key = &(q.key[(q.off.keyf ++) % kzQUEUE_SIZE]);
 *	}
 **** put:
 *	if ((q.off.keyl + 1) % kzQUEUE_SIZE < q.off.keyf ||
 *		(q.off.keyl + 1 > q.off.keyf && q.off.keyl + 1 < kzQUEUE_SIZE))
 *	{
 *		memcpy (&q.key[++ q.off.keyl], key, sizeof (struct kzSurfKey_t));
 *	}
 */
#define kzQUEUE_SIZE	1024
struct kzSurfQueueOffset_t
{
	size_t keyf; /* key first */
	size_t keyl; /* key last */
	size_t motf; /* motion first */
	size_t motl; /* motion last */
};

struct kzSurfQueue_t
{
	struct kzSurfQueueOffset_t off;
	struct kzSurfKey_t key[kzQUEUE_SIZE];
	struct kzSurfMotion_t mot[kzQUEUE_SIZE];
};

struct kzSurf_t
{
	struct kzSurfShape_t shape;
	void *ptr;
	uint32_t events;
	struct kzSurfKey_t key;
	struct kzSurfMotion_t mot;
	struct kzSurfQueue_t *queue;
};

#define kzEVENT_ZERO	0			/* none */
#define kzEVENT_RESHAPE	1			/* window want resize */
#define kzEVENT_REDRAW	(1 << 1)	/* window want redraw */
#define kzEVENT_SSHOW	(1 << 2)	/* window displayed */
#define kzEVENT_SHIDE	(1 << 3)	/* window removed from visible screen */
#define kzEVENT_KEY		(1 << 4)	/* key pressed (keyboard, mouse, etc) */
#define kzEVENT_MOTION	(1 << 5)	/* get motion (mouse, etc) */
#define kzEVENT_TICK	(1 << 6)	/* timer tick */

#define kzPASS_ZERO	0
#define kzPASS_SWAP	1
#define kzPASS_STOP	(1 << 1)
/* return ptr to new window in *xwin_trg */
static inline void
createXWindow (EGLNativeWindowType *xwin_trg, EGLNativeDisplayType *xdpy,
		VisualID *vid, unsigned int wx, unsigned int wy)
{
	int cc;
	XSetWindowAttributes xattr;
	XVisualInfo *visInfo;
	XVisualInfo visTemp;
	Window xroot;
	if (!xwin_trg)
		return;
	/* get root window */
	xroot = XRootWindow (*xdpy, DefaultScreen (*xdpy));
	/* get X visual info */
	memset (&visTemp, 0, sizeof (XVisualInfo));
	visTemp.visualid = *vid;
	visInfo = XGetVisualInfo (*xdpy, VisualIDMask, &visTemp, &cc);
	ec_if (XGetVisualInfo, visInfo)
	{
		/* update attributes for window */
		memset (&xattr, 0, sizeof (XSetWindowAttributes));
		xattr.colormap = XCreateColormap (*xdpy, xroot,
				visInfo->visual, AllocNone);
		xattr.event_mask = StructureNotifyMask | ExposureMask |
				KeyPressMask | KeyReleaseMask |
				ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
		/* create window */
		*xwin_trg = XCreateWindow (
				*xdpy,
				/* Display */
				xroot,
				/* Root win */
				0, 0,
				/* x, y */
				wx, wy,
				/* Width, Height */
				0,
				/* border width */
				visInfo->depth,
				/* depth */
				InputOutput,
				/* class */
				visInfo->visual,
				/* visual */
				CWBackPixel | CWBorderPixel | CWColormap | CWEventMask,
				/* mask */
				&xattr
				/* attributes */
				);
		ec_ec (XCreateWindow, *xwin_trg);
		/* set title */
		{
			XSizeHints shints;
			XClassHint chint;
			chint.res_name = "kz";
			chint.res_class = "Window";
			memset (&shints, 0, sizeof (XSizeHints));
			shints.width = wx;
			shints.height = wy;
			shints.flags = USSize | USPosition;
			XSetNormalHints (*xdpy, *xwin_trg, &shints);
			XSetStandardProperties (*xdpy, *xwin_trg, "2:3", NULL, None, NULL,
					0, &shints);
			XSetClassHint (*xdpy, *xwin_trg, &chint);
		}
		/* display window */
		XMapWindow (*xdpy, *xwin_trg);
		XRaiseWindow (*xdpy, *xwin_trg);
		/* remove garbage information */
		XFree (visInfo);
	}
}

/* return in *xdpy and *edpy allocated ptrs to X11 display and EGL */
static inline bool
initEGL (EGLNativeDisplayType *xdpy, EGLDisplay *edpy)
{
	/* open display */
	*xdpy = XOpenDisplay (NULL);
	ec_if (XOpenDisplay, *xdpy)
	{
		/* connect egl */
		*edpy = eglGetDisplay (*xdpy);
		ec_if (eglGetDisplay, *edpy)
		{
			/* initialize egl */
			ec_if (eglInitialize, eglInitialize (*edpy, NULL, NULL))
				return true;
		}
	}
	return false;
}

/* return in *esurf ptr to new surface */
static inline bool
initSurfy (EGLSurface *esurf, EGLDisplay *edpy, EGLConfig *ecfg,
	EGLContext *ectx, EGLint const *ectx_attr,
	EGLNativeWindowType *xwin, EGLNativeDisplayType *xdpy,
	unsigned int wx, unsigned int wy)
{
	EGLint cc;
	/*
	 * Биндим API рендера, биндится для каждой нити программы
	 * в отдельности, как и контекст, в каждой новой нити программы
	 * следует вызывать eglReleaseThread (), для освобождения ресурсов.
	 *
	 * Вызов eglBindAPI следует производить перед вызовами
	 * 	eglCreateContext, eglGetCurrentContext, eglGetCurrentDisplay,
	 * 	eglGetCurrentSurface, eglWaitClient, eglWaitNative,
	 * 	eglMakeCurrent (при ctx == EGL_NO_CONTEXT)
	 */
	ec_if (eglBindAPI, eglBindAPI (EGL_OPENGL_ES_API))
	{
		/* create context */
		*ectx = eglCreateContext (*edpy, *ecfg, EGL_NO_CONTEXT, ectx_attr);
		ec_if (eglCreateContext, *ectx != EGL_NO_CONTEXT)
		{
			/* create surface */
			eglGetConfigAttrib (*edpy, *ecfg, EGL_NATIVE_VISUAL_ID, &cc);
			createXWindow (xwin, xdpy, (VisualID*)&cc, wx, wy);
			ec_if (createXWindow, *xwin)
			{
				eglCreateWindowSurface (*edpy, *ecfg, *xwin, NULL);
				ec_if (eglCreateWindowSurface, *esurf != EGL_NO_SURFACE)
					return true;
			}
		}
	}
	return false;
}

static inline void
killSurfy (EGLNativeWindowType *xwin, EGLNativeDisplayType *xdpy)
{
	if (xwin && *xwin)
		ec_ec (XDestroyWindow, XDestroyWindow (*xdpy, *xwin) != Success);
	if (xdpy && *xdpy)
	{
		ec_ec (XFlush, XFlush (*xdpy) != Success);
		ec_ec (XFlush, XCloseDisplay (*xdpy) != Success);
	}
}

bool kzPickEvent (struct kzSurf_t *surf, uint32_t event)
{
	if (surf->events & event)
	{
		surf->events &= ~event;
		return true;
	}
	return false;
}

/* return screen update info:
 *	true	-> screen updated
 *	false	-> no changes on screen
 */
static inline uint8_t
eventPass (struct kzSurf_t *surf)
{
	uint8_t result = kzPASS_ZERO;
	if (kzPickEvent (surf, kzEVENT_RESHAPE))
	{
		fprintf (stderr, "RESHAPE\n");
	}
	if (kzPickEvent (surf, kzEVENT_REDRAW))
	{
		fprintf (stderr, "REDRAW\n");
		glClearColor ((float)(rand () % 255) / 255.f,
				(float)(rand () % 255) / 255.f,
				(float)(rand () % 255) / 255.f, 0.f);
		glClear (GL_COLOR_BUFFER_BIT);
		result |= kzPASS_SWAP;
	}
	if (kzPickEvent (surf, kzEVENT_SHIDE))
	{
		fprintf (stderr, "Hide Surface\n");
	}
	if (kzPickEvent (surf, kzEVENT_SSHOW))
	{
		fprintf (stderr, "Show Surface\n");
	}
	if (kzPickEvent (surf, kzEVENT_KEY))
	{
		fprintf (stderr, "getKey -> (dev=%u, %06u (0x%04x), %s)\n",
				surf->key.device, surf->key.key >> 1, surf->key.key >> 1,
				(surf->key.key & 1)?"Up":"Down");
		if (surf->key.key >> 1 == XK_Escape)
		{
			fprintf (stderr, "Escape ## \n");
			result |= kzPASS_STOP;
		}
		if (surf->key.key >> 1 == 0x20)
		{
			surf->shape.wx = 40;
			surf->shape.wy = 40;
		}
	}
#if 0
	if (kzPickEvent (surf, kzEVENT_MOTION))
	{
		fprintf (stderr, "getMotion -> (dev=%u, (%06u; %06u)\n",
				surf->mot.device, surf->mot.x, surf->mot.y);
	}
#endif
	surf->ptr = NULL;
	return result;
}

static inline bool
eventPass_ (struct kzSurf_t *surf, EGLDisplay *edpy, EGLSurface *esurf)
{
	uint8_t passState = 0u;
	/* store events num */
	uint32_t events = surf->events;
	if (!surf->events)
		return false;
	/* call proccess */
	passState = eventPass (surf);
	if (passState & kzPASS_SWAP)
		eglSwapBuffers (*edpy, *esurf);
	/* check events */
	if (surf->events & ~events)
	{
		surf->events &= events;
	}
	if (surf->events == events)
	{
		surf->events = kzEVENT_ZERO;
		if (surf->queue)
			memset (&(surf->queue->off), 0,
					sizeof (struct kzSurfQueueOffset_t));
	}
	if (passState & kzPASS_STOP)
		return false;
	return true;
}

static inline void
eventLoop (struct kzSurf_t *surf,
		EGLNativeDisplayType *xdpy, EGLNativeWindowType *xwin,
		EGLDisplay *edpy, EGLSurface *esurf)
{
	struct kzSurfShape_t shape;
	/* X11 events */
	XEvent event;
	KeySym keysym;
	bool run = true;
	/* save values */
	memcpy (&shape, &(surf->shape), sizeof (struct kzSurfShape_t));
	while (run || surf->events)
	{
		if (run && XNextEvent (*xdpy, &event) != Success)
			run = false;
		if (run)
		{
			switch (event.type)
			{
				/* Visibility */
				case ConfigureNotify: /* resize */
					surf->shape.wx = event.xconfigure.width;
					surf->shape.wy = event.xconfigure.height;
					surf->events |= kzEVENT_RESHAPE;
					/* update value */
					memcpy (&shape, &(surf->shape),
							sizeof (struct kzSurfShape_t));
				case Expose: /* redraw */
					surf->events |= kzEVENT_REDRAW;
					break;
				case MapNotify: /* show window */
					surf->events |= kzEVENT_SSHOW;
					break;
				case UnmapNotify: /* hide window */
					surf->events |= kzEVENT_SHIDE;
					break;
				/* Keyboard */
				case KeyRelease:
					surf->events |= kzEVENT_KEY;
					keysym = XLookupKeysym (&event.xkey, 0);
					surf->key.device = 0;
					surf->key.key = (keysym << 1) | 1;
					break;
				case KeyPress:
					surf->events |= kzEVENT_KEY;
					keysym = XLookupKeysym (&event.xkey, 0);
					surf->key.device = 0;
					surf->key.key = keysym << 1;
					break;
				/* Mouse */
				case MotionNotify:
					surf->events |= kzEVENT_MOTION;
					surf->mot.device = 1;
					surf->mot.x = event.xmotion.x;
					surf->mot.y = event.xmotion.y;
					break;
				case ButtonPress:
					surf->events |= kzEVENT_KEY;
					surf->key.device = 1;
					surf->key.key = (event.xbutton.button << 1);
					break;
				case ButtonRelease:
					surf->events |= kzEVENT_KEY;
					surf->key.device = 1;
					surf->key.key = (event.xbutton.button << 1) | 1;
					break;
				default:
					fprintf (stderr, "Unknown Event: %08d (0x%08X)\n",
							event.type, event.type);
			} /* //switch */
		} /* //if (run) */
		if (memcmp (&(surf->shape), &shape, sizeof (struct kzSurfShape_t)))
		{
			/* feedback */
			/* resize window */
			ec_if (XResizeWindow,
				XResizeWindow (*xdpy, *xwin, surf->shape.wx, surf->shape.wy))
			{
				/* restore value */
				memcpy (&(surf->shape), &shape, sizeof (struct kzSurfShape_t));
			}
		}
		/* pass events */
		if (surf->events && !eventPass_ (surf, edpy, esurf))
			run = 0;
	}
}

void
print_EGLInfo (EGLDisplay *edpy)
{
	fprintf (stderr, "INFO: EGL Version: %s\n",
			eglQueryString (*edpy, EGL_VERSION));
	fprintf (stderr, "INFO: EGL Vendor: %s\n",
			eglQueryString (*edpy, EGL_VENDOR));
	fprintf (stderr, "INFO: EGL APIs: %s\n",
			eglQueryString (*edpy, EGL_CLIENT_APIS));
}

void
print_GLInfo ()
{
	EGLint cc;
	fprintf (stderr, "INFO: GL Vendor: %s\n", glGetString (GL_VENDOR));
	fprintf (stderr, "INFO: GL Render: %s\n", glGetString (GL_RENDERER));
	fprintf (stderr, "INFO: GL Version: %s\n", glGetString (GL_VERSION));
	fprintf (stderr, "INFO: GL Shaders: %s\n",
			glGetString (GL_SHADING_LANGUAGE_VERSION));
	fprintf (stderr, "INFO: GL Extensions: %s\n", glGetString (GL_EXTENSIONS));
	glGetIntegerv (GL_MAX_VERTEX_ATTRIBS, &cc);
	fprintf (stderr, "INFO: GL Shader MAX Attribs:\t%d\n", cc);
	glGetIntegerv (GL_MAX_VERTEX_UNIFORM_VECTORS, &cc);
	fprintf (stderr, "INFO: GL Shader MAX Vectors (uniform):\t%d\n", cc);
	glGetIntegerv (GL_MAX_VARYING_VECTORS, &cc);
	fprintf (stderr, "INFO: GL Shader MAX Vectors (varying):\t%d\n", cc);
	glGetIntegerv (GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &cc);
	fprintf (stderr,
			"INFO: GL Shader MAX Texture units (combined):\t%d\n", cc);
	glGetIntegerv (GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &cc);
	fprintf (stderr, "INFO: GL Shader MAX Texture units (vertex):\t%d\n", cc);
	glGetIntegerv (GL_MAX_TEXTURE_IMAGE_UNITS, &cc);
	fprintf (stderr, "INFO: GL Shader MAX Texture units:\t%d\n", cc);
	glGetIntegerv (GL_MAX_FRAGMENT_UNIFORM_VECTORS, &cc);
	fprintf (stderr,
			"INFO GL Shader MAX Vectors (uniform, fragment):\t%d\n", cc);
}

int
main (int argc, char *argv[])
{
	/* self */
	struct kzSurf_t surf;
	/* X11 */
	EGLNativeDisplayType xdpy;
	EGLNativeWindowType xwin;
	/* EGL */
	EGLint cc;
	EGLConfig ecfg;
	EGLDisplay edpy = EGL_NO_DISPLAY;
	/* visual attribs */
	EGLint const eattribs[] =
	{
		EGL_RED_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8,
		EGL_ALPHA_SIZE, 8, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};
	/* surface attribs */
	EGLSurface esurf = EGL_NO_SURFACE;
	/* context attribs */
	/*
	 * Единственный дополнительный параметр, который можно указать для
	 *  eglCreateContext и которого нет в EGLConfig,
	 *	это EGL_CONTEXT_CLIENT_VERSION, нужный только при EGL_OPENGL_ES_API
	 *  значение атрибута EGL_CONTEXT_CLIENT_VERSION (integer)
	 *  	"1" соотвествует OpenGL ES 1.x
	 *  	"2" соотвествует OpenGL ES 2.x
	 *  Базовое значение равно "1"
	 */
	EGLContext ectx = EGL_NO_CONTEXT;
	EGLint const ectx_attr[] =
	{
		EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE
	};
	memset (&xdpy, 0, sizeof (EGLNativeDisplayType));
	memset (&xwin, 0, sizeof (EGLNativeWindowType));
	/* init */
	memset (&surf, 0, sizeof (struct kzSurf_t));
	surf.shape.wx = 800;
	surf.shape.wy = 800;
	/* X11: open display */
	ec_if (initEGL, initEGL (&xdpy, &edpy))
	{
		print_EGLInfo (&edpy);
		/* match config */
		eglChooseConfig (edpy, eattribs, &ecfg, 1, &cc);
		ec_ec (eglChooseConfig, cc == 0);
		/* create surface and context */
		ec_if (initSurfy, initSurfy (&esurf, &edpy, &ecfg, &ectx, ectx_attr,
				&xwin, &xdpy, surf.shape.wx, surf.shape.wy))
		{
			/* attach surfaces and context */
			ec_ec (eglMakeCurrent, !eglMakeCurrent (edpy, esurf, esurf, ectx));
			/* GL code */
			print_GLInfo ();
			eventLoop (&surf, &xdpy, &xwin, &edpy, &esurf);
		}
		/* GL code END */
		fprintf (stderr, "$ 0x%X\n", eglGetError ());
		/* egl exit */
		ec_ec (eglMakeCurrent, !eglMakeCurrent (edpy, EGL_NO_SURFACE,
				EGL_NO_SURFACE, EGL_NO_CONTEXT));
		if (esurf != EGL_NO_SURFACE)
			ec_ec (eglDestroySurface, !eglDestroySurface (edpy, esurf));
		if (ectx != EGL_NO_CONTEXT)
			ec_ec (eglDestroyContext, !eglDestroyContext (edpy, ectx));
	}
	/* exit */
	if (edpy != EGL_NO_DISPLAY)
		ec_ec (eglTerminate, !eglTerminate (edpy));
	/* x11 exit */
	killSurfy (&xwin, &xdpy);
	return 0;
}

