#ifndef UPDATER_ADAPTOR_H
#define UPDATER_ADAPTOR_H
#include <napi.h>
#include "vendor/updater_program.h"
namespace Sample {
    namespace Updater {
        template <class VertexType, class ContextType>
        class UpdaterAdaptor : public UpdaterProgram<VertexType, ContextType>, public Napi::ObjectWrap<UpdaterAdaptor<VertexType, ContextType>> {
        public:
            // The info[0] and info[1] represent the arguments passed to the constructor in JavaScript. They are used to initialize the UpdaterProgram class
            UpdaterAdaptor(const Napi::CallbackInfo& info) : UpdaterProgram(info[0].As<Napi::Number>().DoubleValue(), info[1].As<Napi::Number>().DoubleValue()), Napi::ObjectWrap<UpdaterAdaptor>(info), executionEnv(info.Env()) {}

            static Napi::Object Initialize(Napi::Env env, Napi::Object exports) {
                Napi::Function updater = DefineClass(env, "Updater", {
                    InstanceMethod("setUpdate", &UpdaterAdaptor::setUpdate),
                    InstanceMethod("run", &UpdaterAdaptor::runUpdate),
                    InstanceMethod("vertex", &UpdaterAdaptor::getVertexWrap),
                    InstanceMethod("context", &UpdaterAdaptor::getContextWrap),
                });
                exports.Set("Updater", updater);
                // The below is boilerplate configuration for object wrapping [10].
                Napi::FunctionReference* constructor = new Napi::FunctionReference();
                *constructor = Napi::Persistent(updater);
                env.SetInstanceData<Napi::FunctionReference>(constructor);
                return exports;
            }
            void update(VertexType& v, ContextType& c) override {
                Napi::Number newV(executionEnv, Napi::Value::From<VertexType>(this->executionEnv, v));
                Napi::Number newC(executionEnv, Napi::Value::From<ContextType>(this->executionEnv, c));
                Napi::Number result(this->executionEnv, this->updateFunction.Call({ newV, newC }));
                // A full implementation would allow conversion to/from full classes for VertexType and ContextType. See the [writeup section](#Limitations).
                v = newC.DoubleValue();
                c = result.DoubleValue();
            }

            void setUpdate(const Napi::CallbackInfo& info) {
                Napi::Function jsCallback = info[0].As<Napi::Function>();
                this->updateFunction = Napi::Persistent(jsCallback);
            }

            void runUpdate(const Napi::CallbackInfo& info) {
                this->run();
            }

            Napi::Value getVertexWrap(const Napi::CallbackInfo& info) {
                return Napi::Value::From<VertexType>(this->executionEnv, this->vert);
            }

            Napi::Value getContextWrap(const Napi::CallbackInfo& info) {
                return Napi::Value::From<ContextType>(this->executionEnv, this->ctx);
            }
        private:
            Napi::FunctionReference updateFunction;
            Napi::Env executionEnv;
        };
    }
}
#endif