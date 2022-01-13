#include <mustache/ecs/world.hpp>
#include <mustache/ecs/job.hpp>
#include <glm/matrix.hpp>
#include <mustache/utils/benchmark.hpp>

namespace {
    struct Position {
        glm::vec3 value;
    };
    struct DistToCamera {
        float value;
    };
    struct Velocity {
        glm::vec3 value;
    };
    struct Acceleration {
        glm::vec3 value;
    };
    struct Tangent {
        glm::vec3 value;
    };
    struct Bitangent {
        glm::vec3 value;
    };

    float randf() {
        return static_cast<float >(rand()) / static_cast<float >(RAND_MAX);
    }
    float randf(float min, float max) {
        return glm::mix(min, max, randf());
    }
    struct UpdateDistToCamaraJob : public mustache::PerEntityJob<UpdateDistToCamaraJob> {
        glm::vec3 camera;
        void operator() (const Position& pos, DistToCamera& dist_to_camera) {
            dist_to_camera.value = glm::distance(pos.value, camera);
        }
    };
//    struct CopyDataJob : public mustache::PerEntityJob<CopyDataJob> {
//        void forEachArray(mustache::ComponentArraySize size,
//                          const Position* pos, const Tangent* tangent, const Bitangent* bitangent) {
//
//        }
//    };
}

void createAlphaNodes(mustache::World& world, uint32_t count) {
    auto& entities = world.entities();
    for (uint32_t i = 0; i < count; ++i) {
        entities.begin()
            .assign<Position>(glm::vec3{randf(), randf(), randf()})
            .assign<DistToCamera>()
        .end();
    }
}


void createParticles(mustache::World& world, uint32_t count) {
    auto& entities = world.entities();
    for (uint32_t i = 0; i < count; ++i) {
        entities.begin()
                /// base class fields
                .assign<Position>(glm::vec3{randf(), randf(), randf()})
                .assign<DistToCamera>(randf())
                /// particles fields
                .assign<Tangent>()
                .assign<Bitangent>()
                .assign<Velocity>(glm::vec3{randf(), randf(), randf()})
                .assign<Acceleration>(glm::vec3{randf(), randf(), randf()})
            .end();
    }
}


void runParticleTest() {
    mustache::World world;
    UpdateDistToCamaraJob update_dist_to_camara_job;

    constexpr uint32_t kNumExtra = 10000;
    constexpr uint32_t kNumParticles = 10000;

    createAlphaNodes(world, kNumExtra);
    createParticles(world, kNumParticles);

    mustache::Benchmark benchmark;

    std::vector<float> buffer;
    buffer.resize(9 * kNumParticles);
    constexpr mustache::JobRunMode mode = mustache::JobRunMode::kCurrentThread;
    benchmark.add([&] {
        float dt = 1.0f / 60.0f;
        glm::vec3 camera {0.0f, 0.0f, 0.0f};
        world.entities().forEach([dt](const Acceleration& acceleration, Velocity& velocity, Position& position) {
            const auto dvel = dt * acceleration.value;
            position.value += velocity.value * dt + dvel * dt * 0.5f;
            velocity.value += dvel;
        }, mode);

        world.entities().forEach([camera, &buffer](const Position& position, Tangent& tangent, Bitangent& bitangent,
                                                mustache::JobInvocationIndex invocation_index) {
            constexpr glm::vec3 up{0, 1, 0};
            constexpr glm::vec3 right{1, 0, 0};
            const auto normal = glm::normalize(camera - position.value);
            const bool use_up = glm::distance(up, normal) > 0.01f;
            const auto tmp = glm::cross(normal, use_up ? up : right);
            tangent.value = glm::normalize(glm::cross(normal, tmp));
            bitangent.value = glm::normalize(glm::cross(normal, tangent.value));

            glm::vec3* begin = reinterpret_cast<glm::vec3*>(buffer.data() + invocation_index.entity_index.toInt() * 3);
            *begin = position.value;
            ++begin;
            *begin = tangent.value;
            ++begin;
            *begin = bitangent.value;
            ++begin;
        }, mode);
    }, 100);
    benchmark.show();
}
