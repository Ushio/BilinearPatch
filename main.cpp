#include "pr.hpp"
#include <iostream>
#include <memory>

int main()
{
	using namespace pr;

	Config config;
	config.ScreenWidth = 1280;
	config.ScreenHeight = 768;
	config.SwapInterval = 1;
	Initialize( config );

	Camera3D camera;
	camera.origin = { -2, 2, 0 };
	camera.lookat = { 0, 0, 0 };
	camera.zUp = false;
	camera.zNear = 0.3;
	camera.zFar = 200;

	SetDataDir( ExecutableDir() );

	Image2DRGBA8 qImage;
	qImage.load( "image.png" );

	double e = GetElapsedTime();

	const int _stride = 1;
	Image2DRGBA8 image;

	ITexture* texture = CreateTexture();

	while( pr::NextFrame() == false )
	{
		if( IsImGuiUsingMouse() == false )
		{
			UpdateCameraBlenderLike( &camera );
		}

		static bool showNormal = false;

		float unit = 0.5f;
		static glm::vec3 p00 = { -unit, 0.5f, -unit };
		static glm::vec3 p10 = { -unit, 0.5f, unit };
		static glm::vec3 p01 = { unit, 0.5f, -unit };
		static glm::vec3 p11 = { unit, 0.5f, unit };

		image.allocate( GetScreenWidth() / _stride, GetScreenHeight() / _stride );

		glm::mat4 viewMat, projMat;
		GetCameraMatrix( camera, &projMat, &viewMat );
		CameraRayGenerator rayGenerator( viewMat, projMat, image.width(), image.height() );
		for( int j = 0; j < image.height(); ++j )
		{
			for( int i = 0; i < image.width(); ++i )
			{
				glm::vec3 ro, rd;
				rayGenerator.shoot( &ro, &rd, i, j, 0.5f, 0.5f );

				float tnear = FLT_MAX;
				float u;
				float v;

				float a = glm::dot( rd, glm::cross( p10 - p00, p11 - p01 ) );
				float c = glm::dot( p01 - p00, glm::cross( rd, p00 - ro ) );
				float b = glm::dot( rd, glm::cross( p10 - p11, p11 - ro ) ) - a - c;

				float det = b * b - 4.0f * a * c;
				if( 0.0f <= det )
				{
					// it might be better to check if a == 0
					float k = ( -b - copysignf( sqrt( det ), b ) ) / 2.0f;
					float u1 = k / a;
					float u2 = c / k;

					if( isfinite( u1 ) && 0.0f <= u1 && u1 <= 1.0f )
					{
						float thisU = u1;
						glm::vec3 pa = ( 1.0f - thisU ) * p00 + thisU * p10;
						glm::vec3 pb = ( 1.0f - thisU ) * p01 + thisU * p11;
						glm::vec3 ab = pb - pa;

						glm::vec3 n = glm::cross( ab, rd );
						float length2n = glm::dot( n, n );
						glm::vec3 n2 = glm::cross( pa - ro, n );
						float v0 = glm::dot( n2, rd ) / length2n;
						float t = glm::dot( n2, ab ) / length2n;

						if( 0.0f <= t && t < tnear && 0.0f <= v0 && v0 <= 1.0f )
						{
							tnear = t;
							u = thisU;
							v = v0;
						}
					}

					if( 0.0f <= u2 && u2 <= 1.0f )
					{
						float thisU = u2;
						glm::vec3 pa = ( 1.0f - thisU ) * p00 + thisU * p10;
						glm::vec3 pb = ( 1.0f - thisU ) * p01 + thisU * p11;
						glm::vec3 ab = pb - pa;

						glm::vec3 n = glm::cross( ab, rd );
						float length2n = glm::dot( n, n );
						glm::vec3 n2 = glm::cross( pa - ro, n );
						float v0 = glm::dot( n2, rd ) / length2n;
						float t = glm::dot( n2, ab ) / length2n;

						if( 0.0f <= t && t < tnear && 0.0f <= v0 && v0 <= 1.0f )
						{
							tnear = t;
							u = thisU;
							v = v0;
						}
					}
				}

				if( tnear != FLT_MAX )
				{
					glm::vec3 pa = ( 1.0f - u ) * p00 + u * p10;
					glm::vec3 pb = ( 1.0f - u ) * p01 + u * p11;
					glm::vec3 pc = ( 1.0f - v ) * p00 + v * p01;
					glm::vec3 pd = ( 1.0f - v ) * p10 + v * p11;
					glm::vec3 nlm = glm::cross( pb - pa, pc - pd );
					nlm = glm::normalize( nlm );

					glm::vec3 color;
					if( showNormal )
					{
						color = ( nlm + glm::vec3( 1.0f ) ) * 0.5f;
					}
					else
					{
						color = glm::vec3( u, v, 0 );

						int x = u * qImage.width();
						int y = ( 1.0f - v ) * qImage.height();
						x = glm::clamp( x, 0, qImage.width() - 1 );
						y = glm::clamp( y, 0, qImage.height() - 1 );
						glm::u8vec4 c = qImage( x, y );
						color = glm::vec4( c ) / 255.0f;
					}

					image( i, j ) = { 255 * color.r, 255 * color.g, 255 * color.b, 255 };
				}
				else
				{
					image( i, j ) = { 0, 0, 0, 255 };
				}
			}
		}

		texture->upload( image );
		ClearBackground( texture );

		BeginCamera( camera );
		PushGraphicState();
		SetDepthTest( true );

		DrawGrid( GridAxis::XZ, 1.0f, 10, { 128, 128, 128 } );
		DrawXYZAxis( 1.0f );

		ManipulatePosition( camera, &p00, 0.2f );
		ManipulatePosition( camera, &p10, 0.2f );
		ManipulatePosition( camera, &p01, 0.2f );
		ManipulatePosition( camera, &p11, 0.2f );

		DrawText( p00, "p00" );
		DrawText( p10, "p10" );
		DrawText( p01, "p01" );
		DrawText( p11, "p11" );

		int N = 10;
		for( int ui = 0; ui < N; ui++ )
		{
			float u = (float)ui / ( N - 1 );
			glm::vec3 pa = ( 1.0f - u ) * p00 + u * p10;
			glm::vec3 pb = ( 1.0f - u ) * p01 + u * p11;
			DrawLine( pa, pb, { 128, 64, 64 } );
		}
		for( int vi = 0; vi < N; vi++ )
		{
			float v = (float)vi / ( N - 1 );
			glm::vec3 pc = ( 1.0f - v ) * p00 + v * p01;
			glm::vec3 pd = ( 1.0f - v ) * p10 + v * p11;
			DrawLine( pc, pd, { 64, 128, 64 } );
		}

		static glm::vec3 ro0 = { 0.5f, 0.8f, 0.5f };
		static glm::vec3 ro1 = { 0.0f, 0.0f, 0.0f };

		ManipulatePosition( camera, &ro0, 0.1f );
		ManipulatePosition( camera, &ro1, 0.1f );
		DrawArrow( ro0, ro1, 0.01f, { 255, 0, 0 } );
		DrawLine( ro0, ( ro1 - ro0 ) * 100.0f, { 255, 0, 0 } );

		glm::vec3 ro = ro0;
		glm::vec3 rd = ro1 - ro0;

		float a = glm::dot( rd, glm::cross( p10 - p00, p11 - p01 ) );
		float c = glm::dot( p01 - p00, glm::cross( rd, p00 - ro ) );
		float b = glm::dot( rd, glm::cross( p10 - p11, p11 - ro ) ) - a - c;

		float det = b * b - 4.0f * a * c;
		if( 0.0f <= det )
		{
			// it might be better to check if a == 0 
			float k = ( -b - copysignf( sqrt( det ), b ) ) / 2.0f;
			float u1 = k / a;
			float u2 = c / k;

			float tnear = FLT_MAX;
			float u;
			float v;
			
			if( isfinite( u1 ) && 0.0f <= u1 && u1 <= 1.0f )
			{
				float thisU = u1;
				glm::vec3 pa = ( 1.0f - thisU ) * p00 + thisU * p10;
				glm::vec3 pb = ( 1.0f - thisU ) * p01 + thisU * p11;
				glm::vec3 ab = pb - pa;

				glm::vec3 n = glm::cross( ab, rd );
				float length2n = glm::dot( n, n );
				glm::vec3 n2 = glm::cross( pa - ro, n );
				float v0 = glm::dot( n2, rd ) / length2n;
				float t = glm::dot( n2, ab ) / length2n;

				if( 0.0f <= t && t < tnear && 0.0f <= v0 && v0 <= 1.0f )
				{
					tnear = t;
					u = thisU;
					v = v0;
				}
			}

			if( 0.0f <= u2 && u2 <= 1.0f )
			{
				float thisU = u2;
				glm::vec3 pa = ( 1.0f - thisU ) * p00 + thisU * p10;
				glm::vec3 pb = ( 1.0f - thisU ) * p01 + thisU * p11;
				glm::vec3 ab = pb - pa;

				glm::vec3 n = glm::cross( ab, rd );
				float length2n = glm::dot( n, n );
				glm::vec3 n2 = glm::cross( pa - ro, n );
				float v0 = glm::dot( n2, rd ) / length2n;
				float t = glm::dot( n2, ab ) / length2n;

				if( 0.0f <= t && t < tnear && 0.0f <= v0 && v0 <= 1.0f )
				{
					tnear = t;
					u = thisU;
					v = v0;
				}
			}

			if( tnear != FLT_MAX )
			{
				glm::vec3 pa = ( 1.0f - u ) * p00 + u * p10;
				glm::vec3 pb = ( 1.0f - u ) * p01 + u * p11;
				DrawLine( pa, pb, { 255, 0, 255 }, 3 );
				DrawText( pa, "pa" );
				DrawText( pb, "pb" );

				glm::vec3 pc = ( 1.0f - v ) * p00 + v * p01;
				glm::vec3 pd = ( 1.0f - v ) * p10 + v * p11;
				DrawLine( pc, pd, { 128, 255, 128 }, 3 );

				glm::vec3 p = ro + rd * tnear;
				DrawSphere( p, 0.02f, { 255, 255, 255 } );

				glm::vec3 nlm = glm::cross( pb - pa, pc - pd );

				DrawArrow( p, p + glm::normalize( nlm ) * 0.2f, 0.005f, { 0, 0, 255 } );
			}
		}

		PopGraphicState();
		EndCamera();

		BeginImGui();

		ImGui::SetNextWindowSize( { 300, 400 }, ImGuiCond_Once );
		ImGui::Begin( "Panel" );
		ImGui::Text( "fps = %f", GetFrameRate() );

		ImGui::InputFloat( "fovy", &camera.fovy, 0.1f );
		ImGui::Text( "d %.2f", glm::length( camera.origin ) );

		ImGui::Checkbox( "showNormal", &showNormal );

		if( ImGui::Button( "Set Quad 1" ) )
		{
			printf( "p00 = { %.5f, %.5f, %.5f };\n", p00.x, p00.y, p00.z );
			printf( "p10 = { %.5f, %.5f, %.5f };\n", p10.x, p10.y, p10.z );
			printf( "p01 = { %.5f, %.5f, %.5f };\n", p01.x, p01.y, p01.z );
			printf( "p11 = { %.5f, %.5f, %.5f };\n", p11.x, p11.y, p11.z );

			p00 = { -unit, 0.5f, -unit };
			p10 = { -unit, 0.5f, unit };
			p01 = { unit, 0.5f, -unit };
			p11 = { unit, 0.5f, unit };
		}
		if( ImGui::Button( "Set Quad 2" ) )
		{
			p00 = { -0.35742, 0.74270, 0.52905 };
			p10 = { -0.78684, 0.26789, -0.31149 };
			p01 = { 0.50000, 0.50000, -0.50000 };
			p11 = { 0.35920, 0.30494, 0.81188 };
		}
		if( ImGui::Button( "Set Quad 3" ) )
		{
			p00 = { 0.03157, -0.26977, 0.05121 };
			p10 = { -0.50000, 0.67611, 0.36598 };
			p01 = { 0.50000, 0.70006, -0.50000 };
			p11 = { 0.44838, 0.03162, 0.50000 };
		}
		ImGui::End();

		EndImGui();
	}

	pr::CleanUp();
}
