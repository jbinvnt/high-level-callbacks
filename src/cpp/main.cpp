#include "sample_updater/updater_adaptor.h"

Napi::Object InitializeEach(Napi::Env env, Napi::Object exports) {
    //Sample::Updater::UpdaterAdaptor<double, double> adaptor;
    return Sample::Updater::UpdaterAdaptor<double, double>::Initialize(env, exports);
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, InitializeEach)