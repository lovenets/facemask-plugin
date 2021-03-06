/*
* Face Masks for SlOBS
* smll - streamlabs machine learning library
*
* Copyright (C) 2017 General Workings Inc
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

// From this example:
//
// https://learnopengl.com/In-Practice/Text-Rendering
//
//


#include "OBSFont.hpp"

#pragma warning( push )
#pragma warning( disable: 4127 )
#pragma warning( disable: 4201 )
#pragma warning( disable: 4456 )
#pragma warning( disable: 4458 )
#pragma warning( disable: 4459 )
#pragma warning( disable: 4505 )

#include <libobs/obs-module.h>
#include <libobs/graphics/matrix4.h>
#include <libobs/graphics/image-file.h>
#include <tiny_obj_loader.h>

#pragma warning( pop )

// FreeType
#include <ft2build.h>
#include FT_FREETYPE_H


namespace smll {

	OBSFont::OBSFont(const std::string& filename, int size) {
		
		obs_enter_graphics();
		char* f = obs_module_file("effects/color_alpha_tex.effect");
		char* errorMessage = nullptr;
		m_effect = gs_effect_create_from_file(f, &errorMessage);
		if (!m_effect || errorMessage) {
			std::string error(errorMessage);
			bfree((void*)errorMessage);
			obs_leave_graphics();
			throw std::runtime_error(error);
		}
		bfree(f);
		obs_leave_graphics();

		SetFont(filename, size);
	}

	OBSFont::~OBSFont() {
		DestroyFontInfo();
		obs_enter_graphics();
		gs_effect_destroy(m_effect);
		obs_leave_graphics();
	}

	void OBSFont::DestroyFontInfo() {
		obs_enter_graphics();
		for (int i = 0; i < m_fontInfos.size(); i++) {
			if (m_fontInfos[i].texture) {
				gs_texture_destroy(m_fontInfos[i].texture);
			}
		}
		obs_leave_graphics();
		m_fontInfos.clear();
	}

	void OBSFont::SetFont(const std::string& filename, int size) {

		// Init FreeType
		FT_Library ft;
		if (FT_Init_FreeType(&ft))
			blog(LOG_ERROR, "ERROR::FREETYPE: Could not init FreeType Library");

		// Load font as face
		FT_Face face;
		if (FT_New_Face(ft, filename.c_str(), 0, &face))
			blog(LOG_ERROR, "ERROR::FREETYPE: Failed to load font");

		// Set size to load glyphs as
		FT_Set_Pixel_Sizes(face, 0, size);

		DestroyFontInfo();

		obs_enter_graphics();

		// Let's do ascii 32 - 126
		for (char c = 32; c < 127; c++)
		{
			// Load character glyph 
			if (FT_Load_Char(face, c, FT_LOAD_RENDER))
			{
				blog(LOG_ERROR, "ERROR::FREETYTPE: Failed to load Glyph");
				continue;
			}

			FontInfo fi;
			vec2_set(&(fi.size), (float)face->glyph->bitmap.width, (float)face->glyph->bitmap.rows);
			vec2_set(&(fi.bearing), (float)face->glyph->bitmap_left, (float)-face->glyph->bitmap_top);
			fi.advance = (float)face->glyph->advance.x;
			fi.texture = nullptr;
			if (face->glyph->bitmap.rows > 0 && face->glyph->bitmap.width > 0) {
				fi.texture = gs_texture_create(face->glyph->bitmap.width,
					face->glyph->bitmap.rows, GS_R8, 1,
					(const uint8_t**)&(face->glyph->bitmap.buffer), 0);
			}

			m_fontInfos.emplace_back(fi);
		}

		obs_leave_graphics();

		// Destroy FreeType once we're finished
		FT_Done_Face(face);
		FT_Done_FreeType(ft);
	}

	void OBSFont::RenderText(const std::string& text, float xx, float y) {

		//gs_effect_t* defaultEffect = obs_get_base_effect(OBS_EFFECT_DEFAULT);

		/*
		gs_enable_color(true, true, true, true);
		gs_enable_depth_test(true);
		gs_depth_function(GS_LESS);
		gs_set_cull_mode(gs_cull_mode::GS_BACK);
		gs_enable_stencil_test(false);
		*/

		vec4 color;
		vec4_set(&color, 210.0f / 255.0f, 180.0f / 255.0f, 222.0f / 255.0f, 1.0f);

		gs_enable_blending(true);
		gs_blend_function_separate(
			gs_blend_type::GS_BLEND_SRCALPHA,
			gs_blend_type::GS_BLEND_INVSRCALPHA,
			gs_blend_type::GS_BLEND_SRCALPHA,
			gs_blend_type::GS_BLEND_INVSRCALPHA
		);

		std::string::const_iterator c;
		for (c = text.begin(); c != text.end(); c++) {
			int idx = (int)*c - 32;
			FontInfo& fi = m_fontInfos[idx];

			if (fi.texture) {
				gs_matrix_push();
				gs_matrix_translate3f(xx + fi.bearing.x, y + fi.bearing.y, 0.0f);
				while (gs_effect_loop(m_effect, "Draw")) {
					gs_effect_set_vec4(gs_effect_get_param_by_name(m_effect,
						"color"), &color);
					gs_effect_set_texture(gs_effect_get_param_by_name(m_effect,
						"image"), fi.texture);
					gs_draw_sprite(fi.texture, 0, (uint32_t)fi.size.x, (uint32_t)fi.size.y);
				}
				gs_matrix_pop();
			}
			xx += (fi.advance / 64.0f); // advance is 1/64 pixels
		}
	}

}
