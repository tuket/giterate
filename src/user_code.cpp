#include "user_api.hpp"

#include <glm/gtc/matrix_inverse.hpp>
#include <imgui.h>
#include <stdio.h>

mat2 rotY(float a)
{
	return mat2 {
		glm::cos(a), -glm::sin(a),
		glm::sin(a), glm::cos(a)
	};
}

constexpr int maxSubDivs = 8;
static int subDivs = 6;
static vec3* verts[maxSubDivs+1] = {};
static int numVerts[maxSubDivs+1] = {};
static int* inds[maxSubDivs+1] = {};
static int numInds[maxSubDivs+1] = {};

void generateIcospheres(int subDivs)
{
	if(verts[subDivs] != nullptr)
		return;

	const int fnl = 1 << subDivs; // num levels per face
	const int nl = 3 * fnl; // num levels
	const int nv = numVerts[subDivs] =
		2 + // top and bot verts
		2 * 5 * (fnl-1) * fnl / 2 + // top and bot caps
		5 * (fnl+1) * fnl; // middle
	const int ni = numInds[subDivs] = 4 * numInds[subDivs-1];
	vec3* vv = verts[subDivs] = new vec3[nv];
	int* ii = inds[subDivs] = new int[ni];
	vv[0] = verts[0][0];

	using glm::mix;
	int vvi = 1;
	for(int l = 1; l < fnl; l++) {
		const float vertPercent = float(l) / fnl;
		for(int f = 0; f < 5; f++) {
			const vec3 pLeft = mix(verts[0][0], verts[0][1+f], vertPercent);
			const vec3 pRight = mix(verts[0][0], verts[0][1+(f+1)%5], vertPercent);
			for(int x = 0; x < l; x++) {
				const vec3 p = mix(pLeft, pRight, float(x) / l);
				vv[vvi] = p;
				vvi++;
			}
		}
	}

	for(int l = 0 ; l < fnl; l++) {
		const float vertPercent = float(l) / fnl;
		for(int f = 0; f < 5; f++) {
			const vec3 topLeft = verts[0][1+f];
			const vec3 topRight = verts[0][1+(f+1)%5];
			const vec3 botLeft = verts[0][6+f];
			const vec3 botRight = verts[0][6+(f+1)%5];
			const vec3 left = mix(topLeft, botLeft, vertPercent);
			const vec3 mid = mix(topRight, botLeft, vertPercent);
			const vec3 right = mix(topRight, botRight, vertPercent);
			for(int x = 0; x < fnl-l; x++) {
				const vec3 p = mix(left, mid, float(x) / (fnl-l));
				vv[vvi] = p;
				if(f != 0)
					vv[vvi] = {0,0,0};
				vvi++;
			}
			for(int x = 0; x < l; x++) {
				const vec3 p = mix(mid, right, float(x) / l);
				vv[vvi] = p;
				if(f != 0)
					vv[vvi] = {0,0,0};
				vvi++;
			}
		}
	}

	for(int l = 0; l < fnl; l++) {
		const float vertPercent = float(l) / fnl;
		for(int f = 0; f < 5; f++) {
			const vec3 pLeft = mix(verts[0][6+f], verts[0][11], vertPercent);
			const vec3 pRight = mix(verts[0][6+(f+1)%5], verts[0][11], vertPercent);
			for(int x = 0; x < fnl-l; x++) {
				const vec3 p = mix(pLeft, pRight, float(x) / (fnl-l));
				vv[vvi] = p;
				vvi++;
			}
		}
	}

	vv[vvi] = verts[0][11];
	vvi++;

	printf("%d - %d\n", vvi, nv);
	assert(vvi == nv);
}

void userInit()
{
	using glm::sqrt;

	//const mat2 rot72 = rotY(0.4f * PI);
	const mat2 rot36 = rotY(0.2f * PI);

	const float r = 1 / (2*sin(0.2f*PI));
	const float h = sqrt(1 - r*r);
	const float a = sqrt(r*r - 0.25f);
	const float h2 = 0.5f * sqrt(1 - (r-a)*(r-a) - 0.25f);

	numVerts[0] = 12;
	verts[0] = new vec3[numVerts[0]];
	numInds[0] = 20*3;
	inds[0] = new int[numInds[0]];

	verts[0][0] = vec3(0, h2+h, 0);
	vec2 p(r, 0);
	for(int i = 0; i < 5; i++) {
		verts[0][1+i] = {p.y, h2, -p.x};
		p = rot36 * p;
		verts[0][6+i] = {p.y, -h2, -p.x};
		p = rot36 * p;
	}
	verts[0][11] = -verts[0][0];

	for(int i = 0; i < 5; i++) {
		const int i1 = (i+1) % 5;
		inds[0][3 * i + 0] = 0;
		inds[0][3 * i + 1] = 1+i;
		inds[0][3 * i + 2] = 1+i1;
	}
	for(int i = 0; i < 5; i++) {
		const int i1 = (i+1) % 5;
		inds[0][3*(5 + i) + 0] = 1+i;
		inds[0][3*(5 + i) + 1] = 6+i;
		inds[0][3*(5 + i) + 2] = 1+i1;
	}
	for(int i = 0; i < 5; i++) {
		const int i1 = (i+1) % 5;
		inds[0][3*(10 + i) + 0] = 6+i;
		inds[0][3*(10 + i) + 1] = 6+i1;
		inds[0][3*(10 + i) + 2] = 1+i1;
	}
	for(int i = 0; i < 5; i++) {
		const int i1 = (i+1) % 5;
		inds[0][3*(15 + i) + 0] = 11;
		inds[0][3*(15 + i) + 1] = 6+i1;
		inds[0][3*(15 + i) + 2] = 6+i;
	}

	for(int subDivs = 1; subDivs <= maxSubDivs; subDivs++) {
		verts[subDivs] = nullptr;
		inds[subDivs] = nullptr;
	}

	generateIcospheres(subDivs);
}

void drawGui()
{
	ImGui::Begin("user");
	if(ImGui::SliderInt("Sub-divisions", &subDivs, 0, maxSubDivs)) {
		subDivs = glm::max(0, subDivs);
		generateIcospheres(subDivs);
	}
	ImGui::End();
}

constexpr vec4 RED = { 1, 0, 0, 1 };
constexpr vec4 GREEN = {0, 1, 0, 1};
constexpr vec4 BLUE = {0, 0, 1, 1};

void userDraws(float dt)
{
	drawGui();

	pushColor(RED);

	/*for(int i = 0; i < 5; i++) {
		const int i1 = (i+1)%5;
		drawLine(verts[0], verts[1+i]);
		drawLine(verts[1+i], verts[1+i1]);
		drawLine(verts[6+i], verts[6+i1]);
		drawLine(verts[1+i], verts[6+i]);
		drawLine(verts[6+i], verts[1+i1]);
		drawLine(verts[11], verts[6+i]);
	}*/

	/*
	generateIcospheres(subDivs);
	pushColor({0, 1, 0, 0.25f});
	for(int i = 0; i < numInds[subDivs]; i+=3) {
		drawTriangle(verts[subDivs][inds[subDivs][i+0]],
					 verts[subDivs][inds[subDivs][i+1]],
					 verts[subDivs][inds[subDivs][i+2]]);
	}
	popColor();
	*/
	for(int i = 0; i < numVerts[subDivs]; i++)
	{
		drawPoint(verts[subDivs][i]);
	}

/*
	for(int i = 0; i < 5; i++) {
		const int i1 = (i+1)%5;
		drawTriangle(pentagon1[i], pentagon1[i1], topVert);
		drawTriangle(pentagon1[i], pentagon2[i], pentagon1[i1]);
		drawTriangle(pentagon2[i], pentagon2[i1], pentagon1[i1]);
		drawTriangle(pentagon2[i], -topVert, pentagon2[i1]);
		//drawTriangle(topVert, pentagon1[i0], pentagon1[i1]);
	}
*/
	popColor();
}