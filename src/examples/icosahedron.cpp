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

constexpr int maxSubDivs = 6;
static int subDivs = 3;
static vec3* verts[maxSubDivs+1] = {};
static int numVerts[maxSubDivs+1] = {};
static int* inds[maxSubDivs+1] = {};
static int numInds[maxSubDivs+1] = {};
static bool wireframe = true;
static bool enableNormalize = false;

/*static void generateIcosahedronVerts(vec3 verts[12])
{
	using glm::sqrt;
	const mat2 rot36 = rotY(0.2f * PI);

	const float r = 1 / (2*sin(0.2f*PI));
	const float h = sqrt(1 - r*r);
	const float a = sqrt(r*r - 0.25f);
	const float h2 = 0.5f * sqrt(1 - (r-a)*(r-a) - 0.25f);

	verts[0] = vec3(0, h2+h, 0);
	vec2 p(r, 0);
	for(int i = 0; i < 5; i++) {
		verts[1+i] = {p.y, h2, -p.x};
		p = rot36 * p;
		verts[6+i] = {p.y, -h2, -p.x};
		p = rot36 * p;
	}
	verts[11] = -verts[0];

	for(int i = 0; i < 12; i++)
		verts[i] = glm::normalize(verts[i]);
}*/
static const vec3 icosahedronVerts[12] = {
	{0.0000000000000000000000000, 1.0000000000000000000000000, 0.0000000000000000000000000},
	{0.0000000000000000000000000, 0.4472136497497558593750000, -0.8944272398948669433593750},
	{-0.8506507873535156250000000, 0.4472136497497558593750000, -0.2763932347297668457031250},
	{-0.5257311463356018066406250, 0.4472136497497558593750000, 0.7236067652702331542968750},
	{0.5257310271263122558593750, 0.4472136497497558593750000, 0.7236068248748779296875000},
	{0.8506507873535156250000000, 0.4472136497497558593750000, -0.2763930857181549072265625},
	{-0.5257310867309570312500000, -0.4472136497497558593750000, -0.7236068248748779296875000},
	{-0.8506507873535156250000000, -0.4472136497497558593750000, 0.2763931453227996826171875},
	{-0.0000000626720364493849047, -0.4472136497497558593750000, 0.8944271802902221679687500},
	{0.8506507277488708496093750, -0.4472136497497558593750000, 0.2763932645320892333984375},
	{0.5257312059402465820312500, -0.4472136497497558593750000, -0.7236067056655883789062500},
	{-0.0000000000000000000000000, -1.0000000000000000000000000, -0.0000000000000000000000000},
};

void generateIcosphere(int& numVerts, int& numInds, vec3** verts, int** inds, int subDivs, bool dontNormalize = false)
{
	const int fnl = 1 << subDivs; // num levels per face
	const int nl = 3 * fnl; // num levels
	numVerts =
		2 + // top and bot verts
		2 * 5 * (fnl-1) * fnl / 2 + // top and bot caps
		5 * (fnl+1) * fnl; // middle

	numInds = 3 * (20 * (1 << (2 * subDivs)));

	if(verts)
	{
		int vi = 0;
		auto addVert = [&](vec3 p) {
			(*verts)[vi++] = /*glm::normalize*/(p);
		};
		addVert(icosahedronVerts[0]);

		using glm::mix;
		for(int l = 1; l < fnl; l++) {
			const float vertPercent = float(l) / fnl;
			for(int f = 0; f < 5; f++) {
				const vec3 pLeft = mix(icosahedronVerts[0], icosahedronVerts[1+f], vertPercent);
				const vec3 pRight = mix(icosahedronVerts[0], icosahedronVerts[1+(f+1)%5], vertPercent);
				for(int x = 0; x < l; x++) {
					const vec3 p = mix(pLeft, pRight, float(x) / l);
					addVert(p);
				}
			}
		}

		for(int l = 0 ; l < fnl; l++) {
			const float vertPercent = float(l) / fnl;
			for(int f = 0; f < 5; f++) {
				const vec3 topLeft = icosahedronVerts[1+f];
				const vec3 topRight = icosahedronVerts[1+(f+1)%5];
				const vec3 botLeft = icosahedronVerts[6+f];
				const vec3 botRight = icosahedronVerts[6+(f+1)%5];
				const vec3 left = mix(topLeft, botLeft, vertPercent);
				const vec3 mid = mix(topRight, botLeft, vertPercent);
				const vec3 right = mix(topRight, botRight, vertPercent);
				for(int x = 0; x < fnl-l; x++) {
					const vec3 p = mix(left, mid, float(x) / (fnl-l));
					addVert(p);
				}
				for(int x = 0; x < l; x++) {
					const vec3 p = mix(mid, right, float(x) / l);
					addVert(p);
				}
			}
		}

		for(int l = 0; l < fnl; l++) {
			const float vertPercent = float(l) / fnl;
			for(int f = 0; f < 5; f++) {
				const vec3 pLeft = mix(icosahedronVerts[6+f], icosahedronVerts[11], vertPercent);
				const vec3 pRight = mix(icosahedronVerts[6+(f+1)%5], icosahedronVerts[11], vertPercent);
				for(int x = 0; x < fnl-l; x++) {
					const vec3 p = mix(pLeft, pRight, float(x) / (fnl-l));
					addVert(p);
				}
			}
		}

		addVert(icosahedronVerts[11]);
		assert(vi == numVerts);
	}

	if(inds)
	{
		int i = 0;
		auto addTri = [&](int i0, int i1, int i2) {
			(*inds)[i++] = i0;
			(*inds)[i++] = i1;
			(*inds)[i++] = i2;
		};

		// top
		for(int f = 0; f < 5; f++)
			addTri(0, 1+f, 1+(f+1)%5);
		
		int rowOffset = 1;
		for(int l = 1; l < fnl; l++) {
			const int rowLen = l*5;
			const int nextRowLen = (l+1)*5;
			for(int f = 0; f < 5; f++) {
				for(int x = 0; x < l; x++) {
					const int topLeft = rowOffset + l*f + x;
					const int topRight = rowOffset + (l*f + x + 1) % rowLen;
					const int botLeft = rowOffset + rowLen + (l+1) * f + x;
					addTri(
						topLeft,
						botLeft,
						botLeft + 1);
					addTri(
						topLeft,
						botLeft + 1,
						topRight);
				}
				addTri(
					rowOffset + (l*(f+1)) % rowLen, // review!
					rowOffset + rowLen + (l+1) * f + l,
					rowOffset + rowLen + ((l+1) * f + l + 1) % nextRowLen);
			}
			rowOffset += rowLen;
		}

		{ // middle
			const int rowLen = 5 * fnl;
			for(int l = 0; l < fnl; l++) {
				for(int x = 0; x < rowLen; x++) {
					const int topLeft = rowOffset + x;
					const int topRight = rowOffset + (x+1) % rowLen;
					const int botLeft = rowOffset + rowLen + x;
					const int botRight = rowOffset + rowLen + (x+1) % rowLen;
					addTri(topLeft, botLeft, topRight);
					addTri(topRight, botLeft, botRight);
				}
				rowOffset += rowLen;
			}
		}

		// bottom
		for(int l = 0; l < fnl-1; l++) {
			const int faceLen = fnl - l;
			const int rowLen = faceLen*5;
			const int nextRowLen = (faceLen-1)*5;
			for(int f = 0; f < 5; f++) {
				for(int x = 0; x < fnl-l-1; x++) {
					const int topLeft = rowOffset + faceLen*f + x;
					const int topRight = rowOffset + faceLen*f + x + 1;
					const int botLeft = rowOffset + rowLen + (faceLen-1) * f + x;
					const int botRight = rowOffset + rowLen + ((faceLen-1) * f + x + 1) % nextRowLen;
					addTri(topLeft, botLeft, topRight);
					addTri(topRight, botLeft, botRight);
				}
				addTri(
					rowOffset + faceLen*(f+1) - 1,
					rowOffset + rowLen + ((faceLen-1) * (f+1)) % nextRowLen,
					rowOffset + faceLen*(f+1) % rowLen);
			}
			rowOffset += rowLen;
		}
		for(int f = 0; f < 5; f++)
			addTri(numVerts-6+f, numVerts-1, numVerts-6 + (f+1)%5);

		assert(i == numInds);
	}
}

void userInit()
{
	for(int subDivs = 1; subDivs <= maxSubDivs; subDivs++) {
		verts[subDivs] = nullptr;
		inds[subDivs] = nullptr;
	}

	if(verts[subDivs] == nullptr) {
		generateIcosphere(numVerts[subDivs], numInds[subDivs], nullptr, nullptr, subDivs);
		verts[subDivs] = new vec3[numVerts[subDivs]];
		inds[subDivs] = new int[numInds[subDivs]];
		generateIcosphere(numVerts[subDivs], numInds[subDivs], &verts[subDivs], &inds[subDivs], subDivs);
	}
}

void drawGui()
{
	ImGui::Begin("user");
	if(ImGui::SliderInt("Sub-divisions", &subDivs, 0, maxSubDivs)) {
		subDivs = glm::max(0, subDivs);
		if(verts[subDivs] == nullptr) {
			generateIcosphere(numVerts[subDivs], numInds[subDivs], nullptr, nullptr, subDivs);
			verts[subDivs] = new vec3[numVerts[subDivs]];
			inds[subDivs] = new int[numInds[subDivs]];
			generateIcosphere(numVerts[subDivs], numInds[subDivs], &verts[subDivs], &inds[subDivs], subDivs);
		}
	}
	ImGui::Checkbox("wireframe", &wireframe);
	ImGui::Checkbox("normalize", &enableNormalize);
	ImGui::End();
}

constexpr vec4 RED = { 1, 0, 0, 1 };
constexpr vec4 GREEN = {0, 1, 0, 1};
constexpr vec4 BLUE = {0, 0, 1, 1};

void userDraws(float dt)
{
	drawGui();

	pushColor(RED);

	/*for(int i = 0; i < numVerts[subDivs]; i++)
	{
		drawPoint(verts[subDivs][i]);
	}*/

	for(int i = 0; i < numInds[subDivs]; i+=3)
	{
		vec3 p0 = verts[subDivs][inds[subDivs][i]];
		vec3 p1 = verts[subDivs][inds[subDivs][i+1]];
		vec3 p2 = verts[subDivs][inds[subDivs][i+2]];
		if(enableNormalize) {
			p0 = glm::normalize(p0);
			p1 = glm::normalize(p1);
			p2 = glm::normalize(p2);
		}
			
		if(wireframe) {
			drawLine(p0, p1);
			drawLine(p1, p2);
			drawLine(p2, p0);
		}
		else {
			drawTriangle(p0, p1, p2);
		}
	}

	popColor();
}