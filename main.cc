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
	// Default values that can be overriden by commandline arguments
	unsigned w = 300;
	unsigned h = 300;
    int raysPerPixel = 1;
    int maxBounces = 5;
	int spheresAmount = 36;

	for (int i = 0; i < argc; i++)
	{
		if (std::string(argv[i]).compare("-w") == 0)
		{
			i++;
			w = std::stoi(argv[i]);
		}
		else if (std::string(argv[i]).compare("-h") == 0)
		{
			i++;
			h = std::stoi(argv[i]);
		}
		else if (std::string(argv[i]).compare("-rpp") == 0)
		{
			i++;
			raysPerPixel = std::stoi(argv[i]);
		}
		else if (std::string(argv[i]).compare("-b") == 0)
		{
			i++;
			maxBounces = std::stoi(argv[i]);
		}
		else if (std::string(argv[i]).compare("-s") == 0)
		{
			i++;
			spheresAmount = std::stoi(argv[i]);
		}
	}

	auto start = std::chrono::high_resolution_clock::now();
    std::vector<Color> framebuffer;

    framebuffer.resize(size_t(w * h));

    Raytracer rt = Raytracer(w, h, framebuffer, raysPerPixel, maxBounces);

    // Create some objects
	Material* mat = new Material();
	mat->type = "Lambertian";
	mat->color = { 0.5,0.5,0.5 };
	mat->roughness = 0.3;
	Sphere* ground = new Sphere(1000, { 0,-1000, -1 }, mat);
	rt.AddObject(ground);

	std::vector<std::string> Types = {"Lambertian", "Dielectric", "Conductor"};
	std::vector<float> Spans = {10, 30, 25};

    for (int it = 0; it < spheresAmount; it++)
    {
		Material* mat = new Material();
		mat->type = Types[it % 3];
		if (it % 3 == 1)
			mat->refractionIndex = 1.65;
		float r = RandomFloat();
		float g = RandomFloat();
		float b = RandomFloat();
		mat->color = { r,g,b };
		mat->roughness = RandomFloat();
		const float span = Spans[it % 3];
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
    
    // camera
    vec3 camPos = { 0,1.0f,10.0f };

    float rotx = 0;
    float roty = 0;

    {
        mat4 xMat = (rotationx(rotx));
        mat4 yMat = (rotationy(roty));
        mat4 cameraTransform = multiply(yMat, xMat);

        
        cameraTransform.m30 = camPos.x;
        cameraTransform.m31 = camPos.y;
        cameraTransform.m32 = camPos.z;

        rt.SetViewMatrix(cameraTransform);
        
        int NumberOfRays = rt.Raytrace();
		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

		std::cout << "Time: " << duration.count()/1000.0f << "\n";
		std::cout << "Number of Rays: " << NumberOfRays << "\n";
		std::cout << "MRays/s: " << (NumberOfRays/1'000'000.0f)/(duration.count()/1000.0f) << "\n";
		std::cout << "Resolution: " << w << "x" << h << "\n";
		std::cout << "Rays Per Pixel: " << raysPerPixel << "\n";
		std::cout << "Max Bounces: " << maxBounces << "\n";
		std::cout << "Number of Spheres: " << spheresAmount << "\n";

		std::vector<uint8_t> framebufferInt;

		for (int y = h - 1; y >= 0; y--)
		{
			for (int x = 0; x < w; x++)
			{
				framebufferInt.push_back(std::clamp(int(framebuffer[size_t(w * y + x)].r * 255), 0, 255));
				framebufferInt.push_back(std::clamp(int(framebuffer[size_t(w * y + x)].g * 255), 0, 255));
				framebufferInt.push_back(std::clamp(int(framebuffer[size_t(w * y + x)].b * 255), 0, 255));
			}
		}

		stbi_write_png("Frame.png", w, h, 3, framebufferInt.data(), w*3);
    }

    return 0;
} 
