#include "user_api.hpp"

#include <glm/gtc/matrix_inverse.hpp>
#include <imgui.h>
#include <stdio.h>

constexpr vec4 RED = { 1, 0, 0, 1 };
constexpr vec4 GREEN = {0, 1, 0, 1};
constexpr vec4 BLUE = {0, 0, 1, 1};

struct Vert_pos_normal { glm::vec3 pos, normal; };

void createCylinderMeshData(u32& numVerts, u32& numInds, Vert_pos_normal* verts, u32* inds,
    float radius, float minY, float maxY, u32 resolution)
{
    assert(resolution >= 3);
    numVerts = 4 * resolution;
    numInds = 2*3 * (resolution - 2) + resolution * 6;

    if(verts)
    {
        // bottom cap
        for(int i = 0; i < resolution; i++) {
            const float alpha = (2*PI * i) / resolution;
            const float x = sin(alpha);
            const float z = cos(alpha);
            verts[i] = {vec3{x, minY, z}, vec3{0, -1, 0}};
        }
        // top cap
        for(int i = 0; i < resolution; i++) {
            vec3 pos = verts[i].pos;
            pos.y = maxY;
            verts[resolution + i] = {pos, {0, +1, 0}};
        }
        // sides
        for(int i = 0; i < resolution; i++) {
            const vec3 normal = {verts[i].pos.x, 0, verts[i].pos.z};
            verts[2*resolution + 2*i + 0] = {verts[i].pos, normal};
            verts[2*resolution + 2*i + 1] = {verts[resolution+i].pos, normal};
        }
        for(int i = 0; i < 4*resolution; i++) {
            verts[i].pos.x *= radius;
            verts[i].pos.z *= radius;
        }
    }

    if(inds)
    {
        // bottom cap
        for(int i = 0; i < resolution-2; i++) {
            inds[3*i + 0] = 0;
            inds[3*i + 1] = i+2;
            inds[3*i + 2] = i+1;
        }
        // top cap
        int offset = 3 * (resolution-2);
        for(int i = 0; i < resolution-2; i++) {
            inds[offset + 3*i + 0] = resolution + 0;
            inds[offset + 3*i + 1] = resolution + i+1;
            inds[offset + 3*i + 2] = resolution + i+2;
        }
        // sides
        offset += offset;
        for(int i = 0; i < resolution; i++) {
            const int a0 = i;
            const int a1 = (i+1) % resolution;
            const int b0 = resolution + i;
            const int b1 = resolution + (i+1) % resolution;
            inds[offset + 6*i + 0] = a0;
            inds[offset + 6*i + 1] = a1;
            inds[offset + 6*i + 2] = b1;
            inds[offset + 6*i + 3] = a0;
            inds[offset + 6*i + 4] = b1;
            inds[offset + 6*i + 5] = b0;
        }
    }
}

static bool wireframe;
static u32 numVerts;
static u32 numInds;
static Vert_pos_normal verts[1<<20];
static u32 inds[1<<20];

void userInit()
{
    createCylinderMeshData(numVerts, numInds, verts, inds, 0.1, 0, 0.2, 8);
}

void drawGui()
{
    ImGui::Begin("user");
    ImGui::Checkbox("wireframe", &wireframe);
    ImGui::End();
}

void userDraws(float dt)
{
    drawGui();

    pushColor(RED);

    /*for(int i = 0; i < numVerts; i++)
    {
        drawPoint(verts[subDivs][i]);
    }*/

    for(int i = 0; i < numInds; i+=3)
    {
        vec3 p0 = verts[inds[i]].pos;
        vec3 p1 = verts[inds[i+1]].pos;
        vec3 p2 = verts[inds[i+2]].pos;
            
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

    pushColor(BLUE);
    for(int i = 0; i < numVerts; i++)
    {
        const vec3 p = verts[i].pos;
        const vec3 n = 0.05f * verts[i].normal;
        drawLine(p, p+n);
    }

    popColor();
}