#ifndef UPDATER_PROGRAM_H
#define UPDATER_PROGRAM_H
namespace Sample {
    namespace Updater {
        template <class VertexType, class ContextType>
        class UpdaterProgram {
        public:
            UpdaterProgram(const VertexType& v, const ContextType& c) : vert(v), ctx(c) {}
            virtual void update(VertexType& v, ContextType& c) = 0;
            void run() {
                for (int i{ 0 }; i < 10; i++) {
                    update(this->vert, this->ctx);
                }
            }
            VertexType getVertex() {
                return this->vert;
            }
            ContextType getContext() {
                return this->ctx;
            }
        protected:
            VertexType vert;
            ContextType ctx;
        };
    }
}
#endif
