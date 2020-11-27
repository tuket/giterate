#include "user_api.hpp"

#include <glm/gtc/matrix_inverse.hpp>
#include <imgui.h>
#include <stdio.h>



#include <assert.h>
#include <stdint.h>

namespace tl
{

template <typename T>
class Span
{
public:
    Span();
    Span(T* data, size_t size);
    Span(T* from, T* to);
    template <size_t N>
    Span(T (&data)[N]);
    Span(Span& o);
    Span(const Span& o);
    operator T*() { return _data; }
    operator const T*()const { return _data; }
    operator Span<const T>()const { return {_data, _size}; }

    T& operator[](size_t i);
    const T& operator[](size_t i)const;

    T* begin() { return _data; }
    T* end() { return _data + _size; }
    const T* begin()const { return _data; }
    const T* end()const { return _data + _size;}
    size_t size()const { return _size; }

    Span<T> subArray(size_t from, size_t to);
    const Span<T> subArray(size_t from, size_t to)const; // "to" is not included i.e: [from, to)

private:
    T* _data;
    size_t _size;
};

// ---------------------------------------------------------------------------------------------
template <typename T>
Span<T>::Span()
    : _data(nullptr)
    , _size(0)
{}

template <typename T>
Span<T>::Span(T* data, size_t size)
    : _data(data)
    , _size(size)
{}

template <typename T>
Span<T>::Span(T* from, T* to)
    : _data(from)
    , _size(to - from)
{}

template <typename T>
template <size_t N>
Span<T>::Span(T (&data)[N])
    : _data(&data[0])
    , _size(N)
{}

template <typename T>
Span<T>::Span(Span<T>& o)
    : _data(o._data)
    , _size(o._size)
{}

template <typename T>
Span<T>::Span(const Span<T>& o)
    : _data(o._data)
    , _size(o._size)
{}

template <typename T>
T& Span<T>::operator[](size_t i) {
    assert(i < _size);
    return _data[i];
}

template <typename T>
const T& Span<T>::operator[](size_t i)const {
    assert(i < _size);
    return _data[i];
}

template <typename T>
Span<T> Span<T>::subArray(size_t from, size_t to) {
    assert(from <= to && to <= _size);
    return Span<T>(_data + from, _data + to);
}

template <typename T>
const Span<T> Span<T>::subArray(size_t from, size_t to)const {
    assert(from <= to && to <= _size);
    return Span<T>(_data + from, _data + to);
}

template <typename T>
using CSpan = Span<const T>;

}

constexpr vec4 RED = { 1, 0, 0, 1 };
constexpr vec4 GREEN = {0, 1, 0, 1};
constexpr vec4 BLUE = {0, 0, 1, 1};

struct Vert_pos_tc {
    glm::vec3 pos;
    glm::vec2 tc;
};
struct Vert {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 tc; // texture coord
};

struct MeshBuilder {
    Vert* verts; // reserved memory for writing vertices
    u32* inds; // reserved memory for writing indices
    u32 numVerts = 0;
    u32 numInds = 0;
    void addQuadStrip(tl::CSpan<Vert_pos_tc> v);
};

void MeshBuilder::addQuadStrip(tl::CSpan<Vert_pos_tc> v)
{
    assert(v.size() >= 4 && v.size() % 2 == 0);

    // vertices
    for(int i = 0; i < v.size(); i += 2)
    {
        const vec3 vm = v[i+1].pos - v[i].pos;
        vec3 N[2];
        if(i == 0) { // first edge
            const vec3 a1 = v[i+2].pos - v[i].pos;
            const vec3 b1 = v[i+3].pos - v[i+1].pos;
            N[0] = cross(vm, a1);
            N[1] = cross(vm, b1);
        }
        else if(i == v.size() - 2) { // last edge
            const vec3 a0 = v[i].pos - v[i-2].pos;
            const vec3 b0 = v[i+1].pos - v[i-1].pos;
            N[0] = cross(vm, a0);
            N[1] = cross(vm, b0);
        }
        else { // one edge in the middle
            const vec3 a0 = v[i].pos - v[i-2].pos;
            const vec3 b0 = v[i+1].pos - v[i-1].pos;
            const vec3 a1 = v[i+2].pos - v[i].pos;
            const vec3 b1 = v[i+3].pos - v[i+1].pos;
            N[0] = normalize(cross(vm, a0)) + normalize(cross(vm, a1));
            N[1] = normalize(cross(vm, b0)) + normalize(cross(vm, b1));
        }
        N[0] = normalize(N[0]);
        N[1] = normalize(N[1]);

        verts[numVerts + i+0] = {v[i+0].pos, N[0], v[i+0].tc};
        verts[numVerts + i+1] = {v[i+1].pos, N[1], v[i+1].tc};
    }

    // indices
    for(int i = 0; i < v.size()-2; i += 2)
    {
        inds[numInds + 3*i + 0] = numVerts + i + 0;
        inds[numInds + 3*i + 1] = numVerts + i + 1;
        inds[numInds + 3*i + 2] = numVerts + i + 3;
        inds[numInds + 3*i + 3] = numVerts + i + 0;
        inds[numInds + 3*i + 4] = numVerts + i + 3;
        inds[numInds + 3*i + 5] = numVerts + i + 2;
    }

    numVerts += v.size();
    const int numQuads = (v.size() - 2) / 2; 
    numInds += 6 * numQuads;
}

static u32 s_numVerts;
static u32 s_numInds;
static Vert s_verts[4 << 10];
static u32 s_inds[4 << 10];

void userInit()
{
    MeshBuilder mb = {s_verts, s_inds};
    const float L = 3;
    Vert_pos_tc pp[2*30] = {
        {{-1, 0, -1}, vec2{}},
        {{-1, 0, +1}, vec2{}},
        {{+1, 0, -1}, vec2{}},
        {{+1, 0, +1}, vec2{}},
    };
    for(int i = 0; i < 30; i++) {
        const float l = (i*L) / 30; 
        const float h = 0.3*sin(2*l);
        pp[2*i+0] = {{l, h, -1}, {}};
        pp[2*i+1] = {{l, h, +1}, {}};
    }
    mb.addQuadStrip(pp);
    s_numVerts = mb.numVerts;
    s_numInds = mb.numInds;
    for(int i = 0; i < 6; i++) {
        printf("%d\n", s_inds[i]);
    }
}

void drawGui()
{
}

void userDraws(float dt)
{
    drawGui();

    pushColor(RED);
    for(int i = 0; i < s_numInds; i += 3)
    {
        drawTriangle(
            s_verts[s_inds[i+0]].pos,
            s_verts[s_inds[i+1]].pos,
            s_verts[s_inds[i+2]].pos
        );
    }

    pushColor(BLUE);
    for(int i = 0; i < s_numVerts; i++)
    {
        const vec3 p = s_verts[i].pos;
        const vec3 N = s_verts[i].normal;
        drawLine(p, p + 0.1f*N);
    }

    popColor();
}