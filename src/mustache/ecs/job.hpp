#pragma once

#include <cstdint>
#include <mustache/ecs/world.hpp>
#include <mustache/ecs/entity_manager.hpp>
#include <mustache/ecs/job_arg_parcer.hpp>
#include <mustache/ecs/world_filter.hpp>
#include <mustache//ecs/task_view.hpp>
#include <mustache/ecs/base_job.hpp>
#include <mustache/utils/dispatch.hpp>

namespace mustache {

    class Dispatcher;
    class Archetype;
    class World;

    template<typename T>
    class PerEntityJob : public BaseJob {
    public:
        using Info = JobInfo<T>;

        PerEntityJob() {
            filter_result_.mask = Info::componentMask();
        }

        ComponentIdMask checkMask() const noexcept override {
            return ComponentIdMask{};
        }

        ComponentIdMask updateMask() const noexcept override {
            return Info::updateMask();
        }

        MUSTACHE_INLINE void runCurrentThread(World& world) override {
            static constexpr auto unique_components = std::make_index_sequence<Info::FunctionInfo::components_count>();
            static constexpr auto shared_components = std::make_index_sequence<Info::FunctionInfo::shared_components_count>();
            auto& dispatcher = world.dispatcher();
            PerEntityJobTaskId task_id = PerEntityJobTaskId::make(0);
            const auto thread_id = dispatcher.currentThreadId();
            for (auto task : JobView::make(filter_result_, 1)) {
                singleTask(task, task_id, thread_id, unique_components, shared_components);
                ++task_id;
            }
        }

        MUSTACHE_INLINE void runParallel(World& world, uint32_t task_count) override {
            static constexpr auto unique_components = std::make_index_sequence<Info::FunctionInfo::components_count>();
            static constexpr auto shared_components = std::make_index_sequence<Info::FunctionInfo::shared_components_count>();

            PerEntityJobTaskId task_id = PerEntityJobTaskId::make(0);
            auto& dispatcher = world.dispatcher();
            for (auto task : JobView::make(filter_result_, task_count)) {
                dispatcher.addParallelTask([task, this, task_id](ThreadId thread_id) {
                    singleTask(task, task_id, thread_id, unique_components, shared_components);
                });
                ++task_id;
            }
            dispatcher.waitForParallelFinish();
        }

        virtual std::string name() const noexcept override {
            return type_name<T>();
        }
    protected:

        template<typename... _ARGS>
        MUSTACHE_INLINE void forEachArrayGenerated(ComponentArraySize count, JobInvocationIndex& invocation_index,
                                   _ARGS MUSTACHE_RESTRICT_PTR ... pointers) {
            T& self = *static_cast<T*>(this);
            if constexpr (Info::has_for_each_array) {
                invokeMethod(self, &T::forEachArray, count, invocation_index, pointers...);
            } else {
                for(ComponentArraySize i = ComponentArraySize::make(0); i < count; ++i) {
                    invoke(self, invocation_index, pointers++...);
                    ++invocation_index.entity_index_in_task;
                }
            }
        }

        template<size_t _I, typename _ViewType>
        MUSTACHE_INLINE auto getComponentHandler(const _ViewType& view, ComponentIndex index) noexcept {
            using RequiredType = typename Info::FunctionInfo::template UniqueComponentType<_I>::type;
            using Component = typename ComponentType<RequiredType>::type;
            if constexpr (IsComponentRequired<RequiredType>::value) {
                auto ptr = view.template getData<FunctionSafety::kUnsafe>(index);
                return RequiredComponent<Component> {reinterpret_cast<Component*>(ptr)};
            } else {
                // TODO: it is possible to avoid per array check.
                auto ptr = view.template getData<FunctionSafety::kSafe>(index);
                return OptionalComponent<Component> {reinterpret_cast<Component*>(ptr)};
            }
        }

        template<size_t... _I, size_t... _SI>
        MUSTACHE_INLINE void singleTask(TaskView task_view, PerEntityJobTaskId task_id, ThreadId thread_id,
                                        const std::index_sequence<_I...>&, const std::index_sequence<_SI...>&) {
            JobInvocationIndex invocation_index;
            invocation_index.task_index = task_id;
            invocation_index.entity_index_in_task = PerEntityJobEntityIndexInTask::make(0);
            invocation_index.thread_id = thread_id;
            auto shared_components = std::make_tuple(
                    static_cast<typename Info::FunctionInfo::template SharedComponentType<_SI>::type>(nullptr)...);
            for (const auto& info : task_view) {
                auto& archetype = *info.archetype();
                archetype.getSharedComponents(shared_components);
                static const std::array<ComponentId, sizeof...(_I)> ids {
                        ComponentFactory::registerComponent<typename ComponentType<typename Info::FunctionInfo::
                        template UniqueComponentType<_I>::type>::type>()...
                };
                std::array<ComponentIndex, sizeof...(_I)> component_indexes {
                        archetype.getComponentIndex(ids[_I])...
                };
                for (auto& array : ArchetypeView{filter_result_, info.archetype_index,
                                                      info.first_entity, info.current_size}) {
                    if constexpr (Info::FunctionInfo::Position::entity >= 0) {
                        forEachArrayGenerated(ComponentArraySize::make(array.arraySize()), invocation_index,
                                              RequiredComponent<Entity>(array.getEntity<FunctionSafety::kUnsafe>()),
                                              getComponentHandler<_I>(array, component_indexes[_I])...,
                                              SharedComponent{std::get<_SI>(shared_components)}...);
                    } else {
                        forEachArrayGenerated(ComponentArraySize::make(array.arraySize()), invocation_index,
                                              getComponentHandler<_I>(array, component_indexes[_I])...,
                                              SharedComponent{std::get<_SI>(shared_components)}...);
                    }
                }
            }
        }

    };

    template<typename _F, typename... ARGS>
    void EntityManager::forEachWithArgsTypes(_F&& function, JobRunMode mode) {
        static std::string job_name = "";
        if (job_name.empty()) {
            std::string str = ((type_name<ARGS>() + ", ") + ... + "");
            if (!str.empty()) {
                str.pop_back();
                str.pop_back();
            }
            job_name = "ForEachJob<" + str + ">";
        }
        struct TmpJob : public PerEntityJob<TmpJob> {
            TmpJob(_F&& f):
                    func{std::forward<_F>(f)} {

            }
            _F&& func;
            void operator() (ARGS... args) {
                func(std::forward<ARGS>(args)...);
            }

            virtual std::string name() const noexcept override {
                return job_name;
            }
        };
        TmpJob job{std::forward<_F>(function)};
        job.run(world_, mode);
    }
}
