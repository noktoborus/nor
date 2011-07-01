/* vim: ft=c ff=unix fenc=utf-8
 * file: printeglconfig.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>

#define GRI_NONE	0
#define GRI_FALSE	GRI_NONE
#define GRI_TRUE	1

#define GRI_ERROR_CRIT_ATTR	5
#define GRI_ERROR_WARN_ATTR	6
#define GRI_ERROR_NORM_ATTR	7
static struct __griError_ctl_t
{
		int level;
			void *freestack[128];
} __griError_ctl__ =
{
		GRI_ERROR_NORM_ATTR
};

void
griCtlErrorCommit (int val)
{
		__griError_ctl__.level = val;
}

void
griCtlErrorPush (int type, void *val)
{
}

void
griError (const char *sbase, ...)
{
		fprintf (stderr, "# EE: %s\n", sbase);
			if (__griError_ctl__.level == GRI_ERROR_CRIT_ATTR)
						exit (1);
}

#define print_eInt(x) \
		{\
					if (eglGetConfigAttrib (dpy, config, x, &val) == EGL_TRUE)\
						printf ("%30s: %d\n", #x, val);\
					else\
						printf ("%30s: not present\n", #x);\
				}

#define print_eBool(x) \
		{\
					if (eglGetConfigAttrib (dpy, config, x, &val))\
						printf ("%30s: EGL_%s\n", #x, val ? "TRUE" : "FALSE");\
					else\
						printf ("%30s: not present\n", #x);\
				}

#define print_enum_Head(x) \
		{\
					if (eglGetConfigAttrib (dpy, config, x, &val))\
					{\
									printf ("%30s: (0x%0*x)", #x, 4, val);\
									switch (val)\
									{

#define print_enum(x) \
														case x:\
															   					printf (#x);\
															break;

#define print_enum_Foot() \
														default:\
																					printf ("unknown");\
													}\
									printf ("\n");\
								}\
					else\
						printf ("not present\n");\
				}

#define print_mask_Head(x) \
		{\
					if (eglGetConfigAttrib (dpy, config, x, &val))\
					{\
									printf ("%30s: (0x%0*x) ", #x, 4, val);

#define print_mask(x) \
									if (val & x)\
										printf (#x" | ");

#define print_mask_Foot() \
									printf ("\n");\
								}\
					else\
						printf ("not present\n");\
				}

void
print_eglConfig (EGLDisplay dpy, EGLConfig config)
{
		EGLint val;
			printf ("# EGLconfig:\n");
				print_eInt (EGL_BUFFER_SIZE);
					print_eInt (EGL_RED_SIZE);
						print_eInt (EGL_GREEN_SIZE);
							print_eInt (EGL_BLUE_SIZE);
								print_eInt (EGL_LUMINANCE_SIZE);
									print_eInt (EGL_ALPHA_SIZE);
										print_eInt (EGL_ALPHA_MASK_SIZE);
											print_eBool (EGL_BIND_TO_TEXTURE_RGB);
												print_eBool (EGL_BIND_TO_TEXTURE_RGBA);
													print_enum_Head (EGL_COLOR_BUFFER_TYPE);
														print_enum (EGL_RGB_BUFFER);
															print_enum (EGL_LUMINANCE_BUFFER);
																print_enum_Foot ();
																	print_enum_Head (EGL_CONFIG_CAVEAT);
																		print_enum (EGL_NONE);
																			print_enum (EGL_SLOW_CONFIG);
																				print_enum (EGL_NON_CONFORMANT_CONFIG);
																					print_enum_Foot ();
																						print_eInt (EGL_CONFIG_ID);
																							print_mask_Head (EGL_CONFORMANT);
																								print_mask (EGL_OPENGL_ES_BIT);
																									print_mask (EGL_OPENGL_ES2_BIT);
																										print_mask (EGL_OPENVG_BIT);
																											print_mask (EGL_OPENGL_BIT);
																												print_mask_Foot ();
																													print_eInt (EGL_DEPTH_SIZE);
																														print_eInt (EGL_LEVEL);
																															print_eInt (EGL_MATCH_NATIVE_PIXMAP);
																																print_eInt (EGL_MAX_PBUFFER_WIDTH);
																																	print_eInt (EGL_MAX_PBUFFER_HEIGHT);
																																		print_eInt (EGL_MAX_PBUFFER_PIXELS);
																																			print_eInt (EGL_MAX_SWAP_INTERVAL);
																																				print_eInt (EGL_MIN_SWAP_INTERVAL);
																																					print_eBool (EGL_NATIVE_RENDERABLE);
																																						print_eInt (EGL_NATIVE_VISUAL_ID);
																																							print_eInt (EGL_NATIVE_VISUAL_TYPE);
																																								print_mask_Head (EGL_RENDERABLE_TYPE);
																																									print_mask (EGL_OPENGL_ES_BIT);
																																										print_mask (EGL_OPENGL_ES2_BIT);
																																											print_mask (EGL_OPENVG_BIT);
																																												print_mask (EGL_OPENGL_BIT);
																																													print_mask_Foot ();
																																														print_eInt (EGL_SAMPLE_BUFFERS);
																																															print_eInt (EGL_SAMPLES);
																																																print_eInt (EGL_STENCIL_SIZE);
																																																	print_mask_Head (EGL_SURFACE_TYPE);
																																																		print_mask (EGL_PBUFFER_BIT);
																																																			print_mask (EGL_PIXMAP_BIT);
																																																				print_mask (EGL_WINDOW_BIT);
																																																					print_mask (EGL_VG_COLORSPACE_LINEAR_BIT);
																																																						print_mask (EGL_VG_ALPHA_FORMAT_PRE_BIT);
																																																							print_mask (EGL_MULTISAMPLE_RESOLVE_BOX_BIT);
																																																								print_mask (EGL_SWAP_BEHAVIOR_PRESERVED_BIT);
																																																									print_mask_Foot ();
																																																										print_mask_Head (EGL_TRANSPARENT_TYPE);
																																																											print_mask (EGL_TRANSPARENT_RGB);
																																																												print_mask_Foot ();
																																																													print_eInt (EGL_TRANSPARENT_RED_VALUE);
																																																														print_eInt (EGL_TRANSPARENT_GREEN_VALUE);
																																																															print_eInt (EGL_TRANSPARENT_BLUE_VALUE);

																																																																printf ("# END\n");
}
#undef print_eInt
#undef print_eBool
#undef print_enum_Head
#undef print_enum
#undef print_enum_Foot
#undef print_mask_Head
#undef print_mask
#undef print_mask_Foot

int
main (int argc, char *argv[])
{
		EGLint config_cc;
			EGLint cc;
				EGLConfig config[100];
					EGLDisplay edpy = EGL_NO_DISPLAY;

						griCtlErrorCommit (GRI_ERROR_CRIT_ATTR);
							/*
							 * 	 * Запрос устройства по-умолчанию
							 * 	 	 */
							edpy = eglGetDisplay (EGL_DEFAULT_DISPLAY);
								if (edpy == EGL_NO_DISPLAY)
											griError ("EGL_GET_DISPLAY_FAIL");
									/*
									 * 	 * Инициализация устройства
									 * 	 	 */
									if (eglInitialize (edpy, NULL, NULL) == EGL_FALSE)
											{
														griError ("EGL_INIT_FAIL");
															}
										/*
										 * 	 * Распечатка версии EGL, поставщика и доступных API
										 * 	 	 */
										printf ("EGL Version: %s\n", eglQueryString (edpy, EGL_VERSION));
											printf ("EGL Vendor: %s\n", eglQueryString (edpy, EGL_VENDOR));
												printf ("EGL APIs: %s\n", eglQueryString (edpy, EGL_CLIENT_APIS));

													/*
													 * 	 * Запрос подходящей конфигурации, подходящей под атрибуты egl_attribs
													 * 	 	 * EGLDisplay dpy		: указатель на интересующее устройство
													 * 	 	 	 * EGLConfig *config	: указатель на массив возвращаемых конфигов
													 * 	 	 	 	 * EGLint config_size	: максимальное количество возможных элементов
													 * 	 	 	 	 	 * 		в *config
													 * 	 	 	 	 	 	 * EGLint *num_config	: количество возвращённых (в *config) конфигураций
													 * 	 	 	 	 	 	 	 */
													if (eglGetConfigs (edpy, config, 100, &config_cc)
																		== EGL_FALSE)
																griError ("EGL_CHOOSE_CONFIG_FAIL");

														if (!config_cc)
																	griError ("EGL_NO_CONFIG_AVAILABLE");

															cc = config_cc;
																printf ("# count: %d\n", config_cc);
																	while (cc--)
																			{
																						printf ("# EGLConfig: %d/%d\n", cc, config_cc);
																								print_eglConfig (edpy, config[cc]);
																									}

																		printf ("@ END\n");
																			return 0;
}


