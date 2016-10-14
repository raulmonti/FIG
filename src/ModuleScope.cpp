#include "ModuleScope.h"

shared_ptr<CompositeModuleScope> CompositeModuleScope::instance_ = nullptr;
std::once_flag CompositeModuleScope::singleInstance_;
