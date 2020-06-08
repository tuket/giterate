#include <stdio.h>
#include <assert.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <vector>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "user_api.hpp"

static void glErrorCallback(const char* name, void* funcptr, int len_args, ...) {
	GLenum error_code;
	error_code = glad_glGetError();
	if (error_code != GL_NO_ERROR) {
		//fprintf(stderr, "ERROR %s in %s\n", gluErrorString(error_code), name);
		assert(false);
	}
}

const char* VERT_SHADER_SRC =
R"GLSL(
#version 330 core
uniform mat4 u_viewProj;

in layout(location = 0) vec3 a_pos;
in layout(location = 1) vec4 a_color; 

out vec4 v_color;

void main()
{
	gl_Position = u_viewProj * vec4(a_pos, 1.0);
	v_color = a_color;
}
)GLSL";

const char* FRAG_SHAD_SRC =
R"GLSL(
#version 330 core
out layout (location = 0) vec4 o_color;

in vec4 v_color;

void main()
{
	o_color = v_color;
}
)GLSL";

constexpr int BUFFER_SIZE = 4 * 1024;
char s_buffer[4 * 1024];

GLFWwindow* window;

char* getShaderCompileErrors(u32 shader)
{
	i32 ok;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
	if (!ok) {
		glGetShaderInfoLog(shader, BUFFER_SIZE, nullptr, s_buffer);
		return s_buffer;
	}
	return nullptr;
}
char* getShaderLinkErrors(u32 prog)
{
	GLint success;
	glGetProgramiv(prog, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(prog, BUFFER_SIZE, nullptr, s_buffer);
		return s_buffer;
	}
	return nullptr;
}

struct Point {
	vec3 pos;
	vec4 color;
};

struct Line {
	Point a, b;
};

struct Triangle {
	Point a, b, c;
};

struct FpsCamera {
	float rotateSpeed = PI;
	float moveSpeed = 1;
	float heading, pitch;
	vec3 pos;
	void feedMouseDelta(vec2 d);
	void feedDir(vec2 dir, float dt);
	mat4 getMtx()const;
};

void FpsCamera::feedMouseDelta(vec2 d)
{
	heading -= d.x * rotateSpeed;
	while (heading < -0.5f * PI)
		heading += 2*PI;
	while (heading > +0.5f * PI)
		heading -= 2*PI;
	pitch -= d.y * rotateSpeed;
	pitch = glm::clamp(pitch, -0.45f * PI, +0.45f * PI);
}

void FpsCamera::feedDir(vec2 dir, float dt)
{
	const mat4 m = glm::eulerAngleYX(heading, pitch);
	const vec3 dir3 = m * vec4(dir.x, 0, -dir.y, 0);
	pos += moveSpeed * dir3 * dt;
}

mat4 FpsCamera::getMtx()const
{
	mat4 m = glm::eulerAngleYX(heading, pitch);
	m[3] = vec4(pos, 1);
	return m;
}

static struct PressState {
	bool w : 1;
	bool a : 1;
	bool s : 1;
	bool d : 1;
	bool leftMouse;
} s_pressed;

static vec2 s_mousePos(0, 0);

struct RenderData {
	u32 shaderProg;
	u32 pointsVbo, pointsVao;
	u32 linesVbo, linesVao;
	u32 trianglesVbo, trianglesVao;

	struct UnifLocs {
		i32 viewProj;
	} unifLocs;
} s_renderData;

struct UserData {
	struct Camera {
		float fovY = PI / 4;
		FpsCamera fps;
	} camera;

	struct Flags {
		bool showAxes : 1;
	};
	union {
		Flags flags;
		unsigned flagsUInt;
	};

	void init();
} g_userData;

void UserData::init()
{
	camera.fps.pos = { 0.1, 0.1, 1 };
	flags.showAxes = true;
}

struct State { // this current state of the frame
	std::vector<vec4> color;
	std::vector<mat4> mtx;
	std::vector<Point> points;
	std::vector<Line> lines;
	std::vector<Triangle> triangles;
} s_state;

void pushColor(vec4 c) { s_state.color.push_back(c); }
void popColor() { s_state.color.pop_back(); }

void pushMtx(mat4 m)
{
	s_state.mtx.push_back(s_state.mtx.back() * m);
}
void popMtx() { s_state.mtx.pop_back(); }

void drawPoint(vec3 a)
{
	const mat4 m = s_state.mtx.back();
	const vec4 color = s_state.color.back();
	a = m * vec4(a, 1);
	s_state.points.push_back(Point{ a, color });
}

void drawLine(vec3 a, vec3 b)
{
	const mat4 m = s_state.mtx.back();
	const vec4 color = s_state.color.back();
	a = m * vec4(a, 1);
	b = m * vec4(b, 1);
	s_state.lines.push_back({
		Point{a, color},
		Point{b, color}
	});
}

static void startRender()
{
	s_state.color.resize(1);
	s_state.color[0] = { 1,1,1,1 };
	s_state.mtx.resize(1);
	s_state.mtx[0] = mat4(1);
	s_state.points.clear();
	s_state.lines.clear();
	s_state.triangles.clear();
}

static void endRender()
{
	int w, h;
	glfwGetWindowSize(window, &w, &h);
	glViewport(0, 0, w, h);
	glScissor(0, 0, w, h);
	glClearColor(0, 0.1f, 0.2f, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	const mat4 viewMtx = glm::affineInverse(g_userData.camera.fps.getMtx());
	const mat4 projMtx = glm::perspective(g_userData.camera.fovY, float(w) / h, 0.02f, 10000.f);
	const mat4 viewProjMtx = projMtx * viewMtx;
	glUniformMatrix4fv(s_renderData.unifLocs.viewProj, 1, GL_FALSE, &viewProjMtx[0][0]);

	// lines
	{
		const u32 n = s_state.lines.size();
		glBindBuffer(GL_ARRAY_BUFFER, s_renderData.linesVbo);
		glBufferData(GL_ARRAY_BUFFER, n * sizeof(Line), s_state.lines.data(), GL_STREAM_DRAW);
		//glBufferSubData(GL_VERTEX_ARRAY, 0, n * sizeof(Line), s_state.lines.data());
		glBindVertexArray(s_renderData.linesVao);
		glDrawArrays(GL_LINES, 0, 2*n);
	}

	// points
	{
		const u32 n = s_state.points.size();
		glBindBuffer(GL_ARRAY_BUFFER, s_renderData.pointsVbo);
		glBufferData(GL_ARRAY_BUFFER, n * sizeof(Point), s_state.points.data(), GL_STREAM_DRAW);
		//glBufferSubData(GL_VERTEX_ARRAY, 0, n * sizeof(Line), s_state.lines.data());
		glBindVertexArray(s_renderData.pointsVao);
		glDrawArrays(GL_POINTS, 0, n);
	}
}

static void onKey(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	const bool pressed = action == GLFW_PRESS || action == GLFW_REPEAT;
	switch (key)
	{
	case GLFW_KEY_W:
		s_pressed.w = pressed;
		break;
	case GLFW_KEY_A:
		s_pressed.a = pressed;
		break;
	case GLFW_KEY_S:
		s_pressed.s = pressed;
		break;
	case GLFW_KEY_D:
		s_pressed.d = pressed;
		break;
	}
}

static void onMouseButton(GLFWwindow* window, int button, int action, int mods)
{
	if (ImGui::GetIO().WantCaptureMouse)
		return; // the mouse is captured by imgui
	switch (button)
	{
	case GLFW_MOUSE_BUTTON_1:
		s_pressed.leftMouse = action == GLFW_PRESS;
	}
}

static void onMouseMove(GLFWwindow* window, double x, double y)
{
	if (s_pressed.leftMouse)
	{
		int w, h;
		glfwGetWindowSize(window, &w, &h);
		const vec2 d = (vec2{ x, y } - s_mousePos) / vec2{w, h};
		g_userData.camera.fps.feedMouseDelta(d);
	}
	s_mousePos = { x, y };
}

void processInput(float dt)
{
	vec2 dir = { 0, 0 };
	if (s_pressed.w)
		dir.y += 1;
	if (s_pressed.s)
		dir.y -= 1;
	if (s_pressed.a)
		dir.x -= 1;
	if (s_pressed.d)
		dir.x += 1;
	if (dir != vec2{0, 0})
	{
		dir = normalize(dir);
		g_userData.camera.fps.feedDir(dir, dt);
	}
}

static void appDraws()
{
	if (g_userData.flags.showAxes)
	{
		pushColor({ 1,0,0,1 });
			drawLine({ 0,0,0 }, { 1000,0,0 });
		popColor();
		pushColor({ 0,1,0,1 });
			drawLine({ 0,0,0 }, { 0,1000,0 });
		popColor();
		pushColor({ 0,0,1,1 });
			drawLine({ 0,0,0 }, { 0,0,1000 });
		popColor();
	}
}

static void drawGui()
{
	ImGui::Begin("giterator", 0, 0);
	ImGui::CheckboxFlags("Show exes", &g_userData.flagsUInt, 0x1);
	if (ImGui::TreeNode("Camera"))
	{
		ImGui::SliderAngle("Rotation speed", &g_userData.camera.fps.rotateSpeed, 0, 360);
		ImGui::SliderFloat("Move speed", &g_userData.camera.fps.moveSpeed, 0, 10000, "%.5f", 5);
		ImGui::TreePop();
	}

	ImGui::End();
}

int main()
{
	glfwSetErrorCallback(+[](int error, const char* description) {
		fprintf(stderr, "Glfw Error %d: %s\n", error, description);
		});
	if (!glfwInit())
		return 1;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE); // https://github.com/glfw/glfw/issues/1499

	window = glfwCreateWindow(1360, 960, "giterate", nullptr, nullptr);
	if (window == nullptr)
		return 2;

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync

	if (gladLoadGL() == 0) {
		fprintf(stderr, "Failed to initialize OpenGL loader!\n");
		return 3;
	}
	glad_set_post_callback(glErrorCallback);

	glfwSetMouseButtonCallback(window, onMouseButton);
	glfwSetCursorPosCallback(window, onMouseMove);
	glfwSetKeyCallback(window, onKey);
	//glfwSetScrollCallback(window, onMouseWheel);

	{
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsClassic();

		// Setup Platform/Renderer bindings
		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init();
	}
	
	g_userData.init();

	{
		s_renderData.shaderProg = glCreateProgram();
		const u32 vertShad = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertShad, 1, &VERT_SHADER_SRC, nullptr);
		glCompileShader(vertShad);
		if (const char* errorMsg = getShaderCompileErrors(vertShad)) {
			printf("vertShad compile error:\n%s\n", errorMsg);
			assert(false);
		}
		glAttachShader(s_renderData.shaderProg, vertShad);

		const u32 fragShad = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragShad, 1, &FRAG_SHAD_SRC, nullptr);
		glCompileShader(fragShad);
		if (const char* errorMsg = getShaderCompileErrors(fragShad)) {
			printf("fragShad compile error:\n%s\n", errorMsg);
			assert(false);
		}
		glAttachShader(s_renderData.shaderProg, fragShad);

		glLinkProgram(s_renderData.shaderProg);
		if (const char* errorMsg = getShaderLinkErrors(s_renderData.shaderProg)) {
			printf("link errors:\n%s\n", errorMsg);
			assert(false);
		}
		glUseProgram(s_renderData.shaderProg);
		glGetUniformLocation(s_renderData.shaderProg, "u_viewProj");
	}

	{ // points
		glGenVertexArrays(1, &s_renderData.pointsVao);
		glBindVertexArray(s_renderData.pointsVao);
		glGenBuffers(1, &s_renderData.pointsVbo);
		glBindBuffer(GL_ARRAY_BUFFER, s_renderData.pointsVbo);
		glBufferData(GL_ARRAY_BUFFER, 1, nullptr, GL_STREAM_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point), nullptr);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)(sizeof(vec3)));
	}

	{ // lines
		glGenVertexArrays(1, &s_renderData.linesVao);
		glBindVertexArray(s_renderData.linesVao);
		glGenBuffers(1, &s_renderData.linesVbo);
		glBindBuffer(GL_ARRAY_BUFFER, s_renderData.linesVbo);
		glBufferData(GL_ARRAY_BUFFER, 1, nullptr, GL_STREAM_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point), nullptr);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)(sizeof(vec3)));
	}

	userInit();

	float t0 = glfwGetTime();
	while (!glfwWindowShouldClose(window))
	{
		const float t1 = glfwGetTime();
		const float dt = t1 - t0;
		t0 = t1;

		glfwPollEvents();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		drawGui();

		processInput(dt);

		startRender();
		appDraws();
		userDraws(dt);
		endRender();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		//glfwWaitEventsTimeout(0.01);
	}

	return 0;
}