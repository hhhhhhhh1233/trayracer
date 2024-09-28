#include <algorithm>
#include <chrono>
#include <iostream>
#include <stdio.h>
#include <string>
#include "color.h"
#include "window.h"
#include "vec3.h"
#include "raytracer.h"
#include "sphere.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image_write.h"

#define degtorad(angle) angle * MPI / 180

int main(int argc, char *argv[])
{ 
	unsigned w = 400;
	unsigned h = 300;
    int raysPerPixel = 1;
    int maxBounces = 5;

	for (int i = 0; i < argc; i++)
	{
		if (std::string(argv[i]).compare("-w") == 0)
		{
			i++;
			w = std::stoi(argv[i]);
		}
		if (std::string(argv[i]).compare("-h") == 0)
		{
			i++;
			h = std::stoi(argv[i]);
		}
		if (std::string(argv[i]).compare("-rpp") == 0)
		{
			i++;
			raysPerPixel = std::stoi(argv[i]);
		}
		if (std::string(argv[i]).compare("-mb") == 0)
		{
			i++;
			maxBounces = std::stoi(argv[i]);
		}
	}

    // Display::Window wnd;
    
    // wnd.SetTitle("TrayRacer");
    // 
    // if (!wnd.Open())
    //     return 1;

    std::vector<Color> framebuffer;

    // const unsigned w = 400;
    // const unsigned h = 300;
    framebuffer.resize(w * h);
    

    Raytracer rt = Raytracer(w, h, framebuffer, raysPerPixel, maxBounces);

    // Create some objects
    Material* mat = new Material();
    mat->type = "Lambertian";
    mat->color = { 0.5,0.5,0.5 };
    mat->roughness = 0.3;
    Sphere* ground = new Sphere(1000, { 0,-1000, -1 }, mat);
    rt.AddObject(ground);

    for (int it = 0; it < 12; it++)
    {
        {
            Material* mat = new Material();
                mat->type = "Lambertian";
                float r = RandomFloat();
                float g = RandomFloat();
                float b = RandomFloat();
                mat->color = { r,g,b };
                mat->roughness = RandomFloat();
                const float span = 10.0f;
                Sphere* ground = new Sphere(
                    RandomFloat() * 0.7f + 0.2f,
                    {
                        RandomFloatNTP() * span,
                        RandomFloat() * span + 0.2f,
                        RandomFloatNTP() * span
                    },
                    mat);
            rt.AddObject(ground);
        }{
            Material* mat = new Material();
            mat->type = "Conductor";
            float r = RandomFloat();
            float g = RandomFloat();
            float b = RandomFloat();
            mat->color = { r,g,b };
            mat->roughness = RandomFloat();
            const float span = 30.0f;
            Sphere* ground = new Sphere(
                RandomFloat() * 0.7f + 0.2f,
                {
                    RandomFloatNTP() * span,
                    RandomFloat() * span + 0.2f,
                    RandomFloatNTP() * span
                },
                mat);
            rt.AddObject(ground);
        }{
            Material* mat = new Material();
            mat->type = "Dielectric";
            float r = RandomFloat();
            float g = RandomFloat();
            float b = RandomFloat();
            mat->color = { r,g,b };
            mat->roughness = RandomFloat();
            mat->refractionIndex = 1.65;
            const float span = 25.0f;
            Sphere* ground = new Sphere(
                RandomFloat() * 0.7f + 0.2f,
                {
                    RandomFloatNTP() * span,
                    RandomFloat() * span + 0.2f,
                    RandomFloatNTP() * span
                },
                mat);
            rt.AddObject(ground);
        }
    }
    
    bool exit = false;

    // camera
    bool resetFramebuffer = false;
    vec3 camPos = { 0,1.0f,10.0f };
    vec3 moveDir = { 0,0,0 };

    // wnd.SetKeyPressFunction([&exit, &moveDir, &resetFramebuffer](int key, int scancode, int action, int mods)
    // {
    //     switch (key)
    //     {
    //     case GLFW_KEY_ESCAPE:
    //         exit = true;
    //         break;
    //     case GLFW_KEY_W:
    //         moveDir.z -= 1.0f;
    //         resetFramebuffer |= true;
    //         break;
    //     case GLFW_KEY_S:
    //         moveDir.z += 1.0f;
    //         resetFramebuffer |= true;
    //         break;
    //     case GLFW_KEY_A:
    //         moveDir.x -= 1.0f;
    //         resetFramebuffer |= true;
    //         break;
    //     case GLFW_KEY_D:
    //         moveDir.x += 1.0f;
    //         resetFramebuffer |= true;
    //         break;
    //     case GLFW_KEY_SPACE:
    //         moveDir.y += 1.0f;
    //         resetFramebuffer |= true;
    //         break;
    //     case GLFW_KEY_LEFT_CONTROL:
    //         moveDir.y -= 1.0f;
    //         resetFramebuffer |= true;
    //         break;
    //     default:
    //         break;
    //     }
    // });

    float pitch = 0;
    float yaw = 0;
    float oldx = 0;
    float oldy = 0;

    // wnd.SetMouseMoveFunction([&pitch, &yaw, &oldx, &oldy, &resetFramebuffer](double x, double y)
    // {
    //     x *= -0.1;
    //     y *= -0.1;
    //     yaw = x - oldx;
    //     pitch = y - oldy;
    //     resetFramebuffer |= true;
    //     oldx = x;
    //     oldy = y;
    // });

    float rotx = 0;
    float roty = 0;

    // number of accumulated frames
    int frameIndex = 0;

    std::vector<Color> framebufferCopy;
    framebufferCopy.resize(w * h);

    // rendering loop
    // while (wnd.IsOpen() && !exit)
    {
        resetFramebuffer = false;
        moveDir = {0,0,0};
        pitch = 0;
        yaw = 0;

        // poll input
        // wnd.Update();

        rotx -= pitch;
        roty -= yaw;

        moveDir = normalize(moveDir);

        mat4 xMat = (rotationx(rotx));
        mat4 yMat = (rotationy(roty));
        mat4 cameraTransform = multiply(yMat, xMat);

        camPos = camPos + transform(moveDir * 0.2f, cameraTransform);
        
        cameraTransform.m30 = camPos.x;
        cameraTransform.m31 = camPos.y;
        cameraTransform.m32 = camPos.z;

        rt.SetViewMatrix(cameraTransform);
        
        if (resetFramebuffer)
        {
            rt.Clear();
            frameIndex = 0;
        }

		auto start = std::chrono::high_resolution_clock::now();
        rt.Raytrace();
		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
		std::cout << "Time: " << duration.count()/1000.0f << "\n";
		std::cout << "Resolution: " << w << "x" << h << "\n";
		std::cout << "Rays Per Pixel: " << raysPerPixel << "\n";
		std::cout << "Max Bounces: " << maxBounces << "\n";
        frameIndex++;

		std::vector<uint8_t> framebufferCopyTest;
		std::vector<uint8_t> forwards;

        // Get the average distribution of all samples
        {
            size_t p = 0;
            for (Color const& pixel : framebuffer)
            {
                framebufferCopy[p] = pixel;
                framebufferCopy[p].r /= frameIndex;
                framebufferCopy[p].g /= frameIndex;
                framebufferCopy[p].b /= frameIndex;
				// Color c;
				// c.r = 10;
				// c.g = 10;
				// c.b = 10;
				// std::cout << "R: " << framebufferCopy[p].r << "\n";
				framebufferCopyTest.push_back(std::clamp(int(framebufferCopy[p].r * 255), 0, 255));
				framebufferCopyTest.push_back(std::clamp(int(framebufferCopy[p].g * 255), 0, 255));
				framebufferCopyTest.push_back(std::clamp(int(framebufferCopy[p].b * 255), 0, 255));
				// int i = std::clamp(int(framebufferCopy[p].b * 255), 0, 255);
                p++;
            }
        }

		for (int y = h - 1; y >= 0; y--)
		{
			for (int x = 0; x < w * 3; x++)
			{
				forwards.push_back(framebufferCopyTest[(w * 3) * y + x]);
			}
		}


		stbi_write_png("Pic.png", w, h, 3, forwards.data(), w*3);

        // glClearColor(0, 0, 0, 1.0);
        // glClear( GL_COLOR_BUFFER_BIT );

        // wnd.Blit((float*)&framebufferCopy[0], w, h);
        // wnd.SwapBuffers();
    }

    // if (wnd.IsOpen())
    //     wnd.Close();

    return 0;
} 
