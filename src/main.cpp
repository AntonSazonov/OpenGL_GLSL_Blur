﻿
#include <cstdio>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

// For 3.3 core context, if not - use 4.5
#define SAN_GL_33
#define SAN_GL_DEBUG
#include "san_gl_window.hpp"
#include "san_gl_shader.hpp"
#include "san_gl_framebuffer.hpp"
#include "san_gl_vao.hpp"
#include "san_gl_utils.hpp"

using namespace san;

class window : public gl::window {
	gl::vao_quad		m_quad;	// dummy quad

	gl::framebuffer		m_fb1;
	gl::framebuffer		m_fb2;

	gl::shader::vert	m_vert;
	gl::shader::frag	m_frag;
	gl::shader::prog	m_prog;

	gl::gaussian_blur_kernel <>	m_kernel;

public:
	window( const glm::ivec2 & size )
		: gl::window( size, "OpenGL Blur Example. Photo by Sophie Turner on Unsplash (https://unsplash.com/photos/LZVmvKlchM0)" )
		, m_fb1( size )
		, m_fb2( size )
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::GetIO().IniFilename = nullptr; // disable use of ini-file for ImGui.
		ImGui::StyleColorsDark();
		ImGui_ImplGlfw_InitForOpenGL( gl::window::get_window(), true );
		ImGui_ImplOpenGL3_Init( "#version 130" );

		// load texture...
		if ( !gl::load_texture( m_fb1.get_tex(), m_fb1.get_size(), "images/sophie-turner-LZVmvKlchM0-unsplash.jpg", 1/*flip vertically*/ ) ) {
			if ( !gl::load_texture( m_fb1.get_tex(), m_fb1.get_size(), "../images/sophie-turner-LZVmvKlchM0-unsplash.jpg", 1/*flip vertically*/ ) ) {
				fprintf( stderr, "load_texture(): error\n" );
				close();
				return;
			}
		}

		// compile shaders...
		if ( !m_vert.compile_from_file( "shader/blur.vert" ) ||
			 !m_frag.compile_from_file( "shader/blur.frag" ) )
		{
			fprintf( stderr, "Shader compile error.\n" );
			close();
			return;
		}

		m_prog.attach( m_vert );
		m_prog.attach( m_frag );

		if ( !m_prog.link() ) {
			fprintf( stderr, "Shader link error.\n" );
			close();
			return;
		}

		// Always detach shaders after a successful link.
		m_prog.detach( m_vert );
		m_prog.detach( m_frag );
		m_prog.bind();
		m_prog.uniform( "u_tex", 0 );

		glUseProgram( 123 ); // Test debug context error handling.
	}

	virtual ~window() {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void on_key( int key, int scancode, int action, int mods ) override {
		if ( action == GLFW_PRESS &&
				key == GLFW_KEY_ESCAPE ) close();
	}

	void on_frame( double time, const glm::ivec2 & fb, const glm::ivec2 & /*mouse*/ ) override {
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		{
			ImGui::SetNextWindowPos( ImVec2( 10, 10 ), ImGuiCond_FirstUseEver );
			ImGui::SetNextWindowSize( ImVec2( 300, 0 ), ImGuiCond_FirstUseEver );
			ImGui::Begin( "Gaussian blur kernel" );
			{
				static int		radius		= 20;
				static float	sigma_coeff	= 2.5;

				ImGui::SliderInt( "Radius", &radius, 2, 64 );
				ImGui::SliderFloat( "Sigma coeff.", &sigma_coeff, 2.f, 5.f );

				m_kernel.set_sigma_coeff( sigma_coeff );
				m_kernel.set_radius( radius );

				ImGui::PlotHistogram( "Kernel"/*label*/, m_kernel.get_values()/*values*/, m_kernel.get_size()/*num. values*/, 0/*v.offset*/, nullptr/*ovl.text*/, 0.f/*scale min*/, 1.f/*scale max*/, ImVec2( 0, 128 )/*graph size*/ );

				m_kernel.normalize();

				static bool vsync = true;
				ImGui::Checkbox( "V-Sync", &vsync );
				glfwSwapInterval( int(vsync) );

				ImGui::Text( "-------------------------" );
				ImGui::Text( "%.3f ms/f. (%.1f FPS)", 1000.f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate );
			}
			ImGui::End();

			if ( time > 3 ) fprintf( stderr, "\r%5.2f ms/f., %7.2f FPS", 1000.f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate );
		}

		// No multisampling while blur.
		glDisable( GL_MULTISAMPLE );
		m_prog.uniform( "u_viewport", fb );
		m_prog.uniform( "u_radius", m_kernel.get_radius() );

		// We need only one half of kernel
		m_prog.uniform( "u_kernel", m_kernel.get_values() + m_kernel.get_radius(), m_kernel.get_radius() );

		// hor. blur (1st pass)
		// fb1 --> fb2
		m_prog.uniform( "u_direction", glm::vec2( 1, 0 ) );
		m_fb1.bind_as_tex( 0 /*active texture*/ );
		m_fb2.bind_as_fbo();
		m_quad.draw();

		// vert. blur (2nd pass)
		// fb2 --> screen
		m_prog.uniform( "u_direction", glm::vec2( 0, 1 ) );
		m_fb2.bind_as_tex( 0 /*active texture*/ );
		glBindFramebuffer( GL_FRAMEBUFFER, 0 );
		m_quad.draw();

		// Histogram looks more smoother with multisampling.
		glEnable( GL_MULTISAMPLE );
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData( ImGui::GetDrawData() );
	}
}; // class window

int main() {
	window app( { 1280, 720 } );
	fprintf( stderr, "\n" );
	if ( app ) app.run();
	return 0;
}
