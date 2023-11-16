#include "raylib.h"
#include "rlgl.h"
#include <math.h>
#include <stdint.h>

#define NOB_IMPLEMENTATION
#include "nob.h"

Mesh GenMesh(uint8_t* data, size_t map_size, float size);
Camera ResetCamera();

int main(int argc, char **argv) {
    const char *program = nob_shift_args(&argc, &argv);

    if (argc <= 0) {
        nob_log(NOB_ERROR, "No input is provided");
        nob_log(NOB_INFO, "Usage: %s <input>", program);
        return 1;
    }

    Nob_String_Builder content = {0};

    const char *input_file_path = argv[0];
    content.count = 0;
    nob_log(NOB_INFO, "Reading %s", input_file_path);
    if (!nob_read_entire_file(input_file_path, &content))
        return 1;
    
    int map_size = cbrt((float) content.count / sizeof(uint8_t));
    if (map_size * map_size * map_size * sizeof(uint8_t) != content.count) {
        nob_log(NOB_ERROR, "Size of %d is not a cube of a natural number! File corrupted.", input_file_path);
        return 1;
    }

    nob_log(NOB_INFO, "Size: %d ^ 3", map_size);

    InitWindow(800, 450, "binviz3D viewer");

    Camera camera = ResetCamera();

    Mesh shellMesh = GenMesh((uint8_t*) content.items, map_size, 10.0f);
    Model shellModel = LoadModelFromMesh(shellMesh);
    Shader shader = LoadShader(0, "shader.fs");
    shellModel.materials[0].shader = shader;

    Vector3 cubePosition = { 0.0f, 5.0f, 0.0f };

    DisableCursor();

    SetTargetFPS(30);
    rlDisableBackfaceCulling();

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_R)) {
            camera = ResetCamera();
        }
        UpdateCamera(&camera, CAMERA_THIRD_PERSON);
        BeginDrawing();
        ClearBackground(BLACK);
        BeginMode3D(camera);

        BeginShaderMode(shader);
        DrawModel(shellModel, (Vector3){-5.0f, 0.0f, -5.0f}, 1.0f, RED);
        EndShaderMode();

        DrawCubeWires(cubePosition, 10.0f, 10.0f, 10.0f, WHITE);
        DrawGrid(10, 1.0f);
        EndMode3D();
        DrawText("ESC - Exit, R - Reset camera", 5, 5, 16, LIGHTGRAY);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}

Camera ResetCamera() {
    Camera camera = {0};
    camera.position = (Vector3){ 3.0f, 8.0f, 3.0f };
    camera.target = (Vector3){ 0.0f, 5.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    return camera;
}

Mesh GenMesh(uint8_t* data, size_t map_size, float size) {
    Vector3 *vertices = (Vector3 *)RL_MALLOC(map_size * map_size * map_size * 4 * sizeof(Vector3));
    Vector2 *texcoords = (Vector2 *)RL_MALLOC(map_size * map_size * map_size * 4 * sizeof(Vector2));
    int quads = 0;
    for (size_t x = 0; x < map_size; x++) {
        for (size_t y = 0; y < map_size; y++) {
            for (size_t z = 0; z < map_size; z++) {
                uint8_t point = data[z * map_size * map_size + x * map_size + y];
                if (point != 0x00) {
                    int index = quads * 4;
                    vertices[index] =     (Vector3){size * x / map_size,       size * y / map_size,       size * z / map_size};
                    vertices[index + 1] = (Vector3){size * (x + 1) / map_size, size * y / map_size,       size * z / map_size};
                    vertices[index + 2] = (Vector3){size * x / map_size,       size * (y + 1) / map_size, size * z / map_size};
                    vertices[index + 3] = (Vector3){size * (x + 1) / map_size, size * (y + 1) / map_size, size * z / map_size};

                    float color = point / 256.0f;
                    Vector2 color2D = {color, color};
                    texcoords[index] = color2D;
                    texcoords[index + 1] = color2D;
                    texcoords[index + 2] = color2D;
                    texcoords[index + 3] = color2D;

                    quads++;
                }
            }
        }
    }

    unsigned short* triangles = (unsigned short*)RL_MALLOC(quads * 6 * sizeof(unsigned short));
    for (int i = 0; i < quads; i++) {
        triangles[i * 6] = i * 4;
        triangles[i * 6 + 1] = i * 4 + 1;
        triangles[i * 6 + 2] = i * 4 + 2;

        triangles[i * 6 + 3] = i * 4 + 2;
        triangles[i * 6 + 4] = i * 4 + 1;
        triangles[i * 6 + 5] = i * 4 + 3;
    }

    Mesh mesh = {0};

    mesh.vertexCount = quads * 4;
    mesh.triangleCount = quads * 2;
    mesh.vertices = (float *)RL_MALLOC(mesh.vertexCount * 3 * sizeof(float));
    mesh.indices = (unsigned short *)RL_MALLOC(mesh.triangleCount * 3 * sizeof(unsigned short));
    mesh.texcoords = (float *)RL_MALLOC(mesh.vertexCount * 2 * sizeof(float));

    memcpy(mesh.vertices, vertices, mesh.vertexCount * 3 * sizeof(float));
    memcpy(mesh.indices, triangles, mesh.triangleCount * 3 * sizeof(unsigned short));
    memcpy(mesh.texcoords, texcoords, mesh.vertexCount * 2 * sizeof(float));

    RL_FREE(vertices);
    RL_FREE(triangles);

    // Upload vertex data to GPU (static mesh)
    UploadMesh(&mesh, false);

    return mesh;
}
