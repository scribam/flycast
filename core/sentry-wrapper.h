#pragma once

#include "log/InMemoryListener.h"
#include "version.h"
#include "wsi/context.h"

#include <sentry.h>

namespace sentry
{
	// TODO: send InMemoryListener::getInstance()->getLog() as a sentry attachment
	// sentry_options_add_attachment? or full log but how to trim content?
	
	static sentry_value_t on_crash_callback(const sentry_ucontext_t *uctx, sentry_value_t event, void *closure)
	{
		sentry_value_t game = sentry_value_new_object();
		sentry_value_set_by_key(game, "name", sentry_value_new_string(settings.content.gameId.c_str()));
		sentry_set_context("game", game);
		
		GraphicsContext *gctx = GraphicsContext::Instance();
		if (gctx != nullptr) {
			sentry_value_u api_type{};
			switch (config::RendererType)
			{
				case RenderType::OpenGL:
				case RenderType::OpenGL_OIT:
					api_type = sentry_value_new_string("OpenGL");
					break;
				case RenderType::Vulkan:
				case RenderType::Vulkan_OIT:
					api_type = sentry_value_new_string("Vulkan");
					break;
				case RenderType::DirectX9:
					api_type = sentry_value_new_string("DirectX 9");
					break;
				case RenderType::DirectX11:
				case RenderType::DirectX11_OIT:
					api_type = sentry_value_new_string("DirectX 11");
					break;
			}

			sentry_value_t gpu = sentry_value_new_object();
			sentry_value_set_by_key(gpu, "name", sentry_value_new_string(gctx->getDriverName().c_str()));
			sentry_value_set_by_key(gpu, "version", sentry_value_new_string(gctx->getDriverVersion().c_str()));
			sentry_value_set_by_key(gpu, "api_type", api_type);
			sentry_set_context("gpu", gpu);
		}
		
		return event;
	}

#ifdef USE_SENTRY_NATIVE
	static int init()
	{
		sentry_options_t *options = sentry_options_new();
#ifndef NDEBUG
		sentry_options_set_debug(options, 1);
#endif
#ifdef SENTRY_UPLOAD
		sentry_options_set_dsn(options, SENTRY_UPLOAD);
#endif
		sentry_options_set_on_crash(options, on_crash_callback, nullptr);
		sentry_options_set_release(options, "flycast@" GIT_VERSION);
		return sentry_init(options);
	}
	
	static void close()
	{
		sentry_close();
	}
#else
	void init() {}
	void close() {}
#endif
};
