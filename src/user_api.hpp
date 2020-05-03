#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

void userInit();
void userDraws(float dt);

typedef uint32_t u32;
typedef int32_t i32;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat2;
using glm::mat3;
using glm::mat4;

constexpr float PI = glm::pi<float>();

void pushColor(glm::vec4 c);
void popColor();

void drawPoint(vec3 a);
void drawLine(vec3 a, vec3 b);
void drawTriangle(vec3 a, vec3 b, vec3 c);