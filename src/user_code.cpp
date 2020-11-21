#include "user_api.hpp"

#include <glm/gtc/matrix_inverse.hpp>
#include <imgui.h>
#include <stdio.h>

static float distance = 3, pitch = 0, heading = 0;

void userInit()
{
}

void drawGui()
{
	ImGui::Begin("user");
	ImGui::SliderFloat("distance", &distance, 0, 100);
	ImGui::SliderAngle("pitch", &pitch, 0, 90);
	ImGui::SliderAngle("heading", &heading, 0, 360);
	ImGui::End();
}

constexpr vec4 RED = { 1, 0, 0, 1 };
constexpr vec4 GREEN = {0, 1, 0, 1};
constexpr vec4 BLUE = {0, 0, 1, 1};

static glm::mat4 calcCameraViewMtx(float distance, float pitch, float heading)
{
    const float sp = glm::sin(pitch);
    const float cp = glm::cos(pitch);
    const float sh = glm::sin(heading);
    const float ch = glm::cos(heading);
    const vec3 dir = {
        cp * sh,
        sp,
        cp * ch,
    };
    const vec3 up = {
    	-sp * sh,
    	cp,
    	-sp * ch
    };
    const vec3 right = { ch, 0, -sh };
    glm::mat4 m = glm::transpose(mat3(right, up, dir));
    m[3] = {0, 0, -distance, 1};
    return m;
}

void userDraws(float dt)
{
	drawGui();

	const auto m = calcCameraViewMtx(distance, pitch, heading);
	const vec3 pos = m[3];
	const vec3 X = m[0];
	//printf("%g %g %g\n", X.x, X.y, X.z);
	const vec3 Y = m[1];
	const vec3 Z = m[2];

	pushColor(RED);
	drawLine(pos, pos + X);
	pushColor(GREEN);
	drawLine(pos, pos + Y);
	pushColor(BLUE);
	drawLine(pos, pos + Z);

	/*for(int i = 0; i < numVerts[subDivs]; i++)
	{
		drawPoint(verts[subDivs][i]);
	}*/

	popColor();
}