#include "user_api.hpp"

#include <glm/gtc/matrix_inverse.hpp>
#include <imgui.h>

void userInit()
{

}

int phase = 0;

union {
	unsigned uInt;
	struct {
		bool originalFrustum : 1;
		bool invTR : 1;
		bool P : 1;
		bool S : 1;
		bool allInOne : 1;
		bool basis : 1;
	};

} flags;

void drawGui()
{
	ImGui::Begin("user");
	ImGui::CheckboxFlags("Original frustum", &flags.uInt, 1);
	ImGui::CheckboxFlags("invTR", &flags.uInt, 1 << 1);
	ImGui::CheckboxFlags("P", &flags.uInt, 1 << 2);
	ImGui::CheckboxFlags("S", &flags.uInt, 1 << 3);
	ImGui::CheckboxFlags("allInOne", &flags.uInt, 1 << 4);
	ImGui::CheckboxFlags("basis", &flags.uInt, 1 << 5);
	ImGui::End();
}

constexpr vec4 RED = { 1, 0, 0, 1 };
constexpr vec4 GREEN = {0, 1, 0, 1};
constexpr vec4 BLUE = {0, 0, 1, 1};

void userDraws(float dt)
{
	drawGui();

	vec3 pp[] = {
		vec3{9005.59961, 1892.62, -711.854980},
		vec3{-14079.7998, -6845.97021, 23258.0996},
		vec3{-16129.7998, -6845.97021, -19866.9004},
		vec3{-17058.1992, 657.554016, -19822.8008},
		vec3{-15008.2002, 657.554016, 23302.1992}
	};
	const float scale = 1.000f;
	for (int i = 0; i < 5; i++)
		pp[i] *= scale;

	auto drawFrust4 = [&](const vec3* vv)
	{
		for (int i = 0; i < 4; i++)
		{
			drawLine(pp[0], vv[i]);
			drawLine(vv[i], vv[(i+1) % 4]);
		}
	};
	auto drawFrust5 = [&](const vec3* vv)
	{
		for (int i = 1; i < 5; i++)
		{
			drawLine(vv[0], vv[i]);
			drawLine(vv[i], vv[i % 4 + 1]);
		}
	};

	if(flags.originalFrustum)
		drawFrust4(pp + 1);

	vec3 v[4];
	for (int i = 1; i < 5; i++)
	{
		v[i - 1] = pp[i] - pp[0];
		v[i - 1] = normalize(v[i - 1]);
	}
	const vec3 axisX = normalize(v[1] - v[0]);
	const vec3 axisY = normalize(v[3] - v[0]);
	const vec3 axisZ = normalize(v[0] + v[1] + v[2] + v[3]);
	const float t0 = dot(v[0], axisZ);
	const float t1 = dot(v[1], axisZ);
	const float t2 = dot(v[2], axisZ);
	const float t3 = dot(v[3], axisZ);
	const float xDist = dot(v[1], axisX) / dot(v[1], axisZ);
	const float yDist = dot(v[3], axisY) / dot(v[2], axisZ);

	/*pushColor({ 0, 1, 0, 1 });
	drawLine(pp[0] + v[0], pp[0] + v[1]);
	drawLine(pp[0] + v[3], pp[0] + v[2]);
	drawLine(pp[0] + v[0], pp[0] + v[3]);
	popColor();*/

	const mat4 TR = {
		vec4(axisX, 0),
		vec4(axisY, 0),
		vec4(axisZ, 0),
		vec4(pp[0], 1)
	};
	const mat4 invTR = glm::affineInverse(TR);

	const mat4 P{
		{1, 0, 0, 0},
		{0, 1, 0, 0},
		{0, 0, 1, 1},
		{0, 0, 0, 0}
	};
	const mat4 S{
		{1 / xDist, 0, 0, 0},
		{0, 1 / yDist, 0, 0},
		{0, 0, 1, 0},
		{0, 0, 0, 1}
	};
	mat4 frustumProj = invTR * P;

	vec3 res[4][5];
	for (int i = 0; i < 5; i++)
		res[0][i] = invTR * vec4(pp[i], 1);
	
	if(flags.invTR)
		drawFrust5(res[0]);

	for (int i = 0; i < 5; i++) {
		vec4 v4 = P * vec4(res[0][i], 1);
		res[1][i] = v4 / v4.w;
	}

	if (flags.P)
		drawFrust5(res[1]);

	for (int i = 0; i < 5; i++) {
		res[2][i] = S * vec4(res[1][i], 1);
	}

	if (flags.S)
		drawFrust5(res[2]);

	for (int i = 0; i < 5; i++) {
		vec4 v4 = S * P * invTR * vec4(pp[i], 1);
		res[3][i] = v4 / v4.w;
	}

	if (flags.allInOne)
		drawFrust5(res[3]);

	const vec3 basis[3] = {
		{-0.0744673461, -0.00976325851, -0.997175634},
		{-0.161663443, 0.985808134, 0.0452456661},
		{-0.977452576, -0.197151959, 0.0756144673}
	};
	float dots[3][3];
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			dots[i][j] = dot(basis[i], basis[j]);
	if (flags.basis)
	{
		pushColor(RED);
		drawLine({}, basis[0]);
		popColor();
		pushColor(GREEN);
		drawLine({}, basis[1]);
		popColor();
		pushColor(BLUE);
		drawLine({}, basis[2]);
		popColor();
	}
}